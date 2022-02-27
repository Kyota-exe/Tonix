#pragma once

#include <stdint.h>

constexpr uint16_t USER_CODE_SEGMENT = 0b01'0'11;
constexpr uint16_t USER_DATA_SEGMENT = 0b10'0'11;

constexpr uint16_t KERNEL_CODE_SEGMENT = 0b101'0'00;
constexpr uint16_t KERNEL_DATA_SEGMENT = 0b110'0'00;

constexpr uint64_t INITIAL_RFLAGS = 0b1000000010;