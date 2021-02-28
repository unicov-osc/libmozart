/**
 * Mozart++ Template Library
 * Licensed under Apache 2.0
 * Copyright (C) 2020-2021 Chengdu Covariant Technologies Co., LTD.
 * Website: https://covariant.cn/
 * Github:  https://github.com/chengdu-zhirui/
 */
#include <mozart++/core>

#ifdef MOZART_PLATFORM_UNIX

#include <mozart++/process>
#include <mozart++/string>
#include <dirent.h>
#include <cerrno>
#include <fcntl.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <cctype>
#include <climits>
#include <sys/stat.h>
#include <sys/wait.h>
#include <csignal>

#ifdef MOZART_PLATFORM_DARWIN
#define FD_DIR "/dev/fd"
#define dirent64 dirent
#define readdir64 readdir
#else
#define FD_DIR "/proc/self/fd"
#endif

#ifdef MOZART_PLATFORM_DARWIN
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#else
extern char **environ;
#endif

namespace mpp_impl {
    /**
     * We use -1 to indicate that a process is still running,
     * bacause the return value of a process can never be -1.
     */
    static constexpr int PROCESS_STILL_ALIVE = -1;
    static constexpr int PROCESS_POLL_FAILED = -2;

    /**
     * Poll child process status without reaping the exitValue.
     * waitid() is standard on all POSIX platforms.
     * Note: waitid on Mac OS X 10.7 seems to be broken;
     * it does not return the exit status consistently.
     */
    static int poll_process_status(int pid) {
        siginfo_t info;
        memset(&info, '\0', sizeof(info));

        if (waitid(P_PID, pid, &info, WEXITED | WSTOPPED | WNOHANG | WNOWAIT) == -1) {
            // cannot get process status at this moment
            // return early in case of undefined behavior.
            return PROCESS_POLL_FAILED;
        }

        switch (info.si_code) {
            case CLD_EXITED:
                // The child exited normally, get its exit code
                return info.si_status;
            case CLD_KILLED:
            case CLD_DUMPED:
                // The child exited because of a signal.
                // The best value to return is 0x80 + signal number,
                // because that is what all Unix shells do, and because
                // it allows callers to distinguish between process exit and
                // oricess death by signal.
                //
                // Breaking changes happens if we are running on solaris:
                // the historical behaviour on Solaris is to return the
                // original signal number, but we will ignore that!
                return 0x80 + WTERMSIG(info.si_status);
            case CLD_STOPPED:
                return 0x80 + WSTOPSIG(info.si_status);
            default:
                // process is still alive
                return PROCESS_STILL_ALIVE;
        }
    }

    static bool close_all_descriptors(int from_fd, int fail_fd) {
        DIR *dp = nullptr;
        struct dirent64 *dirp = nullptr;

        // We're trying to close all file descriptors, but opendir() might\
        // itself be implemented using a file descriptor, and we certainly
        // don't want to close that while it's in use.  We assume that if
        // opendir() is implemented using a file descriptor, then it uses
        // the lowest numbered file descriptor, just like open().  So we
        // close a couple explicitly.

        // for possible use by opendir()
        close(from_fd);
        // another one for good luck
        close(from_fd + 1);

        if ((dp = opendir(FD_DIR)) == nullptr) {
            return false;
        }

        // use readdir64 in case of fd > 1024
        while ((dirp = readdir64(dp)) != nullptr) {
            int fd;
            if (std::isdigit(dirp->d_name[0])
                && (fd = strtol(dirp->d_name, nullptr, 10)) >= from_fd + 2
                && fd != fail_fd) {
                close(fd);
            }
        }

        closedir(dp);
        return true;
    }

    /*
     * Reads nbyte bytes from file descriptor fd into buf,
     * The read operation is retried in case of EINTR or partial reads.
     *
     * Returns number of bytes read (normally nbyte, but may be less in
     * case of EOF).  In case of read errors, returns -1 and sets errno.
     */
    mpp::ssize_t read_fully(int fd, void *buf, size_t nbyte) {
        ssize_t remaining = nbyte;
        while (true) {
            ssize_t n = read(fd, buf, remaining);
            if (n == 0) {
                return nbyte - remaining;
            } else if (n > 0) {
                remaining -= n;
                if (remaining <= 0) {
                    return nbyte;
                }
                // We were interrupted in the middle of reading the bytes.
                // Unlikely, but possible.
                buf = reinterpret_cast<void *>(reinterpret_cast<char *>(buf) + n);
            } else if (errno == EINTR) {
                // we received some strange signals, which interrupted the
                // read system call, we just proceed to continue reading.
            } else {
                return -1;
            }
        }
    }

