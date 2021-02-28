/**
 * Mozart++ Template Library
 * Licensed under Apache 2.0
 * Copyright (C) 2020-2021 Chengdu Covariant Technologies Co., LTD.
 * Website: https://covariant.cn/
 * Github:  https://github.com/chengdu-zhirui/
 */
#include <mozart++/core>
#ifdef MOZART_PLATFORM_WIN32

#include <mozart++/process>

#include <Windows.h>

namespace mpp_impl {
    void create_process_impl(const process_startup &startup,
                              process_info &info,
                              fd_type *pstdin, fd_type *pstdout, fd_type *pstderr) {
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags |= STARTF_USESTDHANDLES;

        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = true;
        sa.lpSecurityDescriptor = nullptr;

        if (!SetHandleInformation(pstdin[PIPE_WRITE], HANDLE_FLAG_INHERIT, 0)) {
            mpp::throw_ex<mpp::runtime_error>("unable to set handle information on stdin");
        }

        si.hStdInput = pstdin[PIPE_READ];
        si.hStdOutput = pstdout[PIPE_WRITE];

        /*
         * pay special attention to stderr,
         * there are 2 cases:
         *      1. redirect stderr to stdout
         *      2. redirect stderr to a file
         */
        if (startup.merge_outputs) {
            // redirect stderr to stdout
            si.hStdError = pstdout[PIPE_WRITE];
        } else {
            // redirect stderr to a file
            si.hStdError = pstderr[PIPE_WRITE];
        }

        ZeroMemory(&pi, sizeof(pi));

        std::stringstream ss;
        for (const auto &s : startup._cmdline) {
            ss << s << " ";
        }

        std::string command = ss.str();

        char *envs = nullptr;

        if (!startup._env.empty()) {
            // starting from 1, which is the block terminator '\0'
            size_t env_size = 1;
            for (const auto &e : startup._env) {
                // need 2 more, which is the '=' and variable terminator '\0'
                env_size += e.first.length() + e.second.length() + 2;
            }

            envs = new char[env_size]{0};
            char *p = envs;

            for (const auto &e : startup._env) {
                strncat(p, e.first.c_str(), e.first.length());
                p += e.first.length();
                *p++ = '=';
                strncat(p, e.second.c_str(), e.second.length());
                p += e.second.length();
                *p++ = '\0'; // variable terminator
            }
            *p++ = '\0'; // block terminator

            // ensure envs are copied correctly
            if (p != envs + env_size) {
                delete[] envs;
                mpp::throw_ex<mpp::runtime_error>("unable to copy environment variables");
            }
        }

        if (!CreateProcess(nullptr, const_cast<char *>(command.c_str()),
                           nullptr, nullptr, true, CREATE_NO_WINDOW, envs,
                           startup._cwd.c_str(), &si, &pi)) {
            delete[] envs;
            mpp::throw_ex<mpp::runtime_error>("unable to fork subprocess");
        }

        delete[] envs;
        CloseHandle(pstdin[PIPE_READ]);
        CloseHandle(pstdout[PIPE_WRITE]);
        CloseHandle(pstderr[PIPE_WRITE]);

        info._pid = pi.hProcess;
        info._tid = pi.hThread;
        info._stdin = pstdin[PIPE_WRITE];
        info._stdout = pstdout[PIPE_READ];
        info._stderr = pstderr[PIPE_READ];
    }

    void close_process(process_info &info) {
        mpp_impl::close_fd(info._pid);
        mpp_impl::close_fd(info._tid);
        mpp_impl::close_fd(info._stdin);
        mpp_impl::close_fd(info._stdout);
        mpp_impl::close_fd(info._stderr);
    }

    int wait_for(const process_info &info) {
        WaitForSingleObject(info._pid, INFINITE);
        DWORD code = 0;
        GetExitCodeProcess(info._pid, &code);
        return code;
    }

    void terminate_process(const process_info &info, bool force) {
        TerminateProcess(info._pid, 0);
    }

    bool process_exited(const process_info &info) {
        DWORD code = 0;
        GetExitCodeProcess(info._pid, &code);
        return code != STILL_ACTIVE;
    }
}

#endif
