## argument passing：
### 1. deal with args having only file-name 
&emsp; after this, expected: \
&emsp;&emsp; 1. file_name on the top of stack in _start function with GDB\
&emsp;&emsp; 2. practice syscall should have some progress
### status : at least worked for the given tests
### 2. deal with one/many args passing
&emsp; what to do \
&emsp;&emsp; 1. break file_name into tokens separated by ' ' \
&emsp;&emsp; 2. store : struct process member [argc] [argv]
### status : at least worked for the given tests

## process control syscalls
### 1. practice 2. halt 
&emsp; both are trivial
### 2. exec
&emsp; what to do 

&emsp; important point: \
&emsp;&emsp; synchronization: before return from exec syscall, \
&emsp;&emsp; make sure the child process executable is loaded successfully or not. \
&emsp; after this, expected: \
&emsp;&emsp; test exec-missing should pass