    /**
     * If PATH is not defined, the OS provides some default value.
     */
    static const char *default_path_env() {
        return ":/bin:/usr/bin";
    }

    static const char *get_path_env() {
        const char *s = getenv("PATH");
        return (s != nullptr) ? s : default_path_env();
    }

    static const char *const *effective_pathv() {
        const char *path = get_path_env();
        // it's safe to convert from size_t to int, :)
        int count = static_cast<int>(mpp::string_ref(path).count(':')) + 1;
        size_t pathvsize = sizeof(const char *) * (count + 1);
        size_t pathsize = strlen(path) + 1;
        const char **pathv = reinterpret_cast<const char **>(malloc(pathvsize + pathsize));

        if (pathv == nullptr) {
            return nullptr;
        }

        char *p = reinterpret_cast<char *>(pathv) + pathvsize;
        memcpy(p, path, pathsize);

        // split PATH by replacing ':' with '\0'
        // and empty components with "."
        for (int i = 0; i < count; i++) {
            char *sep = p + strcspn(p, ":");
            pathv[i] = (p == sep) ? "." : p;
            *sep = '\0';
            p = sep + 1;
        }
        pathv[count] = nullptr;
        return pathv;
    }

    /**
     * Exec file as a shell script but without shebang (#!).
     * This is a historical tradeoff.
     * see GNU libc documentation.
     */
    static void execve_without_shebang(const char *file, const char **argv, char **envp) {
        // Use the extra word of space provided for us in argv by caller.
        const char *argv0 = argv[0];
        const char *const *end = argv;
        while (*end != nullptr) {
            ++end;
        }
        memmove(argv + 2, argv + 1, (end - argv) * sizeof(*end));
        argv[0] = "/bin/sh";
        argv[1] = file;
        execve(argv[0], const_cast<char **>(argv), envp);

        // oops, /bin/sh can't be executed, just fall through
        memmove(argv + 1, argv + 2, (end - argv) * sizeof(*end));
        argv[0] = argv0;
    }

    /**
     * Like execve(2), but the file is always assumed to be a shell script
     * and the system default shell is invoked to run it.
     */
    static void execve_or_shebang(const char *file, const char **argv, char **envp) {
        execve(file, const_cast<char **>(argv), envp);
        // or the shell doesn't provide a shebang
        if (errno == ENOEXEC) {
            execve_without_shebang(file, argv, envp);
        }
    }

    /**
     * mpp implementation of the GNU extension execvpe()
     */
    static void mpp_execvpe(const char *file, const char **argv, char **envp) {
        if (envp == nullptr || envp == environ) {
            execvp(file, const_cast<char *const *>(argv));
            return;
        }

        if (*file == '\0') {
            errno = ENOENT;
            return;
        }

        if (strchr(file, '/') != nullptr) {
            execve_or_shebang(file, argv, envp);

        } else {
            // We must search PATH (parent's, not child's)
            const char *const *pathv = effective_pathv();

            // prepare the full space to avoid memory allocation
            char absolute_path[PATH_MAX] = {0};
            int filelen = strlen(file);
            int sticky_errno = 0;

            for (auto dirs = pathv; *dirs; dirs++) {
                const char *dir = *dirs;
                int dirlen = strlen(dir);
                if (filelen + dirlen + 2 >= PATH_MAX) {
                    errno = ENAMETOOLONG;
                    continue;
                }

                memcpy(absolute_path, dir, dirlen);
                if (absolute_path[dirlen - 1] != '/') {
                    absolute_path[dirlen++] = '/';
                }

                memcpy(absolute_path + dirlen, file, filelen);
                absolute_path[dirlen + filelen] = '\0';
                execve_or_shebang(absolute_path, argv, envp);

                // If permission is denied for a file (the attempted
                // execve returned EACCES), these functions will continue
                // searching the rest of the search path.  If no other
                // file is found, however, they will return with the
                // global variable errno set to EACCES.
                switch (errno) {
                    case EACCES:
                        sticky_errno = errno;
                        // fall-through
                    case ENOENT:
                    case ENOTDIR:
#ifdef ELOOP
                    case ELOOP:
#endif
#ifdef ESTALE
                    case ESTALE:
#endif
#ifdef ENODEV
                    case ENODEV:
#endif
#ifdef ETIMEDOUT
                    case ETIMEDOUT:
#endif
                        // Try other directories in PATH
                        break;
                    default:
                        return;
                }
            }

            // tell the caller the real errno
            if (sticky_errno != 0) {
                errno = sticky_errno;
            }
        }
    }

