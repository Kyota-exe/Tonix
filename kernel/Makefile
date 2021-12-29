KERNEL := bin/kernel.elf
CC := x86_64-elf-gcc
LD := ld

CFLAGS ?= -Wall -Wextra -O2 -pipe

LDFLAGS ?=

INTERNALCFLAGS :=        \
	-I.                  \
	-ffreestanding       \
	-fno-stack-protector \
	-fno-pic             \
	-mabi=sysv           \
	-mno-80387           \
	-mno-mmx             \
	-mno-3dnow           \
	-mno-sse             \
	-mno-sse2            \
	-mno-red-zone        \
	-mcmodel=kernel      \
	-MMD

INTERNALLDFLAGS :=         \
	-Tlinker.ld            \
	-nostdlib              \
	-zmax-page-size=0x1000 \
	-static

SRC := $(shell find src -type f -name '*.cpp')
OBJ := $(patsubst src/%, obj/%, $(SRC:.cpp=.o))
HEADER_DEPS := $(OBJ:.o=.d)

.PHONY: all
all: $(KERNEL)

src/stivale2.h:
	curl https://raw.githubusercontent.com/stivale/stivale/master/stivale2.h -o $@

$(KERNEL): $(OBJ)
	$(LD) $(OBJ) $(LDFLAGS) $(INTERNALLDFLAGS) -o $@

-include $(HEADER_DEPS)
obj/%.o: src/%.cpp src/stivale2.h
	$(CC) $(CFLAGS) $(INTERNALCFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(KERNEL) $(OBJ) $(HEADER_DEPS)

.PHONY: distclean
distclean: clean
	rm -f src/stivale2.h
