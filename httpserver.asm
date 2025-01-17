format ELF64 executable 3

segment readable executable

entry $
    ; /usr/include/x86_64/bits/socket.h
    ; #define PF_INET		2

    ; /usr/include/x86_64/bits/socket_type.h
    ; SOCK_STREAM = 1,		

setup_socket:
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

    ; int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
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
    mov rsi, 5      ; max connection queue
    syscall

    cmp rax, 0         ; if 0 > listen()
    jl exit_failure        ; exit

accept_new_client:

    ; int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
    mov rax, 43         ; accept
    mov rdi, [sfd]      ; server file destor
    mov rsi, caddr      ; & client address
    mov rdx, clientlen  ; & client addr len
    syscall

    ; rax will have client fd or 0>rax on error
    cmp rax, 0         ; if 0 > accept()
    jl exit_failure        ; exit

    mov [cfd], rax   ; cfd = rax ; save client file descriptor
    
    ;todo, fill buffer with request from client
    jmp route;

exit_ok:
    mov rax, 60
    mov rdi, 0
    syscall

exit_failure: 
    mov rax, 60
    mov rdi, 1
    syscall



route:
    ; finds the METHOD (GET/POST)
    ; reads the ROUTE (/index.html)
    ; provies the file or error page
    
    mov r10, [cfd]
    mov r11, html_ok_text
    mov r12, htmllen

    call WriteData

; after routing, we close the connection and get ready for the next request
.cleanup:
    ; int3

    ; close (cfd)
    mov rax, 3 ; close
    mov rdi, [cfd]
    syscall
  
    cmp rax, 0         ; if 0 > close()
    jl exit_failure        ; exit

    jmp accept_new_client ; go and start accepting another client
    
; R10 - FD
; R11 - Ptr
; R12 - Len
WriteData:
    ; write
    mov rax, 1
    mov rdi, r10
    mov rsi, r11
    mov rdx, r12
    syscall

    ; TODO
    ; if len > rax && 0 > rax
    ;   we need to subtract rax from len, and re-call write()

    ; if 0 > rax close()
    cmp rax, 0         ; if 0 > close()
    jl exit_failure        ; exit
    ret

segment readable writeable
; space for our global vars
cfd:dq 0  ;client file discripter
sfd:dq 0  ;server file discripter

clientlen:dq 0 ;clent socketaddr len
serverlen:dq 0 ;server socketaddr len

buffer: rb 1024
buffer_len = $-buffer

; struct sockaddr_in (16 bytes)
saddr: db 16 dup (0)
saddr_len = $-saddr

caddr: dd 16 dup (0)

segment readable
one:dd 1
one_len = $-one

; some hello world HTML
html_ok_text : file "http_200_ok.txt"
htmltext: file "index.html"
htmllen = $-htmltext + $-html_ok_text 