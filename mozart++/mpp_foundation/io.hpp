/**
 * Mozart++ Template Library: System/IO
 * Licensed under Apache 2.0
 * Copyright (C) 2020-2021 Chengdu Covariant Technologies Co., LTD.
 * Website: https://covariant.cn/
 * Github:  https://github.com/chengdu-zhirui/
 */
#pragma once

#include <mozart++/core>
#include <cstring>
#include <cstdint>
#include <cstdio>

#ifdef MOZART_PLATFORM_WIN32

#include <Windows.h>
#include <io.h>

#else

#include <unistd.h>

#endif

#ifdef _MSC_VER

#include <BaseTsd.h>

#endif

namespace mpp {
#ifdef _MSC_VER
    // On MSVC, ssize_t is SSIZE_T
    using ssize_t = SSIZE_T;
#else
    using ssize_t = ::ssize_t;
#endif

#ifdef MOZART_PLATFORM_WIN32
    using fd_type = HANDLE;
    static constexpr fd_type FD_INVALID = nullptr;

    static fd_type fileno(FILE* fp)
    {
        return reinterpret_cast<fd_type>(_get_osfhandle(_fileno(fp)));
    }

    static mpp::ssize_t read(fd_type handle, void *buf, size_t count) {
        DWORD dwRead;
        if (ReadFile(handle, buf, count, &dwRead, nullptr)) {
            return dwRead;
        } else {
            return 0;
        }
    }

    static mpp::ssize_t write(fd_type handle, const void *buf, size_t count) {
        DWORD dwWritten;
        if (WriteFile(handle, buf, count, &dwWritten, nullptr)) {
            return dwWritten;
        } else {
            return 0;
        }
    }

#else
    using fd_type = int;
    static constexpr fd_type FD_INVALID = -1;
    using ::fileno;
    using ::read;
    using ::write;
#endif

    static void close_fd(fd_type &fd) {
        if (fd == FD_INVALID) {
            return;
        }
#ifdef MOZART_PLATFORM_WIN32
        CloseHandle(fd);
#else
        ::close(fd);
#endif
        fd = FD_INVALID;
    }

    static constexpr int PIPE_READ = 0;
    static constexpr int PIPE_WRITE = 1;

    static bool create_pipe(fd_type fds[2]) {
#ifdef MOZART_PLATFORM_WIN32
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = true;
        sa.lpSecurityDescriptor = nullptr;
        return CreatePipe(&fds[PIPE_READ], &fds[PIPE_WRITE], &sa, 0);
#else
        return ::pipe(fds) == 0;
#endif
    }

    static void close_pipe(fd_type fds[2]) {
        close_fd(fds[PIPE_READ]);
        close_fd(fds[PIPE_WRITE]);
    }
}
