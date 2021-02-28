/**
 * Mozart++ Template Library
 * Licensed under Apache 2.0
 * Copyright (C) 2020-2021 Chengdu Covariant Technologies Co., LTD.
 * Website: https://covariant.cn/
 * Github:  https://github.com/chengdu-zhirui/
 */

#include <mozart++/format>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <deque>
#include <list>
#include <tuple>

struct obj {
    int a = 0;
    int b = 0;
};

struct objx {
    int a = 0;
    int b = 0;

    objx(int a, int b) : a(a), b(b) {}
};

std::ostream &operator<<(std::ostream &out, const objx &x) {
    mpp::format(out, "objxOut{a: {}, b: {}}", x.a, x.b);
    return out;
}

template <>
std::string mpp::to_string(const objx &val) {
    return mpp::format("objxToString{a: {}, b: {}}", val.a, val.b);
}

class only_string_writable {
public:

};

only_string_writable &operator<<(only_string_writable &out, const std::string &s) {
    std::cout << s;
    return out;
}

class nothing_writable {
};

int main() {
    auto s = mpp::format("hello {} {} {}", 0.1 + 0.2);
    printf("%s\n", s.c_str());

    mpp::format(std::cout, "this is {} formatter\n", "mpp");

    const char ss[3] = "it";
    mpp::format(std::cout, "love {}\n", ss);

    const char *f = "lover, fucker";
    mpp::format(std::cout, "hop, {}\n", f);

    mpp::format(std::cout, "object is {}\n", obj{});
    mpp::format(std::cout, "objectX is {}\n", objx{10, 20});

    only_string_writable op;
    mpp::format(op, "test must be string {}\n", 'a');

    nothing_writable nop;
    mpp::format(nop, "this line will never be formatted", 'f');

    mpp::format(std::cout, "an int inside a bracket {{}}\n", 100);

    int matrix[3][4] = {
        {1, 2, 3, 4},
        {5, 6, 7, 8},
        {9, 0, 1, 2},
    };

    const int matX[2][3][4] = {
        {
            {1, 2, 3,  4},
            {5, 6, 7, 8},
            {9, 0, 1, 2},
        },
        {
            {4, 5, 11, 4},
            {5, 1, 4, 1},
            {5, 4, 1, 1},
        }
    };

    mpp::format(std::cout, "Matrix = {}\n", matrix);
    mpp::format(std::cout, "MatrixX = {}\n", matX);
    mpp::format(std::cout, "pair of ints = {}\n", std::make_pair(1, 2));
    mpp::format(std::cout, "vector of ints = {}\n", std::vector<int>{1, 1, 4, 5, 1, 4});
    mpp::format(std::cout, "deque of ints = {}\n", std::deque<int>{1, 1, 4, 5, 1, 4});
    mpp::format(std::cout, "list of ints = {}\n", std::list<int>{1, 1, 4, 5, 1, 4});
    mpp::format(std::cout, "empty tuple = {}\n", std::make_tuple());
    mpp::format(std::cout, "tuple = {}\n", std::make_tuple(1, 2, 3));
    mpp::format(std::cout, "tuple = {}\n", std::make_tuple(5, "love", 0x0));
    mpp::format(std::cout, "maps = {}\n",
        std::unordered_map<int, int>{{1, 2}, {3, 4}, {5, 6}}
    );

    mpp::format(std::cout, "dec 15 in hex is {x}\n", 15);
    mpp::format(std::cout, "decf 15.0 in hexf is {x}\n", 15.0);
    mpp::format(std::cout, "decf 15.0 in sci is {e}\n", 15.0);
    mpp::format(std::cout, "decf 15.0 in sci is {.2e}\n", 15.0);

    mpp::format(std::cout, "left aligned 10: [{:-10}]\n", 10);
    mpp::format(std::cout, "right aligned 10: [{:10}]\n", 10);

    mpp::format(std::cout, "left aligned 10, fill with =: [{:-10|=}]\n", 10);
    mpp::format(std::cout, "right aligned 10, fill with 6: [{:10|6}]\n", 10);

    mpp::format(std::cout,
        "in float format, up to 2 floating points, right aligned to 4 {.2:4}\n",
        3.14);

    mpp::format(std::cout,
        "in hex format, up to 2 floating points, right aligned to 4 {.2x:4}\n",
        3.14);
    return 0;
}
