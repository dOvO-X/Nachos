#include "syscall.h"

int main()
{
    Print("子进程 exec_test 开始运行\n");
    Print("子进程 exec_test 即将退出，返回状态码 42\n");
    Exit(42);
    /* not reached */
}
 