section .text
global _start

_start:
	push 0x21
	mov rdi, 0x69
	int 0x80
