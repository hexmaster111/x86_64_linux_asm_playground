format ELF64 executable 3

segment readable executable

entry $
    ; int3

    ; /usr/include/x86_64/bits/socket.h
    ; #define PF_INET		2

    ; /usr/include/x86_64/bits/socket_type.h
    ; SOCK_STREAM = 1,		

    ;socket(AF_INET, SOCK_STREAM, 0);
    mov rax, 41 ; SOCKET
    mov rdi, 2  ; PF_INET
    mov rsi, 1  ; SOCK_STREAM
    mov rdx, 0  ; protocall
    syscall

    ; rax has the resault / our file descriptor
    cmp rax, 0         ; if 0 > socket()
    jl exit_failure        ; exit
    mov [sfd], rax     ; sfd = rax
    

    ; /usr/include/x86_64-linux-gnu/bits/socket-constants.h
    ; #define SOL_SOCKET 1
    ; #define SO_REUSEADDR 2

    ;setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))
    mov rax, 54     ; setsockopt
    mov rdi, [sfd]  ; server fd
    mov rsi, 1      ; SOL_SOCKET
    mov rdx, 2      ; SO_REUSEADDR
    mov r10, one    ; &(int){1}
    mov r8, one_len ; sizeof(int) 
    syscall 

    cmp rax, 0         ; if 0 > setsockopt()
    jl exit_failure        ; exit
    
    ; /usr/include/asm-generic/socket.h
    ; #define SO_REUSEPORT 15

    ;setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int))
    mov rax, 54     ; setsockopt
    mov rdi, [sfd]  ; server fd
    mov rsi, 1      ; SOL_SOCKET
    mov rdx, 15     ; SO_REUSEADDR
    mov r10, one    ; &(int){1}
    mov r8, one_len ; sizeof(int) 
    syscall 

    cmp rax, 0         ; if 0 > setsockopt()
    jl exit_failure        ; exit

    mov word  [saddr]     , 2       ; saddr.sin_family = AF_INET
    mov word  [saddr+2]   , 37151   ; saddr.sin_port = htons(8081)
    mov dword [saddr+2+2] , 0       ; saddr.sin_addr.s_addr = htonl (any) 

    ; bind(sfd, &saddr, sizeof(saddr))
    mov rax, 49         ; BIND
    mov rdi, [sfd]      ; sfd
    mov rsi, saddr      ; &saddr
    mov rdx, saddr_len  ; sizeof(saddr)
    syscall

    cmp rax, 0         ; if 0 > bind()
    jl exit_failure        ; exit

    ; listen ( sfd, 1 )
    mov rax, 50     ; listen
    mov rdi, [sfd]  ; server file descripter
    mov rsi, 1      ; max connection queue
    syscall

    cmp rax, 0         ; if 0 > listen()
    jl exit_failure        ; exit

.accept_new_clients:
    ; accept ( sfd, &caddr, &caddrlen )
    mov rax, 43         ; accept
    mov rdi, [sfd]      ; server file destor
    mov rsi, cfd        ; & client address
    mov rdx, clientlen  ; & client addr len
    syscall

    ; rax will have client fd or 0>rax on error
    cmp rax, 0         ; if 0 > accept()
    jl exit_failure        ; exit

    mov [cfd], rax   ; cfd = rax ; save client file descriptor

.echo_loop:
    
    ; read(cfd, buffer, buffer_len)
    mov rax, 0 ; read
    mov rdi, [cfd] ; cfd
    mov rsi, buffer ; & readbuffer
    mov rdx, buffer_len ; sizeof(readbuffer)
    syscall

    ; rax is bytes got, or 0>rax on error
    cmp rax, 0         ; if 0 > read()
    jl exit_failure        ; exit -- TODO, jl done

    mov rdx, rax ; save bytes in into rdx  
    
    ; write (cfd, buffer, bytes_in)
    mov rax, 1 ; write
    mov rdi, [cfd]
    mov rsi, buffer
    ; mov rdx, rax -- saved above
    syscall

    ; rax is bytes out, or 0>rax on error
    cmp rax, 0         ; if 0 > write()
    jl exit_failure        ; exit -- TODO, jl done

    ; todo -- we may need to call write more then once... this
    ; example dose not check the write call to write more bytes

    ; if (buffer[0] == 'q') goto done else goto echo_loop
    cmp BYTE [buffer], 'q'
    je  .done
    jmp .echo_loop 

.done:
    ; close (cfd)
    mov rax, 3 ; close
    mov rdi, [cfd]
    syscall
    
    cmp rax, 0         ; if 0 > close()
    jl exit_failure        ; exit

    ; close (sfd)
    mov rax, 3 ; close
    mov rdi, [sfd]
    syscall
    
    cmp rax, 0         ; if 0 > close()
    jl exit_failure        ; exit
    jmp exit_ok

exit_ok:
    mov rdi, 0
    mov rax, 60
    syscall


exit_failure:
    mov rdi, 1
    mov rax, 60
    syscall


segment readable
socketmsg:db "socket"
one:dd 1
one_len = $-one

segment readable writable
cfd:dd 0  ;client file discripter
sfd:dd 0  ;server file discripter
clientlen:dd 0 ;clent socketaddr len
serverlen:dd 0 ;server socketaddr len

; struct sockaddr_in (16 bytes)
saddr: db 16 dup (0)
saddr_len = $-saddr

buffer: db 1024 dup (0)
buffer_len = $-buffer