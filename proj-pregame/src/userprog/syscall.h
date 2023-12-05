#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

typedef int pid_t;

void syscall_init(void);
int syscall_write(int fd, const void *buffer, unsigned size);
int practice(int i);
void halt(void);
pid_t exec(const char* cmd_line);
int wait(pid_t pid);
#endif /* userprog/syscall.h */