    static void restartable_write_error(int fail_fd, int errnum) {
        ssize_t result = 0;
        do {
            result = write(fail_fd, &errnum, sizeof(errnum));
        } while ((result == -1) && (errno == EINTR));
    }

    __attribute__((noreturn))
    static void exit_with_error(int fail_fd) {
        // the child failed to exec, tell our parent.
        int errnum = errno;
        restartable_write_error(fail_fd, errnum);
        close(fail_fd);
        _exit(-1);
    }

    __attribute__((noreturn))
    static void child_proc(const process_startup &startup, process_info &info,
                           fd_type *pstdin, fd_type *pstdout, fd_type *pstderr,
                           fd_type *pfail) {
        // close child side of read pipe
        close_fd(pfail[PIPE_READ]);
        int fail_fd = pfail[PIPE_WRITE];

        if (!startup._stdin.redirected()) {
            close_fd(pstdin[PIPE_WRITE]);
        }
        if (!startup._stdout.redirected()) {
            close_fd(pstdout[PIPE_READ]);
        }

        dup2(pstdin[PIPE_READ], STDIN_FILENO);
        dup2(pstdout[PIPE_WRITE], STDOUT_FILENO);

        /*
         * pay special attention to stderr,
         * there are 2 cases:
         *      1. redirect stderr to stdout
         *      2. redirect stderr to a file
         */
        if (startup.merge_outputs) {
            // redirect stderr to stdout
            dup2(pstdout[PIPE_WRITE], STDERR_FILENO);
        } else {
            // redirect stderr to a file
            if (!startup._stderr.redirected()) {
                close_fd(pstderr[PIPE_READ]);
            }
            dup2(pstderr[PIPE_WRITE], STDERR_FILENO);
        }

        close_fd(pstdin[PIPE_READ]);
        close_fd(pstdout[PIPE_WRITE]);
        close_fd(pstderr[PIPE_WRITE]);

        // command-line and environments
        size_t asize = startup._cmdline.size();
        size_t esize = startup._env.size();
        char *argv[asize + 1];
        char *envp[esize + 1];

        // argv and envp are always terminated with a nullptr
        argv[asize] = nullptr;
        envp[esize] = nullptr;

        // copy command-line arguments
        for (std::size_t i = 0; i < asize; ++i) {
            argv[i] = const_cast<char *>(startup._cmdline[i].c_str());
        }

        // copy environment variables
        std::vector<std::string> envs;
        std::stringstream buffer;
        for (const auto &e : startup._env) {
            buffer.str("");
            buffer.clear();
            buffer << e.first << "=" << e.second;
            envs.emplace_back(buffer.str());
        }

        for (std::size_t i = 0; i < esize; ++i) {
            envp[i] = const_cast<char *>(envs[i].c_str());
        }

        // close everything
        if (!close_all_descriptors(STDERR_FILENO + 1, fail_fd)) {
            // try luck failed, close the old way
            int max_fd = static_cast<int>(sysconf(_SC_OPEN_MAX));
            for (int fd = STDERR_FILENO + 1; fd < max_fd; fd++) {
                // do not close fail pipe
                if (fd == fail_fd) {
                    continue;
                }
                if (close(fd) == -1 && errno != EBADF) {
                    // oops, we cannot close this fd
                    MOZART_LOGEV("failed to close inherit fd");
                    continue;
                }
            }
        }

        // change cwd
        if (chdir(startup._cwd.c_str()) != 0) {
            exit_with_error(fail_fd);
            // never return
        }

        // make 100% sure the fail pipe will be closed,
        // or the parent may get stuck in read_fully.
        if (fcntl(fail_fd, F_SETFD, FD_CLOEXEC) == -1) {
            // oops, we lost our double-insurance
            exit_with_error(fail_fd);
            // never return
        }

        // run subprocess
        mpp_execvpe(argv[0], const_cast<const char **>(argv), envp);

        // exec failed
        exit_with_error(fail_fd);
        // never return
    }

