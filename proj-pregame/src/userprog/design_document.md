## argument passing：
### 1. deal with args having only file-name 
&emsp; after this, should see : \
&emsp;&emsp; 1. file_name on the top of stack in _start function with GDB\
&emsp;&emsp; 2. practice syscall should have some progress
 ### status : at least worked, but not refactored
### 2. deal with one/many args passing
&emsp; what to do \
&emsp;&emsp; 1. break file_name into tokens separated by ' ' \
&emsp;&emsp; 2. store : struct process member [argc] [argv]
