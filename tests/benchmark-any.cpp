#include <mozart++/timer>
#include <mozart++/any>
#include <iostream>
#include <string>
#include <any>

int test_epoch = 10000000;

struct test_large_data {
    std::string str;
    unsigned char alignment[32];

    test_large_data(const char *s) : str(s) {}
};

int main() {
    std::cout << "Size of std::any        : " << sizeof(std::any) << std::endl;
    std::cout << "Size of mpp::any        : " << sizeof(mpp::any) << std::endl;

    std::cout << std::endl;

    std::cout << "[Small Data] std::any copying: " << mpp::timer::measure([]() {
        std::any a(10);
        for (int i = 0; i < test_epoch; ++i)
            std::any b(a);
    }) << std::endl;
    std::cout << "[Small Data] mpp::any copying: " << mpp::timer::measure([]() {
        mpp::any a(10);
        for (int i = 0; i < test_epoch; ++i)
            mpp::any b(a);
    }) << std::endl;

    std::cout << std::endl;

    std::cout << "[Small Data] std::any instancing: " << mpp::timer::measure([]() {
        std::any a(10);
        for (int i = 0; i < test_epoch; ++i) {
            ++std::any_cast<int &>(a);
            --std::any_cast<int &>(a);
        }
    }) << std::endl;
    std::cout << "[Small Data] mpp::any instancing: " << mpp::timer::measure([]() {
        mpp::any b(10);
        for (int i = 0; i < test_epoch; ++i) {
            ++b.get<int>();
            --b.get<int>();
        }
    }) << std::endl;

    std::cout << std::endl;

    std::cout << "[Large Data] std::any copying: " << mpp::timer::measure([]() {
        std::any a(test_large_data("Hello"));
        for (int i = 0; i < test_epoch; ++i)
            std::any b(a);
    }) << std::endl;
    std::cout << "[Large Data] mpp::any copying: " << mpp::timer::measure([]() {
        mpp::any a(test_large_data("Hello"));
        for (int i = 0; i < test_epoch; ++i)
            mpp::any b(a);
    }) << std::endl;

    std::cout << std::endl;

    std::cout << "[Large Data] std::any instancing: " << mpp::timer::measure([]() {
        std::any a(test_large_data("Hello"));
        for (int i = 0; i < test_epoch; ++i) {
            std::any_cast<test_large_data &>(a).str.push_back('c');
            std::any_cast<test_large_data &>(a).str.pop_back();
        }
    }) << std::endl;
    std::cout << "[Large Data] mpp::any instancing: " << mpp::timer::measure([]() {
        mpp::any b(test_large_data("Hello"));
        for (int i = 0; i < test_epoch; ++i) {
            b.get<test_large_data>().str.push_back('c');
            b.get<test_large_data>().str.pop_back();
        }
    }) << std::endl;

    return 0;
}
