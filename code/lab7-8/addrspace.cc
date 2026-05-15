// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
// #include <set>
// #include "bitmap.h"


//----------------------------------------------------------------------
//pid和内存管理相关的全局变量
int AddrSpace::globalPid = 100;
std::set<SpaceId> AddrSpace::pids;
BitMap AddrSpace::memfreemap(NumPhysPages);

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    //为每个地址空间分配一个唯一的PID
    if (pids.count(globalPid) == 0)
    {
        //如果当前 globalPid 没有被占用，则直接分配给新创建的地址空间，
        //并将 globalPid 加 1，以便下一个地址空间使用。
        pid = globalPid;
        globalPid++;
        if (globalPid == 1024)//如果 globalPid 达到 1024，则重新从100开始分配 PID。
            globalPid = 100;
        pids.insert(pid);
    } else
    {
        //如果当前 globalPid 已经被占用，则继续增加 globalPid，直到找到一个未被占用的 PID。
        int prepid = globalPid;
        while (pids.count(globalPid))
        {
            globalPid++;
            if (globalPid == 1024)
                globalPid = 100;
            if (globalPid == prepid)
            {
                printf("exceed max processes");
                interrupt->Halt();
            }
        }
        pid = globalPid;
        globalPid++;
        if (globalPid == 1024)
            globalPid = 100;
        pids.insert(pid);
    } 
    NoffHeader noffH;
    unsigned int i, size;
//读取可执行文件的头部（NoffHeader）
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// 计算地址空间大小
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);		//检查是否超过物理内存最大页数

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// 设置页表项的有效位、只读位等属性
    pageTable = new TranslationEntry[numPages];//为每一页分配一个页表项
    for (i = 0; i < numPages; i++) {
	pageTable[i].virtualPage = i;	
	// pageTable[i].physicalPage = i;
    // 为页表项从memfreemap位图中分配一个物理页，并将该页标记为已使用
    pageTable[i].physicalPage = memfreemap.Find();
    ASSERT(pageTable[i].physicalPage != -1);
	pageTable[i].valid = TRUE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only
    }
    
// 清零内存
// 用 bzero 将整个用户地址空间（包括未初始化数据段和栈）清零，以确保未初始化数据段和栈中的内容为零。这是因为在 C/C++ 中，未初始化的全局变量和局部变量的值是不确定的，可能包含垃圾值。通过清零内存，可以确保这些变量在程序开始执行时具有确定的初始值。
    //bzero(machine->mainMemory, size);

// // 加载代码和数据段
//     //如果代码段大小大于 0，则将代码段内容从可执行文件读入内存的指定位置。
//     if (noffH.code.size > 0) {
//         DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
// 			noffH.code.virtualAddr, noffH.code.size);
//         executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
// 			noffH.code.size, noffH.code.inFileAddr);
//     }
//     //如果已初始化数据段大小大于 0，则将已初始化数据段内容从可执行文件读入内存的指定位置。
//     if (noffH.initData.size > 0) {
//         DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
// 			noffH.initData.virtualAddr, noffH.initData.size);
//         executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
// 			noffH.initData.size, noffH.initData.inFileAddr);
//     }
    // 通过一个循环来处理代码段、已初始化数据段和未初始化数据段这三个部分，
    // 确保它们正确地加载到内存中，处理了跨页的情况。
    int reCodeSize = noffH.code.size;//记录代码段剩余未加载的大小
    int reIdataSize = noffH.initData.size;//记录已初始化数据段剩余未加载的大小
    int reUdataSize = noffH.uninitData.size + UserStackSize;//记录未初始化数据段和栈剩余未加载的大小
    i = 0;//记录当前正在处理的页表项索引，从第0页开始加载。
    int currentSize = PageSize;//记录当前页剩余的大小，初始值为一页的大小
    while (reCodeSize || reIdataSize || reUdataSize)
    {
        int phyPos = pageTable[i].physicalPage * PageSize + PageSize - currentSize;
        if (reCodeSize)//如果代码段还有未加载的部分，则优先加载代码段
        {
            int writedSize = noffH.code.size - reCodeSize;
            if (currentSize > reCodeSize)
            {
                executable->ReadAt(&(machine->mainMemory[phyPos]),
                                   reCodeSize, noffH.code.inFileAddr + writedSize);
                currentSize -= reCodeSize;
                reCodeSize = 0;
            } else
            {
                executable->ReadAt(&(machine->mainMemory[phyPos]),
                                   currentSize, noffH.code.inFileAddr + writedSize);
                reCodeSize -= currentSize;
                i++;
                currentSize = PageSize;
            }
        } else if (reIdataSize)
        {
            int writedSize = noffH.initData.size - reIdataSize;
            if (currentSize > reIdataSize)
            {
                executable->ReadAt(&(machine->mainMemory[phyPos]),
                                   reIdataSize, noffH.initData.inFileAddr + writedSize);
                currentSize -= reIdataSize;
                reIdataSize = 0;
            } else
            {
                executable->ReadAt(&(machine->mainMemory[phyPos]),
                                   currentSize, noffH.initData.inFileAddr + writedSize);
                reIdataSize -= currentSize;
                i++;
                currentSize = PageSize;
            }
        } else if (reUdataSize > 0)
        {
            bzero(&(machine->mainMemory[phyPos]), currentSize);
            i++;
            currentSize = PageSize;
            reUdataSize -= currentSize;
        }    
    }
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   delete [] pageTable;
   //当一个地址空间退出时，将对应的PID值从集合中移除，
   //并将该地址空间占用的物理页标记为可用，以便其他地址空间可以重新分配这些资源。
    pids.erase(pid);
    for (int i = 0; i < numPages; ++i)
    {
        memfreemap.Clear(pageTable[i].physicalPage);
    }
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
   
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    currentThread->SaveUserState();//保存用户寄存器状态，以便在上下文切换时能够恢复
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

void AddrSpace::Print() { 
        printf("spaceId %d:", pid);
    printf("page table dump:  %d pages  in total\n", numPages);  
    printf("=============================\n");  
    printf("\tVirtPage, \tPhysPage\n"); 
    for (int i=0; i < numPages;  i++) { 
    printf("\t %d, \t\t%d\n", pageTable[i].virtualPage, pageTable[i].physicalPage); 
    } 
    printf("============================================\n\n"); 
} 