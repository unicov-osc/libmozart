/**
 * Mozart++ Template Library
 * Licensed under Apache 2.0
 * Copyright (C) 2020-2021 Chengdu Covariant Technologies Co., LTD.
 * Website: https://covariant.cn/
 * Github:  https://github.com/chengdu-zhirui/
 */
#pragma once

#include <mozart++/core>
#include <mozart++/fdstream>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <vector>
#include <string>
#include <memory>

namespace mpp_impl {
    using mpp::fd_type;
    using mpp::FD_INVALID;
    using mpp::close_fd;
    using mpp::close_pipe;
    using mpp::create_pipe;
    using mpp::PIPE_READ;
    using mpp::PIPE_WRITE;

    struct redirect_info {
        fd_type _target = FD_INVALID;

        bool redirected() const {
            return _target != FD_INVALID;
        }
    };

    struct process_startup {
        std::vector<std::string> _cmdline;
        std::unordered_map<std::string, std::string> _env;
        std::string _cwd = ".";
        redirect_info _stdin;
        redirect_info _stdout;
        redirect_info _stderr;
        bool merge_outputs = false;
    };

    struct process_info {
        /**
         * Unused on *nix systems.
         */
        fd_type _tid = FD_INVALID;

        fd_type _pid = FD_INVALID;
        fd_type _stdin = FD_INVALID;
        fd_type _stdout = FD_INVALID;
        fd_type _stderr = FD_INVALID;
    };

    void create_process_impl(const process_startup &startup,
                             process_info &info,
                             fd_type *pstdin, fd_type *pstdout, fd_type *pstderr);

    bool redirect_or_pipe(const redirect_info &r, fd_type fds[2]);

    void create_process(const process_startup &startup, process_info &info);

    void close_process(process_info &info);

    int wait_for(const process_info &info);

    void terminate_process(const process_info &info, bool force);

    bool process_exited(const process_info &info);
}

namespace mpp {
    using mpp_impl::redirect_info;
    using mpp_impl::process_info;
    using mpp_impl::process_startup;
    using mpp_impl::fd_type;

    class process {
        friend class process_builder;

    private:
        struct member_holder {
            process_info _info;
            fdostream _stdin;
            fdistream _stdout;
            fdistream _stderr;
            int _exit_code = -1;

            explicit member_holder(const process_info &info)
                : _info(info), _stdin(_info._stdin),
                  _stdout(_info._stdout), _stderr(_info._stdout) {}

            ~member_holder() {
                mpp_impl::close_process(_info);
            }
        };

        std::unique_ptr<member_holder> _this;

        explicit process(const process_info &info)
            : _this(std::make_unique<member_holder>(info)) {}

    public:
        process() = delete;

        process(const process &) = delete;

        process(process &&) = default;

        process &operator=(process &&) = delete;

        process &operator=(const process &) = delete;

    public:
        ~process() = default;

        std::ostream &in() {
            return _this->_stdin;
        }

        std::istream &out() {
            return _this->_stdout;
        }

        std::istream &err() {
            return _this->_stderr;
        }

        int wait_for() {
            if (has_exited() && _this->_exit_code >= 0) {
                return _this->_exit_code;
            }
            _this->_exit_code = mpp_impl::wait_for(_this->_info);
            return _this->_exit_code;
        }

        bool has_exited() const {
            return mpp_impl::process_exited(_this->_info);
        }

        void interrupt(bool force = false) {
            mpp_impl::terminate_process(_this->_info, force);
        }

    public:
        static process exec(const std::string &command);

        static process exec(const std::string &command,
                            const std::vector<std::string> &args);
    };

    class process_builder {
    private:
        process_startup _startup;

    public:
        process_builder() = default;

        ~process_builder() = default;

        process_builder(process_builder &&) = default;

        process_builder(const process_builder &) = default;

        process_builder &operator=(process_builder &&) = default;

        process_builder &operator=(const process_builder &) = default;

    public:
        process_builder &command(const std::string &command) {
            if (_startup._cmdline.empty()) {
                _startup._cmdline.push_back(command);
            } else {
                _startup._cmdline[0].assign(command);
            }
            return *this;
        }

        template <typename Container>
        process_builder &arguments(const Container &c) {
            if (_startup._cmdline.size() <= 1) {
                std::copy(c.begin(), c.end(), std::back_inserter(_startup._cmdline));
            } else {
                // invalid operation, do nothing
            }
            return *this;
        }

        process_builder &environment(const std::string &key, const std::string &value) {
            _startup._env.emplace(key, value);
            return *this;
        }

        process_builder &redirect_stdin(fd_type target) {
            _startup._stdin._target = target;
            return *this;
        }

        process_builder &redirect_stdout(fd_type target) {
            _startup._stdout._target = target;
            return *this;
        }

        process_builder &redirect_stderr(fd_type target) {
            _startup._stderr._target = target;
            return *this;
        }

#ifdef MOZART_PLATFORM_WIN32

        process_builder &redirect_stdin(int cfd) {
            return redirect_stdin(reinterpret_cast<fd_type>(_get_osfhandle(cfd)));
        }

        process_builder &redirect_stdout(int cfd) {
            return redirect_stdout(reinterpret_cast<fd_type>(_get_osfhandle(cfd)));
        }

        process_builder &redirect_stderr(int cfd) {
            return redirect_stderr(reinterpret_cast<fd_type>(_get_osfhandle(cfd)));
        }

#endif

        process_builder &directory(const std::string &cwd) {
            _startup._cwd = cwd;
            return *this;
        }

        process_builder &merge_outputs(bool r) {
            _startup.merge_outputs = r;
            return *this;
        }

        process start() {
            process_info info{};
            mpp_impl::create_process(_startup, info);
            return process(info);
        }
    };
}
