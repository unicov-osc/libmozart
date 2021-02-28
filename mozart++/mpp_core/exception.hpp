/**
 * Mozart++ Template Library: Core Library/Exception
 * Licensed under Apache 2.0
 * Copyright (C) 2020-2021 Chengdu Covariant Technologies Co., LTD.
 * Website: https://covariant.cn/
 * Github:  https://github.com/chengdu-zhirui/
 */

#pragma once

#include "base.hpp"

#include <exception>
#include <string>

namespace mpp {
    /**
     * This class represents unexpected runtime exceptions.
     * User-defined exceptions can derive from this base class.
     */
    class runtime_error : public std::exception {
        std::string mWhat = "Runtime Error";

    public:
        runtime_error() = default;

        explicit runtime_error(const std::string &str) noexcept
                : mWhat("Runtime Error: " + str) {}

        runtime_error(const runtime_error &) = default;

        runtime_error(runtime_error &&) noexcept = default;

        ~runtime_error() override = default;

        runtime_error &operator=(const runtime_error &) = default;

        runtime_error &operator=(runtime_error &&) = default;

        const char *what() const noexcept override {
            return this->mWhat.c_str();
        }
    };

    template <typename, typename... ArgsT>
    MOZART_NORETURN void throw_ex(ArgsT &&...);
}
