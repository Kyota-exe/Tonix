KERNEL := bin/kernel.elf
CC := x86_64-elf-g++
LD := x86_64-elf-ld

CFLAGS ?= -Wall -Wextra -O2 -pipe
LDFLAGS ?=
ASMFLAGS ?= -f elf64 -gdwarf

INTERNALCFLAGS :=        \
	-I.                  \
	-ffreestanding       \
	-fno-stack-protector \
	-fno-pic             \
	-fno-rtti            \
	-fno-exceptions      \
	-mabi=sysv           \
	-mno-80387           \
	-mno-mmx             \
	-mno-3dnow           \
	-mno-sse             \
	-mno-sse2            \
	-mno-red-zone        \
	-mcmodel=kernel

INTERNALLDFLAGS :=         \
	-Tlinker.ld            \
	-nostdlib              \
	-zmax-page-size=0x1000 \
	-static

SRC := $(shell find src -type f -name '*.cpp')
ASMSRC := $(shell find src -type f -name '*.asm')
OBJ = $(patsubst src/%, obj/%, $(SRC:.cpp=.o))
OBJ += $(patsubst src/%, obj/%, $(ASMSRC:.asm=.o))
HEADER_DEPS := $(OBJ:.o=.d)

.PHONY: all
all: $(KERNEL)

src/stivale2.h:
	curl https://raw.githubusercontent.com/stivale/stivale/master/stivale2.h -o $@

$(KERNEL): $(OBJ)
	$(LD) $(OBJ) $(LDFLAGS) $(INTERNALLDFLAGS) -o $@

-include $(HEADER_DEPS)
obj/%.o: src/%.cpp src/stivale2.h
	$(CC) $(CFLAGS) $(INTERNALCFLAGS) -g -c $< -o $@

obj/%.o: src/%.asm
	nasm $(ASMFLAGS) -g $< -o $@

.PHONY: clean
clean:
	rm -rf $(KERNEL) $(OBJ) $(HEADER_DEPS)
	find ./obj/ -type f -delete

.PHONY: distclean
distclean: clean
	rm -f src/stivale2.h
