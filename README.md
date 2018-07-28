# Theatre booking system

A  server written in C, that can handle booking requests from multiple clients at the same time and allocates seats to them.
Two server implementations are made, one using the fork system call (`projectOS_process`) and another one using POSIX threads to answer to clients (`projectOS_threads`).
