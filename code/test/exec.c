#include "syscall.h"

int main()
{
    SpaceId pid;
    int status;

    Print("Parent: starting Exec test\n");

    pid = Exec("../test/exec_test.noff");

    Print("Parent: child created, now calling Join...\n");
    status = Join(pid);

    Print("Parent: Join returned, child has exited\n");
    Exit(0);
}

 