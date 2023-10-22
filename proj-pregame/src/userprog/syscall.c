#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <console.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"

static void syscall_handler(struct intr_frame*);

void syscall_init(void) { intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall"); }
int syscall_write(int fd, const void *buffer, unsigned size);

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
  }
}

int syscall_write(int fd, const void *buffer, unsigned size) {
    //printf("fd : %d, buffer : %s, size : %d\n", fd, (char*)buffer, size);
    int result = size;
    if (fd == 1) {
      // STDOUT 
      //printf("<1>\n");
      putbuf(buffer, size);
      //printf("<2>\n");
    }
    
    return result;
}