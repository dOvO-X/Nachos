// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

// typedef int SpaceId;

#include "copyright.h"
#include "filesys.h"

#undef min
#undef max

#include <set>
#include <map>
#include "translate.h"
#include "syscall.h"
#include "bitmap.h"

class Semaphore;  // 前向声明，避免循环包含


#define UserStackSize		1024 	// increase this as necessary!

class AddrSpace {
  public:
    AddrSpace(OpenFile *executable);	// Create an address space,
					// initializing it with the program
					// stored in the file "executable"
    ~AddrSpace();			// De-allocate an address space

    void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code

    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch 
    void Print();			// Print the address space (debugging)

    SpaceId getSpaceID(){ return pid; };

    // Join 相关方法
    int Join();                     // 等待该地址空间退出，返回退出状态
    void Exit(int status);          // 设置退出状态并通知等待者
    int getExitStatus() { return exitStatus; }
    bool isJoined() { return joined; }       // 是否已被 Join
    static AddrSpace* getSpaceById(SpaceId id); // 通过 SpaceId 查找地址空间

    static int globalPid;//全局变量，记录当前分配的最大PID值，每创建一个新的地址空间时，globalPid加1，并将新的PID值分配给新创建的地址空间。
    static std::set<SpaceId> pids;
    //全局变量，使用一个集合来记录当前正在使用的PID值。
    // 当创建一个新的地址空间时，将新的PID值添加到集合中；
    // 当一个地址空间退出时，将对应的PID值从集合中移除。
    // 通过检查集合中的PID值，可以确保每个地址空间都有一个唯一的PID，
    // 并且在地址空间退出后，PID值可以被重新分配给新的地址空间。
    static BitMap memfreemap;//全局变量，使用一个位图来管理内存页的分配情况。
    static std::map<SpaceId, AddrSpace*> spaceMap; //全局映射表，通过SpaceId查找AddrSpace

  private:
    TranslationEntry *pageTable;	// Assume linear page table translation
					// for now!
    unsigned int numPages;		// Number of pages in the virtual 
					// address space
    SpaceId pid;
    int exitStatus;                 // 进程退出状态码
    Semaphore *joinSemaphore;       // 用于 Join 等待的信号量（初值为0）
    bool joined;                    // 是否已被 Join（防止无人回收）
};

#endif // ADDRSPACE_H
