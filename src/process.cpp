/**
 * Mozart++ Template Library
 * Licensed under Apache 2.0
 * Copyright (C) 2020-2021 Chengdu Covariant Technologies Co., LTD.
 * Website: https://covariant.cn/
 * Github:  https://github.com/chengdu-zhirui/
 */

#include <mozart++/process>

namespace mpp_impl {
    bool redirect_or_pipe(const redirect_info &r, fd_type fds[2]) {
        if (!r.redirected()) {
            // no redirect target specified
            return create_pipe(fds);
        }

        fds[PIPE_READ] = r._target;
        fds[PIPE_WRITE] = r._target;
        return true;
    }

    void create_process(const process_startup &startup,
                        process_info &info) {
        fd_type pstdin[2] = {FD_INVALID, FD_INVALID};
        fd_type pstdout[2] = {FD_INVALID, FD_INVALID};
        fd_type pstderr[2] = {FD_INVALID, FD_INVALID};

        if (!redirect_or_pipe(startup._stdin, pstdin)) {
            mpp::throw_ex<mpp::runtime_error>("unable to bind stdin");
        }

        if (!redirect_or_pipe(startup._stdout, pstdout)) {
            close_pipe(pstdin);
            mpp::throw_ex<mpp::runtime_error>("unable to bind stdout");
        }

        if (!startup.merge_outputs) {
            // if the user doesn't redirect stderr to stdout,
            // we bind stderr to a new file descriptor
            if (!redirect_or_pipe(startup._stderr, pstderr)) {
                close_pipe(pstdin);
                close_pipe(pstdout);
                mpp::throw_ex<mpp::runtime_error>("unable to bind stderr");
            }
        }

        try {
            create_process_impl(startup, info, pstdin, pstdout, pstderr);
        } catch (...) {
            // do rollback work
            // note: we should NOT close user provided redirect target fd,
            // let users to close.
            if (!startup._stdin.redirected()) {
                close_pipe(pstdin);
            }
            if (!startup._stdout.redirected()) {
                close_pipe(pstdout);
            }
            if (!startup._stderr.redirected()) {
                close_pipe(pstderr);
            }
            throw;
        }
    }
}

namespace mpp {
    process process::exec(const std::string &command) {
        return process_builder().command(command).start();
    }

    process process::exec(const std::string &command,
                          const std::vector<std::string> &args) {
        return process_builder().command(command).arguments(args).start();
    }
}
