/**
 * Mozart++ Template Library
 * Licensed under Apache 2.0
 * Copyright (C) 2020-2021 Chengdu Covariant Technologies Co., LTD.
 * Website: https://covariant.cn/
 * Github:  https://github.com/chengdu-zhirui/
 */

#include <cstdio>
#include <cstdlib>
#include <mozart++/string>
#include <mozart++/process>

#ifdef MOZART_PLATFORM_WIN32
#define SHELL "C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe"
#else
#define SHELL "/bin/bash"
#endif

using mpp::process;
using mpp::process_builder;

void test_basic() {
    process p = process::exec(SHELL);
    p.in() << "ls /" << std::endl;
    p.in() << "exit" << std::endl;
    p.wait_for();

    std::string s;
    while (std::getline(p.out(), s)) {
        printf("process: test-basic: %s\n", s.c_str());
    }
}

void test_execvpe_unix() {
#ifndef MOZART_PLATFORM_WIN32
    process p = process::exec("ls");
    p.wait_for();

    std::string s;
    while (std::getline(p.out(), s)) {
        printf("process: test_execvpe_unix: %s\n", s.c_str());
    }
#endif
}

void test_error_unix() {
#ifndef MOZART_PLATFORM_WIN32
    try {
        process p = process::exec("no-such-command");
        p.wait_for();
    } catch (const mpp::runtime_error &e) {
        printf("%s\n", e.what());

        if (!mpp::string_ref(e.what()).contains("No such file or directory")) {
            printf("process: test_error_unix: failed\n");
            exit(1);
        }
    }
#endif
}

void test_stderr() {
    // PS> echo fuckms 1>&2
    // + CategoryInfo          : ParserError: (:) [], ParentContainsErrorRecordException
    // + FullyQualifiedErrorId : RedirectionNotSupported

#ifndef MOZART_PLATFORM_WIN32
    // the following code is equivalent to "/bin/bash 2>&1"
    process p = process_builder().command(SHELL)
        .merge_outputs(true)
        .start();

    // write to stderr
    p.in() << "echo fuckcpp 1>&2" << std::endl;
    p.in() << "exit" << std::endl;
    p.wait_for();

    // and we can get from stdout
    std::string s;
    p.out() >> s;

    if (s != "fuckcpp") {
        printf("process: test-stderr: failed\n");
        exit(1);
    }
#endif
}

void test_env() {
    // Thanks to the fucking powershit,
    // I can't refer to my variables till now.
    // God knows why MS designed an object-oriented shell,
    // and it just tastes like shit.
#ifndef MOZART_PLATFORM_WIN32
    process p = process_builder().command(SHELL)
        .environment("VAR1", "fuck")
        .environment("VAR2", "cpp")
        .start();

    p.in() << "echo $VAR1$VAR2" << std::endl;

    p.in() << "exit" << std::endl;
    p.wait_for();

    std::string s;
    p.out() >> s;

    if (s != "fuckcpp") {
        printf("process: test-env: failed\n");
        exit(1);
    }
#endif
}

void test_r_file() {
    // VAR=fuckcpp bash <<< "echo $VAR; exit" > output-all.txt

    FILE *fout = fopen("output-all.txt", "w");

    process p = process_builder().command(SHELL)
#ifndef MOZART_PLATFORM_WIN32
        .environment("VAR", "fuckcpp")
#endif
        .redirect_stdout(fileno(fout))
        .merge_outputs(true)
        .start();


    p.in() << "echo $VAR" << std::endl;
    p.in() << "exit" << std::endl;
    p.wait_for();

    fclose(fout);

    fout = fopen("output-all.txt", "r");
    mpp::fdistream fin(fileno(fout));
    std::string s;
    fin >> s;

#ifndef MOZART_PLATFORM_WIN32
    if (s != "fuckcpp") {
        printf("process: test-redirect-file: failed\n");
        exit(1);
    }
#else
    if (s != "Windows") {
        printf("process: test-redirect-file: failed\n");
        exit(1);
    }
#endif
}

void test_exit_code() {
    process p = process::exec(SHELL);

    p.in() << "exit 120" << std::endl;
    int code = p.wait_for();

    if (code != 120) {
        printf("process: test-exit-code: failed\n");
        exit(1);
    }
}

int main(int argc, const char **argv) {
    test_basic();
    test_execvpe_unix();
    test_error_unix();
    test_stderr();
    test_env();
    test_r_file();
    test_exit_code();
    return 0;
}
