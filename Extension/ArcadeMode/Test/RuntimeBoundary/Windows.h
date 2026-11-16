#pragma once
// This test target alone substitutes OS clocks. The runtime translation unit
// and its real logging formats remain included unchanged in the fixture.
#include <cstdint>
using DWORD=std::uint32_t;
std::uint64_t GetTickCount64();
DWORD GetLastError();
void SetLastError(DWORD);
