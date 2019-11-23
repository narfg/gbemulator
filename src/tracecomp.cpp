#include <iostream>
#include <fstream>

// Small tool to compare debug trace files of the binjgb emulator
// (https://github.com/binji/binjgb) and this emulator.
int main() {
    std::ifstream if_test;
    std::ifstream if_ref;

    if_test.open("/tmp/trace_emulator.txt");
    if_ref.open("/tmp/trace_reference.txt");

    std::string test;
    std::string ref;

    // Get test header
    std::getline(if_test, test);

    // Get ref header
    std::getline(if_ref, ref);
    std::getline(if_ref, ref);
    std::getline(if_ref, ref);
    std::getline(if_ref, ref);
    std::getline(if_ref, ref);
    std::getline(if_ref, ref);
    std::getline(if_ref, ref);

    const size_t compare_cnt = 43;

    size_t line_cnt = 0;
    while (true) {
        std::getline(if_test, test);
        std::getline(if_ref, ref);

        if (test.substr(0, compare_cnt) !=  ref.substr(0, compare_cnt)) {
            std::cout << line_cnt << " " << test << std::endl;
            std::cout << line_cnt << " " << ref << std::endl;
        }
        line_cnt++;
    }
    return 0;
}
