#pragma once
// Native memory is an external boundary in the runtime lifecycle fixture.
#include <cstddef>
#include <cstdint>
namespace dingosdk::memory {
bool read_bytes(std::uintptr_t,void*,std::size_t) noexcept;
template<class T> bool read(std::uintptr_t at,T& value) noexcept{return read_bytes(at,&value,sizeof value);}
template<class T> bool peek(std::uintptr_t at,T& value) noexcept{return read_bytes(at,&value,sizeof value);}
}
