KERNEL := bin/kernel.elf
CC := x86_64-elf-g++
LD := x86_64-elf-ld

CFLAGS ?= -Wall -Wextra -O2 -pipe
LDFLAGS ?=
ASMFLAGS ?= -f elf64 -gdwarf

INTERNALCFLAGS :=        \
	-Iinclude/           \
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
OBJ += $(patsubst src/%, obj/%, $(ASMSRC:.asm=.asm.o))
OBJSUBDIRS := $(sort $(shell dirname $(OBJ)))

.PHONY: all
all: $(KERNEL)

$(KERNEL): $(OBJSUBDIRS) $(OBJ)
	$(LD) $(OBJ) $(LDFLAGS) $(INTERNALLDFLAGS) -o $@

$(OBJSUBDIRS):
	mkdir -p $@

obj/%.o: src/%.cpp
	$(CC) $(CFLAGS) $(INTERNALCFLAGS) -g -c $< -o $@

obj/%.asm.o: src/%.asm
	nasm $(ASMFLAGS) -g $< -o $@

.PHONY: clean
clean:
	rm -r obj/* || true
	rm -rf $(KERNEL) $(OBJ)