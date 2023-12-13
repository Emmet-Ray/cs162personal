#include <stdio.h>
#include <syscall-nr.h>
#include <console.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "userprog/syscall.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"
#include "lib/syscall-nr.h"
#include "lib/string.h"
#include "lib/float.h"
#include "filesys/filesys.h"
#include "filesys/file.h"


static void syscall_handler(struct intr_frame*);

bool invalid_vaddr(void* addr); 
bool invalid_string(void* string_); 
bool invalid_string_pointer(void* string_p); 
bool invalid_buffer(void* buffer); 

// check an [addr] is valid or not, if invalid return true;
// invalid type : NULL || not user_vaddr || unmapped || ...
bool invalid_vaddr(void* addr) {
  return !addr || !is_user_vaddr(addr) || !pagedir_get_page(thread_current()->pcb->pagedir, addr);
}
// the string maybe cross the boundary
bool invalid_string(void* string_) {
  char* string = (char*)string_;
  while (!invalid_vaddr((void*)string)) {
    if (*string == '\0') {
      return false;
    }
    string++;
  }
  return true;
}
// the pointer maybe cross the page boundary
bool invalid_string_pointer(void* string_p) {
    void* up_bound = pg_round_up(string_p);
    uint32_t cross = up_bound - string_p;
    if (cross > 3) {
      return invalid_vaddr(string_p);
    } else {
      return invalid_vaddr(string_p) || invalid_vaddr(up_bound);
    }
}
bool invalid_buffer(void* buffer) {
  return invalid_string_pointer((void*)(buffer)) || invalid_vaddr((void*)*(uint32_t*)buffer) || invalid_string((void*)*(uint32_t*)buffer);
}
extern void start_process(void* argument);
void syscall_init(void) { intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall"); }



static void syscall_handler(struct intr_frame* f UNUSED) {
  uint32_t* args = ((uint32_t*)f->esp);

  /*
   * The following print statement, if uncommented, will print out the syscall
   * number whenever a process enters a system call. You might find it useful
   * when debugging. It will cause tests to fail, however, so you should not
   * include it in your final submission.
   */

  /* printf("System call number: %d\n", args[0]); */
  if (invalid_vaddr(args) || invalid_string_pointer((void*)args)) {
    add_to_exit_list(-1);
    process_exit();
  }
  if (args[0] == SYS_EXIT) {
    if (invalid_vaddr((void*)(args + 1))) {
      process_exit();
    }
    add_to_exit_list(args[1]);
    process_exit();
  } else if (args[0] == SYS_WRITE) {
    if (invalid_vaddr((void*)(args + 1)) || invalid_buffer((void*)(args + 2)) || invalid_vaddr((void*)(args + 3))) {
      add_to_exit_list(-1);
      process_exit();
    }
    f->eax = write(args[1], (void*)args[2], args[3]);
  } else if (args[0] == SYS_PRACTICE) {
    int integer = args[1];
    f->eax = practice(integer);
  } else if (args[0] == SYS_HALT) {
    halt();
  } else if (args[0] == SYS_EXEC) {
    // need to check : 1. the string pointer 2. the string address 3. the string (across boundary)
    if (invalid_buffer((void*)(args + 1))) {
      add_to_exit_list(-1);
      process_exit();
    } 
    f->eax = exec((char*)args[1]);
  } else if (args[0] == SYS_WAIT) {
    f->eax = wait(args[1]);
  } else if (args[0] == SYS_CREATE) {
    if (invalid_buffer((void*)(args + 1)) || invalid_vaddr((void*)(args + 2))) {
      add_to_exit_list(-1);
      process_exit();
    } 
    f->eax = create((char*)args[1], (unsigned)args[2]);
  } else if (args[0] == SYS_REMOVE) {
    if (invalid_buffer((void*)(args + 1))) {
      add_to_exit_list(-1);
      process_exit();
    }
    f->eax = remove((char*)args[1]);
  } else if (args[0] == SYS_OPEN) {
    if (invalid_buffer((void*)(args + 1))) {
      add_to_exit_list(-1);
      process_exit();
    }
    f->eax = open((char*)args[1]);
  } else if (args[0] == SYS_FILESIZE) {
    if (invalid_vaddr((void*)(args + 1))) {
      add_to_exit_list(-1);
      process_exit();
    }
    f->eax = filesize(args[1]);
  } else if (args[0] == SYS_READ) {
    if (invalid_vaddr((void*)(args + 1)) || invalid_vaddr((void*)(args + 2)) || invalid_string_pointer((void*)(args[2])) || invalid_vaddr((void*)(args + 3))) {
      add_to_exit_list(-1);
      process_exit();
    }
    f->eax = read(args[1], (void*)args[2], args[3]);
  } else if (args[0] == SYS_SEEK) {
    if (invalid_vaddr((void*)(args + 1)) || invalid_vaddr((void*)(args + 2))) {
      add_to_exit_list(-1);
      process_exit();
    }
    seek(args[1], args[2]);
  } else if (args[0] == SYS_TELL) {
    if (invalid_vaddr((void*)(args + 1))) {
      add_to_exit_list(-1);
      process_exit();
    }
    f->eax = tell(args[1]);
  } else if (args[0] == SYS_CLOSE) {
    if (invalid_vaddr((void*)(args + 1))) {
      add_to_exit_list(-1);
      process_exit();
    }
    close(args[1]);
  } else if (args[0] == SYS_COMPUTE_E) {
    if (invalid_vaddr((void*)(args + 1))) {
      add_to_exit_list(-1);
      process_exit();
    }
    f->eax = compute_e(args[1]);
  }
}

int practice(int i) {
  return i + 1;
}

void halt(void) {
  shutdown_power_off();
}

pid_t exec(const char* cmd_line) {

  pid_t pid = process_execute(cmd_line);  
  return pid;
}

int wait(pid_t pid) {
  int result = process_wait(pid);
  return result;
}

// file operation syscalls
bool create(const char* file, unsigned file_size) {
  lock_acquire(&temporary);
  bool result = filesys_create(file, file_size);
  lock_release(&temporary);
  return result;
}

bool remove(const char* file) {
  lock_acquire(&temporary);
  bool result = filesys_remove(file);
  lock_release(&temporary);
  return result;
}

int open(const char* file) {
  struct file* result = filesys_open(file);
  if (!result) {
    return -1;
  }
  lock_acquire(&temporary);
  int result_ = add_to_file_list(result);
  lock_release(&temporary);
  return result_;
}

int filesize(int fd) {
  struct file* result = find_in_file_list(fd);
  if (!result) {
    return -1;
  }
  lock_acquire(&temporary);
  int result_ = file_length(result);
  lock_release(&temporary);
  return result_;
}

int read (int fd, void *buffer, unsigned size) {
  // todo: stdin case
  if (fd == 1) {
  }
  struct file* result = find_in_file_list(fd);
  if (!result) {
    return -1;
  }
  lock_acquire(&temporary);
  int result_ = file_read(result, buffer, size);
  lock_release(&temporary);
  return result_;
}

int write(int fd, const void *buffer, unsigned size) {
  if (fd == 1) {
    // STDOUT 
    // todo: maybe need to break up large buffers
    lock_acquire(&temporary);
    putbuf(buffer, size);
    int result = size;
    lock_release(&temporary);
    return result;
  }
  struct file* result = find_in_file_list(fd);
  if (!result) {
    return -1;
  }
  lock_acquire(&temporary);
  int result_ = file_write(result, buffer, size);
  lock_release(&temporary);
  return result_;
}

void seek (int fd, unsigned position) {
  struct file* result = find_in_file_list(fd);
  if (!result) {
    return;
  } 
  lock_acquire(&temporary);
  file_seek(result, position);
  lock_release(&temporary);
}

unsigned tell(int fd) {
  struct file* result = find_in_file_list(fd);
  if (!result) {
    return -1;
  } 
  lock_acquire(&temporary);
  unsigned result_ = file_tell(result);
  lock_release(&temporary);
  return result_;
}

void close (int fd) {
  struct file* result = find_in_file_list(fd);
  if (!result) {
    return;
  } 
  lock_acquire(&temporary);
  file_close(result);
  lock_release(&temporary);
  remove_from_file_list(fd);
}

double compute_e (int n) {
  return sys_sum_to_e(n);
}