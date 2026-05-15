#include "syscall.h" 
int  main() 
{ 
    SpaceId pid; 
    pid=Exec("../test/halt.noff");   
    Yield();
    Exit(0);
} 