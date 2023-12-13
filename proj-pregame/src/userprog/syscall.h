#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

typedef int pid_t;

void syscall_init(void);
int write(int fd, const void *buffer, unsigned size);
int practice(int i);
void halt(void);
pid_t exec(const char* cmd_line);
int wait(pid_t pid);
bool create(const char* file, unsigned file_size);
bool remove(const char* file);
int open(const char* file);
int filesize(int fd);
int read (int fd, void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell(int fd);
void close (int fd);
double compute_e (int n);
#endif /* userprog/syscall.h */
