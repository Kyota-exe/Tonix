section .text
global _start

_start:
	mov rbx, rsi       ; Move second parameter (Serial::Print addr) into rax
	mov [message], dil ; Move lower 8 bits of rdi into first byte of message
call_print:
	mov rdi, message   ; Move first parameter (character) into first parameter slot
	mov rsi, end       ; Move ending string into second parameter slot
	call rbx
	jmp call_print

section .data
message db 0, 0
end db 0
