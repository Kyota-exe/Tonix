#ifndef MISKOS_IO_H
#define MISKOS_IO_H

#include <stdint.h>
#include <stddef.h>

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t* values, size_t size);
void io_wait();

#endif
