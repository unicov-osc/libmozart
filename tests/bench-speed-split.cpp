/**
 * Mozart++ Template Library
 * Licensed under Apache 2.0
 * Copyright (C) 2020-2021 Chengdu Covariant Technologies Co., LTD.
 * Website: https://covariant.cn/
 * Github:  https://github.com/chengdu-zhirui/
 */

#include <mozart++/string>
#include <chrono>

void split_path_c(const char *path) {
    // it's safe to convert from size_t to int, :)
    int count = static_cast<int>(mpp::string_ref(path).count(':')) + 1;
    size_t pathvsize = sizeof(const char *) * (count + 1);
    size_t pathsize = strlen(path) + 1;
    const char **pathv = reinterpret_cast<const char **>(malloc(pathvsize + pathsize));

    if (pathv == nullptr) {
        return;
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
    free(pathv);
}

void split_path_mpp(const char *path) {
    std::vector<mpp::string_ref> s;
    mpp::string_ref(path).split(s, ':');
}

void split_path_std(const char *path) {
    size_t start = 0;
    size_t end;
    std::string s(path);
    std::vector<std::string> results;

    while ((end = s.find(':', start)) != std::string::npos) {
        results.push_back(s.substr(start, end - start));
        start = end + 1;
    }

    results.push_back(s.substr(start));
}

void benchmark(const char *tag, const char *path, void(*fp)(const char *)) {
    constexpr static int TIMES = 10000000;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < TIMES; ++i) {
        fp(path);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto used = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    printf("[%s] %zd(ms)\n", tag, used);
}

int main() {
    const char *path = getenv("PATH");
    benchmark("c   ", path, split_path_c);
    benchmark("mpp ", path, split_path_mpp);
    benchmark("std ", path, split_path_std);
}

