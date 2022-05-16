#pragma once

#include <stdint.h>

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t* values, uint64_t size);
