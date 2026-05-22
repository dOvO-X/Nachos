/* nojoin.c
 *	Test: parent creates a child via Exec but does NOT Join it.
 *	Parent just exits immediately. Child exits later on its own.
 *	Verifies that orphaned address spaces are cleaned up.
 */

#include "syscall.h"

int main()
{
    SpaceId pid;

    Print("Parent: starting Exec, NO Join test\n");

    pid = Exec("../test/exec_test.noff");

    Print("Parent: child created, NOT calling Join, just Exiting...\n");
    Exit(0);
}
