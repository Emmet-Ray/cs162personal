#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

typedef int pid_t;
typedef pid_t tid_t;

typedef void (*pthread_fun)(void*);
typedef void (*stub_fun)(pthread_fun, void*);

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
tid_t sys_pthread_create(stub_fun sfun, pthread_fun tfun, const void* arg); 
tid_t sys_pthread_join(tid_t tid);
void sys_pthread_exit(void);
#endif /* userprog/syscall.h */
