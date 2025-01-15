# asm learning

This repo is full of me just playing around with linux x86_64 assembly, nothing in here should be consitered the "right way" todo things, i am just trying to see what works and what dose not

most programs are compiled and ran with `rm <program name>; fasm <source file name>.asm; chmod +x ./<program name>; strace ./<program name>` while writing them and debugging them
using `strace` is nice, as we can watch the syscalls, and watch the values that we are putting in and getting out

fasm docs are `/usr/share/doc/fasm/fasm.txt.gz` on my computer. You can use vim to open the compressed text files directly -- it can be nice to find them using find and grep, like `cd /usr; find | grep fasm.txt`

## echoserver

a simple tcp echo server that writes back what the server got

run with `./cechoserver`
connect with `telnet localhost 8081`
quit the connection with q in the telnet console

### echoserver.c

compile with `cc echoserver.c -o cechoserver`

### echoserver.asm

compile with `fasm echoserver.asm`