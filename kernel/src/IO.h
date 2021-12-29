#ifndef MISKOS_IO_H
#define MISKOS_IO_H

#include <stdint.h>
#include <stddef.h>

inline void outb(uint16_t port, uint8_t value);
void outb(uint16_t port, uint8_t* values, size_t size);

#endif
