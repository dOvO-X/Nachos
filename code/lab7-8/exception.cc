// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "filesys.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------


void StartProcess(int baredfunc);

void AdvancePC()
{
    machine->WriteRegister(PCReg, machine->ReadRegister(PCReg) + 4);
    machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
}


void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    } else if ((which == SyscallException) && (type == SC_Exit)) {
        //用户程序调用Exit系统调用时，内核会执行以下操作：
        //从寄存器中读取用户程序传递的退出码（ExitStatus）。
        //调用 space->Exit() 通知等待 Join 的父进程（如果存在）。
        //如果无人 Join，则自行清理地址空间。
        //调用 currentThread->Finish()，结束当前线程的执行。
        DEBUG('a', "Userprog Exit");
        int ExitStatus = machine->ReadRegister(4);
        printf("用户程序%d Exit with code %d\n", currentThread->space->getSpaceID(), ExitStatus);
        currentThread->space->Exit(ExitStatus);     // 通知等待者（如果有）
        if (!currentThread->space->isJoined())      // 无人 Join，自行清理
            delete currentThread->space;
        currentThread->Finish();
        AdvancePC();
    } else if ((which == SyscallException) && (type == SC_Exec)) {
        //用户程序调用Exec系统调用时，内核会执行以下操作：
        //从寄存器中读取用户程序传递的参数，即要执行的可执行文件的名称。
        //从内核文件系统中打开指定的 可执行文件，并创建一个新的地址空间（AddrSpace）对象来加载该文件。
        //创建一个新的线程（Thread）对象，并将新创建的地址空间分配给该线程。
        //调用新线程的Fork方法，传入StartProcess函数作为线程的入口点。
        //将新创建的地址空间的PID写回寄存器，以便用户程序能够获取到新线程的PID。
        //调用AdvancePC()，将程序计数器（PC）增加4，指向下一条指令，避免重复执行Exec系统调用。
        DEBUG('a', "Userprog Execute other userprog");
        char filename[128];
        int addr = machine->ReadRegister(4);
        int i = 0;
        do {
            //read filename from mainMemory
            machine->ReadMem(addr + i, 1, (int *) &filename[i]);
        } while (filename[i++] != '\0');

        OpenFile *executable = fileSystem->Open(filename);
        ASSERT(executable != NULL);

        AddrSpace *space = new AddrSpace(executable);
        delete executable;

        Thread *newthread = new Thread(filename);
        newthread->setSpace(space);
        newthread->Fork(StartProcess, 0);

        // currentThread->Yield();

        machine->WriteRegister(2, space->getSpaceID());
        AdvancePC();//PC 增量指向下条指令
    } else if ((which == SyscallException) && (type == SC_Join))
    {
        //用户程序调用Join系统调用时，内核会执行以下操作：
        //从寄存器中读取目标进程的 SpaceId。
        //通过全局映射表查找对应的 AddrSpace 对象。
        //调用 space->Join() 等待目标进程退出，并获取其退出状态码。
        //删除目标进程的地址空间，释放相关资源。
        //将退出状态码写回寄存器，供用户程序使用。
        //调用 AdvancePC()，将程序计数器（PC）增加4。
        DEBUG('a', "Userprog Join");
        SpaceId targetId = machine->ReadRegister(4);
        AddrSpace *targetSpace = AddrSpace::getSpaceById(targetId);
        int exitStatus = -1;
        if (targetSpace != NULL) {
            exitStatus = targetSpace->Join();   // 阻塞等待子进程退出
            delete targetSpace;                  // Join 返回后，清理子进程资源
        } else {
            printf("Join: 未找到 SpaceId %d\n", targetId);
        }
        machine->WriteRegister(2, exitStatus);   // 返回退出状态码
        AdvancePC();
    } else if ((which == SyscallException) && (type == SC_Yield))
    {
        //用户程序调用Yield系统调用时，内核会执行以下操作：
        //输出调试信息，表明用户程序正在调用Yield系统调用。
        //调用currentThread->Yield()，让出CPU，允许其他线程运行。
        //调用AdvancePC()，将程序计数器（PC）增加4，指向下一条指令，避免重复执行Yield系统调用。
        DEBUG('a', "Userprog Yield");
        currentThread->Yield();
        AdvancePC();
    } else if ((which == SyscallException) && (type == SC_Print))
    {
        //用户程序调用Print系统调用时，内核会执行以下操作：
        //输出调试信息，表明用户程序正在调用Print系统调用。
        //从寄存器中读取用户程序传递的参数，即要打印的字符串的地址。
        //从内存中读取字符串内容，直到遇到字符串结束符（'\0'）。
        //输出用户程序的PID和要打印的字符串，供调试使用。
        //调用AdvancePC()，将程序计数器（PC）增加4，指向下一条指令，避免重复执行Print系统调用。
        DEBUG('a', "Userprog Print message");
        char str[128];
        memset(str, 0, sizeof(str));
        int addr = machine->ReadRegister(4);
        int i = 0;
        do {
            machine->ReadMem(addr + i, 1, (int *) &str[i]);
        } while (str[i++] != '\0');
        //currentThread->space->Print();
        printf("用户程序%d输出:%s\n", currentThread->space->getSpaceID(), str);
        AdvancePC();
    } else
    {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
