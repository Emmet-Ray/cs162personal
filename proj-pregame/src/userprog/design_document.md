## argument passing：
### 1. deal with args having only file-name 
&emsp; status : at least worked for the given tests \

&emsp; after this, expected: \
&emsp;&emsp; 1. file_name on the top of stack in _start function with GDB\
&emsp;&emsp; 2. practice syscall should have some progress

### 2. deal with one/many args passing
&emsp; status : at least worked for the given tests \

&emsp; what to do \
&emsp;&emsp; 1. break file_name into tokens separated by ' ' \
&emsp;&emsp; 2. store : struct process member [argc] [argv]

## process control syscalls
### 1. practice 2. halt 
&emsp; both are trivial
### 2. exec
&emsp; status : at least worked for the given tests \

&emsp; important point: \
&emsp;&emsp; synchronization: before return from exec syscall, \
&emsp;&emsp; make sure the child process executable is loaded successfully or not. \

&emsp; what did I do: \
&emsp;&emsp; 1. basically, reuse the provied code: [thread_create] in thread.c && [start_process] function in process.c \
&emsp;&emsp; but with some change : \
&emsp;&emsp;&emsp; 1. change the signature to a [struct start_process_args] pointer that is defined in process.h \
&emsp;&emsp;&emsp; 2. synchronize using the semaphore in the struct && a bool value to signify load result \
&emsp;&emsp; 2. check args, especially the cmd_line string




