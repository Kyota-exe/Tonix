section .text
global _start

_start:
	mov [message], dil
	mov rdi, message

print:
	int 0x80
	jmp print

section .data
message db "A", 0
