format ELF64 executable

segment readable executable

entry $
    mov rax, 1
    mov rdi, 1
    mov rsi, hellomsg
    mov rdx, hellolen
    syscall

    mov rdi, 0
    mov rax, 60
    syscall


segment readable writable 

hellomsg: db "Hello World!", 0
hellolen = $-hellomsg