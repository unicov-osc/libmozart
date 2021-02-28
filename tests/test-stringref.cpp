/**
 * Mozart++ Template Library
 * Licensed under Apache 2.0
 * Copyright (C) 2020-2021 Chengdu Covariant Technologies Co., LTD.
 * Website: https://covariant.cn/
 * Github:  https://github.com/chengdu-zhirui/
 */

#include <mozart++/string>
#include <iostream>

using mpp::string_ref;

int sum(string_ref val) {
    return val.stream()
            .reduce<int>(0, [](int acc, char c) {
                return std::isdigit(c) ? (acc + c - '0') : acc;
            });
}

void process_command(string_ref command) {
    if (command.startswith_ignore_case("run")) {
        if (command.contains("rm")) {
            printf("Cannot run dangerous command: %s\n",
                   command.substr(4).str().c_str());
        } else {
            printf("process: exec: %s\n",
                   command.substr(4).str().c_str());
        }
    } else if (command.contains_ignore_case("I love you")) {
        printf("I love you too!\n");
    } else if (command.startswith_ignore_case("sum")) {
        printf("=> %d\n", sum(command.substr(4)));
    }
}

int main() {
    process_command("I love you");
    process_command("run rm -rf");
    process_command("sum 12345");
    process_command("run f**k");
    return 0;
}
