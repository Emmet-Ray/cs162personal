#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include <stdint.h>

// At most 8MB can be allocated to the stack
// These defines will be used in Project 2: Multithreading
#define MAX_STACK_PAGES (1 << 11)
#define MAX_THREADS 127

/* PIDs and TIDs are the same type. PID should be
   the TID of the main thread of the process */
typedef tid_t pid_t;

/* Thread functions (Project 2: Multithreading) */
typedef void (*pthread_fun)(void*);
typedef void (*stub_fun)(pthread_fun, void*);

/* The process control block for a given process. Since
   there can be multiple threads per process, we need a separate
   PCB from the TCB. All TCBs in a process will have a pointer
   to the PCB, and the PCB will have a pointer to the main thread
   of the process, which is `special`. */
struct process {
  /* Owned by process.c. */
  uint32_t* pagedir;          /* Page directory. */
  char process_name[16];      /* Name of the main thread */
  struct thread* main_thread; /* Pointer to main thread */
  uint32_t file_name_len;     // the len of the input file_name(untokenized), used for setup_stack
  uint32_t argc;
  char** argv;
  struct list children_list;
  int exit_status;
};

struct exit_status {
  pid_t pid;
  int exit_status;
  struct list_elem elem;
};
struct syn_wait {
  pid_t p_pid;
  pid_t c_pid;
  struct semaphore wait_sema;
  struct list_elem elem;
};

// wrapper around child pid
struct child_pid {
   pid_t pid;
   struct list_elem elem;
};

struct syn_load {
  struct semaphore load_sema;
  bool load_success;
  char* file_name;
};


void userprog_init(void);

pid_t process_execute(const char* file_name);
int process_wait(pid_t);
void process_exit(void);
void process_activate(void);

bool is_main_thread(struct thread*, struct process*);
pid_t get_pid(struct process*);

tid_t pthread_execute(stub_fun, pthread_fun, void*);
tid_t pthread_join(tid_t);
void pthread_exit(void);
void pthread_exit_main(void);

// new additions
void add_to_children_list(pid_t child_pid);
bool is_my_child(pid_t child_pid); 
void remove_from_children_list(pid_t child_pid); 

struct syn_wait* find_self_int_wait_list(void);
struct syn_wait* add_to_wait_list(pid_t child_pid);
void remove_from_wait_list(pid_t child_pid); 

void add_to_exit_list(int exit_status);
void remove_from_exit_list(pid_t pid); 
struct exit_status* find_in_exit_list(pid_t pid);

void show_exit_list();
#endif /* userprog/process.h */