    void create_process_impl(const process_startup &startup, process_info &info,
                             fd_type *pstdin, fd_type *pstdout, fd_type *pstderr) {
        // the child_proc will use this pipe to
        // tell parent whether the process has started.
        fd_type pfail[2] = {FD_INVALID, FD_INVALID};
        if (!create_pipe(pfail)) {
            mpp::throw_ex<mpp::runtime_error>("unable to create communication pipe");
        }

        pid_t pid = fork();

        if (pid < 0) {
            mpp::throw_ex<mpp::runtime_error>("unable to fork subprocess");

        } else if (pid == 0) {
            // in child process, pfail will be closed in child_proc
            child_proc(startup, info, pstdin, pstdout, pstderr, pfail);

            // child never returns

        } else {
            // in parent process

            // receive exec call result form child
            close_fd(pfail[PIPE_WRITE]);
            int child_errno = 0;

            switch (read_fully(pfail[PIPE_READ], &child_errno, sizeof(child_errno))) {
                case 0:
                    // child exec succeeded.
                    break;
                case sizeof(child_errno):
                    // child failed to exec, we will wait it.
                    waitpid(pid, nullptr, 0);
                    mpp::throw_ex<mpp::runtime_error>("child exec failed: " + std::string(strerror(child_errno)));
                    break;
                default:
                    mpp::throw_ex<mpp::runtime_error>("read failed: " + std::string(strerror(errno)));
                    break;
            }

            close_fd(pfail[PIPE_READ]);

            if (!startup._stdin.redirected()) {
                close_fd(pstdin[PIPE_READ]);
            }
            if (!startup._stdout.redirected()) {
                close_fd(pstdout[PIPE_WRITE]);
            }

            /*
             * pay special attention to stderr,
             * there are 2 cases:
             *      1. redirect stderr to stdout
             *      2. redirect stderr to a file
             */
            if (startup.merge_outputs) {
                // redirect stderr to stdout
                // do nothing
            } else {
                // redirect stderr to a file
                if (!startup._stderr.redirected()) {
                    close_fd(pstderr[PIPE_WRITE]);
                }
            }

            info._pid = pid;
            info._stdin = pstdin[PIPE_WRITE];
            info._stdout = pstdout[PIPE_READ];
            info._stderr = pstderr[PIPE_READ];

            // on *nix systems, fork() doesn't create threads to run process
            info._tid = FD_INVALID;
        }
    }

    void close_process(process_info &info) {
        mpp_impl::close_fd(info._stdin);
        mpp_impl::close_fd(info._stdout);
        mpp_impl::close_fd(info._stderr);
    }

    int wait_for(const process_info &info) {
        while (true) {
            int status = poll_process_status(info._pid);
            if (status == PROCESS_STILL_ALIVE) {
                // continue waiting
                continue;

            } else if (status == PROCESS_POLL_FAILED) {
                switch (errno) {
                    case ECHILD:
                        // The process specified by pid does not exist
                        // or is not a child of the calling process.
                        return 0;
                    default:
                        // cannot get exit code
                        return -1;
                }

            } else {
                // the process has exited.
                return status;
            }
        }
    }

    void terminate_process(const process_info &info, bool force) {
        kill(info._pid, force ? SIGKILL : SIGTERM);
    }

    bool process_exited(const process_info &info) {
        // if WNOHANG was specified and one or more child(ren)
        // specified by pid exist, but have not yet changed state,
        // then 0 is returned. On error, -1 is returned.
        int status = poll_process_status(info._pid);

        if (status == PROCESS_POLL_FAILED) {
            if (errno != ECHILD) {
                // when WNOHANG was set, errno could only be ECHILD
                mpp::throw_ex<mpp::runtime_error>("should not reach here");
            }

            // waitpid() cannot find the child process identified by pid,
            // there are two cases of this situation depending on signal set
            struct sigaction sa{};
            if (sigaction(SIGCHLD, nullptr, &sa) != 0) {
                // only happens when kernel bug
                mpp::throw_ex<mpp::runtime_error>("should not reach here");
            }

#if defined(MOZART_PLATFORM_DARWIN)
            void *handler = reinterpret_cast<void *>(sa.__sigaction_u.__sa_handler);
#elif defined(MOZART_PLATFORM_LINUX)
            void *handler = reinterpret_cast<void *>(sa.sa_handler);
#endif

            if (handler == reinterpret_cast<void *>(SIG_IGN)) {
                // in this situation we cannot check whether
                // a child process has exited in normal way, because
                // the child process is not belong to us any more, and
                // the kernel will move its owner to init without notifying us.
                // so we will try the fallback method.
                std::string path = std::string("/proc/") + std::to_string(info._pid);
                struct stat buf{};

                // when /proc/<pid> doesn't exist, the process has exited.
                // there will be race conditions: our process exited and
                // another process started with the same pid.
                // to eliminate this case, we should check /proc/<pid>/cmdline
                // but it's too complex and not always reliable.
                return stat(path.c_str(), &buf) == -1 && errno == ENOENT;

            } else {
                // we didn't set SIG_IGN for SIGCHLD
                // there is only one case here theoretically:
                // the child has exited too early before we checked it.
                return true;
            }
        }

        return status != PROCESS_STILL_ALIVE;
    }
}

#endif
