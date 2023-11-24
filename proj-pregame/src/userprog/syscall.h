#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init(void);
int syscall_write(int fd, const void *buffer, unsigned size);
int practice(int i);
#endif /* userprog/syscall.h */
