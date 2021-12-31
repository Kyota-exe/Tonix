[BITS 64]
section .text

extern ISRHandler
global ISRWrapperNoErrorCode
global ISRWrapperErrorCode

KERNEL_DS equ 0b110000;

%macro ISRWrapper 0
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    ;---------------------------
    mov rax, ds
    push rax
    mov rax, es
    push rax
    mov rdi, rsp
    mov ax, KERNEL_DS
    mov ds, ax
    mov es, ax
    cld
    call ISRHandler
    pop rax
    mov es, rax
    pop rax
    mov ds, rax
    ;---------------------------
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax
    add rsp, 8
    iretq
%endmacro

ISRWrapperNoErrorCode:
    push 0 ; Push 0 as dummy value for when there is no error code
    ISRWrapper

ISRWrapperErrorCode:
    ISRWrapper