#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "userprog/pagedir.h"

static void syscall_handler(struct intr_frame*);

void syscall_init(void) { intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall"); }

void syscall_exit(int status) {
  printf("%s: exit(%d)\n", thread_current()->name, status);
  thread_exit();
}

/*
 * This does not check that the buffer consists of only mapped pages; it merely
 * checks the buffer exists entirely below PHYS_BASE.
 */
static void validate_buffer_in_user_region(const void* buffer, size_t length) {
  uintptr_t delta = PHYS_BASE - buffer;
  if (!is_user_vaddr(buffer) || length > delta) {
    syscall_exit(-1);
  }
}

/*
 * This does not check that the string consists of only mapped pages; it merely
 * checks the string exists entirely below PHYS_BASE.
 */
static void validate_string_in_user_region(const char* string) {
  uintptr_t delta = PHYS_BASE - (const void*)string;
  if (!is_user_vaddr(string) || strnlen(string, delta) == delta)
    syscall_exit(-1);
}

static int syscall_open(const char* filename) {
  struct thread* t = thread_current();
  if (t->open_file != NULL)
    return -1;

  t->open_file = filesys_open(filename);
  if (t->open_file == NULL)
    return -1;

  return 2;
}

static int syscall_write(int fd, void* buffer, unsigned size) {
  struct thread* t = thread_current();
  if (fd == STDOUT_FILENO) {
    putbuf(buffer, size);
    return size;
  } else if (fd != 2 || t->open_file == NULL)
    return -1;

  return (int)file_write(t->open_file, buffer, size);
}

static int syscall_read(int fd, void* buffer, unsigned size) {
  struct thread* t = thread_current();
  if (fd != 2 || t->open_file == NULL)
    return -1;

  return (int)file_read(t->open_file, buffer, size);
}

static void syscall_close(int fd) {
  struct thread* t = thread_current();
  if (fd == 2 && t->open_file != NULL) {
    file_close(t->open_file);
    t->open_file = NULL;
  }
}
/*
static void* sbrk_increase(intptr_t increment) {
  struct thread* t = thread_current();
  void* result = (void*) t->heap_end;
  int num_page = increment/PGSIZE;
  if (increment % PGSIZE != 0)
    num_page++;
  if (pagedir_get_page(t->pagedir, (void*) t->heap_end)) 
    num_page--;

  void* temp = palloc_get_multiple(PAL_USER, num_page);
  if (num_page > 0 && !temp) {
    return (void*)-1;
  }
  palloc_free_multiple(temp, num_page);

  while (increment > 0) {
    if (!pagedir_get_page(t->pagedir, (void*) t->heap_end)) {
      void* kpage = palloc_get_page(PAL_ZERO | PAL_USER);
      pagedir_set_page(t->pagedir, (void*) pg_round_down((void*)t->heap_end), kpage, true);
    }
    if (increment >= PGSIZE) {
      t->heap_end += PGSIZE;
    } else {
      t->heap_end += increment;
    }
    increment -= PGSIZE;
  }

  if (!pagedir_get_page(t->pagedir, (void*) t->heap_end) && t->heap_end > (uint32_t)(pg_round_down((void*)t->heap_end))) {
    void* kpage = palloc_get_page(PAL_ZERO | PAL_USER);
    pagedir_set_page(t->pagedir, (void*) pg_round_down((void*)t->heap_end), kpage, true);
  }

  return result;
}
static void* sbrk_decrease(intptr_t decrement) {
  struct thread* t = thread_current();
  void* result = (void*) t->heap_end;
  intptr_t positive = -decrement;

  while (positive > 0) {
    if (t->heap_end <= t->heap_start) {
      t->heap_end = t->heap_start;
      break;
    }

    if (positive >= PGSIZE)  {
      void* temp_base = pg_round_down((void*) t->heap_end);
      pagedir_clear_page(t->pagedir, temp_base);
      palloc_free_page(pagedir_get_page(t->pagedir, temp_base));
      t->heap_end -= PGSIZE;
    } else {
      void* temp_base = pg_round_down((void*) t->heap_end);
      t->heap_end -= positive;
      if (t->heap_end <= (uint32_t)temp_base) {
        pagedir_clear_page(t->pagedir, temp_base);
        palloc_free_page(pagedir_get_page(t->pagedir, temp_base));
      }
    }
    positive -= PGSIZE;
  }
  if (t->heap_end <= t->heap_start) {
    t->heap_end = t->heap_start;
  }
  return result;
}
*/
static void* syscall_sbrk(intptr_t increment) {
  /*
  struct thread* t = thread_current();
  void* result = (void*) t->heap_end;
  if (increment < 0) {
    return sbrk_decrease(increment);
  } else if (increment > 0) {
    return sbrk_increase(increment);
  } 
  return result;
  */
  struct thread* t = thread_current();
  void* brk_ = t->brk;
  t->brk += increment;
  if (t->brk > pg_round_up(brk_)) {
    void* upage;
    bool fail = false;
    for (upage = pg_round_up(brk_); upage != pg_round_up(t->brk); upage += PGSIZE) {
      void* kpage = palloc_get_page(PAL_ZERO | PAL_USER);
      if (kpage == NULL) {
        fail = true;
        break;
      }
      pagedir_set_page(t->pagedir, upage, kpage, true);
    }
    if (fail) {
      for (void* pg = pg_round_up(brk_); pg != upage; pg += PGSIZE) {
        palloc_free_page(pagedir_get_page(t->pagedir, pg));
        pagedir_clear_page(t->pagedir, pg);
      }
      t->brk = brk_;
      return (void*)-1;
    }
  } else if (t->brk <= pg_round_down(brk_)) {
    for (void* upage = pg_round_down(brk_); upage != pg_round_down(t->brk - 1); upage -= PGSIZE) {
      palloc_free_page(pagedir_get_page(t->pagedir, upage));
      pagedir_clear_page(t->pagedir, upage);
    }
  }
  return brk_;

}


static void syscall_handler(struct intr_frame* f) {
  uint32_t* args = (uint32_t*)f->esp;
  struct thread* t = thread_current();
  t->in_syscall = true;

  validate_buffer_in_user_region(args, sizeof(uint32_t));
  switch (args[0]) {
    case SYS_EXIT:
      validate_buffer_in_user_region(&args[1], sizeof(uint32_t));
      syscall_exit((int)args[1]);
      break;

    case SYS_OPEN:
      validate_buffer_in_user_region(&args[1], sizeof(uint32_t));
      validate_string_in_user_region((char*)args[1]);
      f->eax = (uint32_t)syscall_open((char*)args[1]);
      break;

    case SYS_WRITE:
      validate_buffer_in_user_region(&args[1], 3 * sizeof(uint32_t));
      validate_buffer_in_user_region((void*)args[2], (unsigned)args[3]);
      f->eax = (uint32_t)syscall_write((int)args[1], (void*)args[2], (unsigned)args[3]);
      break;

    case SYS_READ:
      validate_buffer_in_user_region(&args[1], 3 * sizeof(uint32_t));
      validate_buffer_in_user_region((void*)args[2], (unsigned)args[3]);
      f->eax = (uint32_t)syscall_read((int)args[1], (void*)args[2], (unsigned)args[3]);
      break;

    case SYS_CLOSE:
      validate_buffer_in_user_region(&args[1], sizeof(uint32_t));
      syscall_close((int)args[1]);
      break;

    case SYS_SBRK:
      validate_buffer_in_user_region(&args[1], sizeof(uint32_t));
      f->eax = syscall_sbrk((intptr_t)args[1]);
      break;

    default:
      printf("Unimplemented system call: %d\n", (int)args[0]);
      break;
  }

  t->in_syscall = false;
}
