#include "Extension/ArcadeMode/arcade_score_numbers.h"
#include <cstdlib>
#include <iostream>

namespace {
unsigned checks{};
void check(bool passed, const char* description) {
    ++checks;
    if (!passed) { std::cerr << "FAIL: " << description << '\n'; std::exit(1); }
}
}
int run_arcade_score_numbers_tests() {
    using namespace dingosdk::arcade_mode;
    const auto zero = parse_score_display_number("0");
    check(zero.valid && zero.value == 0, "displayed zero is valid and differs from unavailable text");
    const auto score = parse_score_display_number("1234567890");
    check(score.valid && score.value == 1234567890, "complete decimal score parses exactly");
    const auto maximum = parse_score_display_number("18446744073709551615");
    check(maximum.valid && maximum.value == UINT64_MAX, "maximum unsigned score remains exact");
    for (const auto text : {"", "18446744073709551616", "999999999999999999999", "-1", "+1", " 1", "1 ", "1\n", "1,000", "1.000", "1 000", "1.5", "1e3", "0x10", "123points", "x2", "X2", "2x"}) {
        const auto parsed = parse_score_display_number(text);
        check(!parsed.valid && parsed.value == 0, "unsupported or partial score formatting is unavailable");
    }
    for (const auto text : {"2", "x2", "X2"}) {
        const auto parsed = parse_score_display_number(text, true);
        check(parsed.valid && parsed.value == 2, "plain and upstream-supported prefix multiplier formats parse");
    }
    for (const auto text : {"0", "x0", "X0", "x", "X", "x-2", "x+2", "x 2", "xx2", "x2x", "x2 ", "x2.5", "x1,000", "x18446744073709551616"}) {
        const auto parsed = parse_score_display_number(text, true);
        check(!parsed.valid && parsed.value == 0, "invalid multiplier never defaults to a successful value");
    }
    const auto max_multiplier = parse_score_display_number("x18446744073709551615", true);
    check(max_multiplier.valid && max_multiplier.value == UINT64_MAX, "prefixed multiplier is overflow checked without truncation");
    check(!parse_score_display_number(std::string_view("12\0junk", 7)).valid,
          "embedded terminator cannot hide a suffix from full-consumption validation");
    std::cout << checks << " score number checks passed\n";
    return 0;
}
#ifndef ARCADE_SCORE_NUMBERS_COMBINED_TESTS
int main() { return run_arcade_score_numbers_tests(); }
#endif
