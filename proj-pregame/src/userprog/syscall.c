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


static void syscall_handler(struct intr_frame*);
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

  if (args[0] == SYS_EXIT) {
    f->eax = args[1];
    printf("%s: exit(%d)\n", thread_current()->pcb->process_name, args[1]);
    process_exit();
  } else if (args[0] == SYS_WRITE) {
    int fd = args[1]; 
    //printf("args[2] : %s\n", (char*)args[2]);
    // args[2] is just the address
    void* buffer = (void*)args[2];
    size_t size = args[3];
    // error case : 
    // fd < 0
    if (fd < 0) {
      f->eax = -1;
      return;
    }

    int result = syscall_write(fd, buffer, size);
    f->eax = result;
  } else if (args[0] == SYS_PRACTICE) {
    int integer = args[1];
    f->eax = practice(integer);
  } else if (args[0] == SYS_HALT) {
    halt();
  } else if (args[0] == SYS_EXEC) {
    // need to check : 1. the string pointer 2. the string address 3. the string (across boundary)
    if (invalid_string_pointer((void*)(args + 1)) || invalid_vaddr((void*)args[1]) || invalid_string((void*)args[1])) {
      printf("%s: exit(%d)\n", thread_current()->pcb->process_name, -1);
      process_exit();
    } 
    char* cmd_line = (char*)args[1];
    f->eax = exec(cmd_line);
  } else if (args[0] == SYS_WAIT) {
    f->eax = wait(args[1]);
  }
}

int syscall_write(int fd, const void *buffer, unsigned size) {
    int result = size;
    if (fd == 1) {
      // STDOUT 
      putbuf(buffer, size);
    }
    
    return result;
}

int practice(int i) {
  return i + 1;
}

void halt(void) {
  shutdown_power_off();
}

pid_t exec(const char* cmd_line) {
  //pid_t pid = process_execute(cmd_line);  
  //return pid;
}

int wait(pid_t pid) {
  //int result = process_wait(pid);
  //return result;
  return 0;
}