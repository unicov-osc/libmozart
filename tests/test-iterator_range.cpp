/**
 * Mozart++ Template Library
 * Licensed under Apache 2.0
 * Copyright (C) 2020-2021 Chengdu Covariant Technologies Co., LTD.
 * Website: https://covariant.cn/
 * Github:  https://github.com/chengdu-zhirui/
 */

#include <cstdio>
#include <mozart++/iterator_range>
#include <vector>

int main() {
    std::vector<int> a{1, 2, 3, 34, 5, 6, 7, 8, 1, 231, 2, 1, 31};
    for (auto i : mpp::make_range(a)) {
        printf("%d\n", i);
    }
}
