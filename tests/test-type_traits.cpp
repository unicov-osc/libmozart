/**
 * Mozart++ Template Library
 * Licensed under Apache 2.0
 * Copyright (C) 2020-2021 Chengdu Covariant Technologies Co., LTD.
 * Website: https://covariant.cn/
 * Github:  https://github.com/chengdu-zhirui/
 */

#include <mozart++/core>
#include <cstdio>

#include <vector>
#include <list>
#include <deque>
#include <unordered_map>

int main() {
    using namespace mpp;
    static_assert(is_iterable_v<std::vector<int>>, "You wrote a bug");
    static_assert(is_iterable_v<std::vector<std::vector<int>>>, "You wrote a bug");
    static_assert(!is_iterable_v<std::string>, "You wrote a bug");
    static_assert(!is_iterable_v<int>, "You wrote a bug");
    static_assert(!is_iterable_v<mpp::function<void()>>, "You wrote a bug");
    static_assert(!is_iterable_v<mpp::function<const char *()>>, "You wrote a bug");
    static_assert(!is_iterable_v<mpp::function<const char *()>>, "You wrote a bug");
}
