CC      ?= cc
CFLAGS  ?= -O2 -std=c11 -Wall -Wextra -Wpedantic

all: bytecode-vm

bytecode-vm: src/main.c src/vm.c src/vm.h
	$(CC) $(CFLAGS) -o $@ src/main.c src/vm.c -lm

vm_tests: tests/vm_tests.c src/vm.c src/vm.h
	$(CC) $(CFLAGS) -o vm_tests tests/vm_tests.c src/vm.c -lm

test: bytecode-vm vm_tests
	./vm_tests
	printf 'PUSH 2\nPUSH 3\nADD\nPRINT\nHALT\n' > /tmp/bc-asm.vm && \
	./bytecode-vm run /tmp/bc-asm.vm | grep -q '^5'

clean:
	rm -f bytecode-vm vm_tests

.PHONY: all test clean
