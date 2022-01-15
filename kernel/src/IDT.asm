[BITS 64]
section .text

extern ISRHandler

KERNEL_DS equ 0b110000

%macro ISRWrapperContents 0
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

    ; Push DS/ES
    mov rax, ds
    push rax
    mov rax, es
    push rax

    mov rdi, rsp

    ; Switch to kernel DS
    mov ax, KERNEL_DS
    mov ds, ax
    mov es, ax

    ; Call C code
    cld
    call ISRHandler

    ; Pop DS/ES (switch back to original DS/ES)
    pop rax
    mov es, rax
    pop rax
    mov ds, rax

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

    ; Add 16 to RSP to account for error code and interrupt number (8 bytes each)
    add rsp, 16

    iretq
%endmacro

%macro ISRWrapperErrorCode 1
    global ISRWrapper%1
ISRWrapper%1:
    push %1
    ISRWrapperContents
%endmacro

%macro ISRWrapperNoErrorCode 1
    global ISRWrapper%1
ISRWrapper%1:
    push 0
    push %1
    ISRWrapperContents
%endmacro

; Exceptions
ISRWrapperNoErrorCode 0
ISRWrapperNoErrorCode 1
ISRWrapperNoErrorCode 2
ISRWrapperNoErrorCode 3
ISRWrapperNoErrorCode 4
ISRWrapperNoErrorCode 5
ISRWrapperNoErrorCode 6
ISRWrapperNoErrorCode 7
ISRWrapperErrorCode 8
ISRWrapperNoErrorCode 9
ISRWrapperErrorCode 10
ISRWrapperErrorCode 11
ISRWrapperErrorCode 12
ISRWrapperErrorCode 13
ISRWrapperErrorCode 14
ISRWrapperNoErrorCode 16
ISRWrapperErrorCode 17
ISRWrapperNoErrorCode 18
ISRWrapperNoErrorCode 19
ISRWrapperNoErrorCode 20
ISRWrapperErrorCode 21
ISRWrapperNoErrorCode 28
ISRWrapperErrorCode 29
ISRWrapperErrorCode 30

; PIC IRQs
ISRWrapperNoErrorCode 32
ISRWrapperNoErrorCode 33
ISRWrapperNoErrorCode 34
ISRWrapperNoErrorCode 35
ISRWrapperNoErrorCode 36
ISRWrapperNoErrorCode 37
ISRWrapperNoErrorCode 38
ISRWrapperNoErrorCode 39
ISRWrapperNoErrorCode 40
ISRWrapperNoErrorCode 41
ISRWrapperNoErrorCode 42
ISRWrapperNoErrorCode 43
ISRWrapperNoErrorCode 44
ISRWrapperNoErrorCode 45
ISRWrapperNoErrorCode 46
ISRWrapperNoErrorCode 47

; Local APIC IRQs
ISRWrapperNoErrorCode 48  ; Timer
ISRWrapperNoErrorCode 255 ; Spurious Interrupt Vector