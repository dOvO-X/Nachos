// synch.cc 
//	线程同步相关的实现函数。此处定义了三种同步机制的实现：
//	信号量（semaphores）、锁（locks）和条件变量（condition variables）
//	（后两者的实现原本需由使用者自行完成，此处已给出完整实现）。
//
// 任何同步机制的实现都依赖某种原子操作原语。本代码假定 Nachos 运行在
// 单处理器环境下，因此可以通过禁用中断来保证操作的原子性。当中断被禁用时，
// 不会发生上下文切换，当前线程将独占 CPU 直至中断重新启用。
//
// 注意：部分同步函数可能在中断已禁用的场景下被调用（例如 Semaphore::V），
// 因此在原子操作结束时，我们不会直接启用中断，而是将中断状态恢复为
// 操作前的原始状态（无论原本是禁用还是启用）。

// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	初始化信号量，使其可用于线程同步。
//
//	"debugName" 是自定义的调试名称，仅用于调试场景。
//	"initialValue" 是信号量的初始值。
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::~Semaphore
// 	销毁信号量，释放其占用的资源。
//	注意：调用此函数时需确保无线程仍在等待该信号量！
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	等待信号量值大于 0，然后将其减 1。
//	检查信号量值和执行减操作必须是原子操作，因此在检查值之前需要禁用中断。
//
//	注意：Thread::Sleep 函数要求调用时中断处于禁用状态。
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// 禁用中断
    
    while (value == 0) { 			// 信号量不可用
	queue->Append((void *)currentThread);	// 将当前线程加入等待队列
	currentThread->Sleep();// 当前线程进入睡眠状态
    } 
    value--; 					// 信号量可用，消耗其值
    
    (void) interrupt->SetLevel(oldLevel);	// 恢复中断状态（重新启用）
}

//----------------------------------------------------------------------
// Semaphore::V
// 	将信号量值加 1，若有线程在等待该信号量则唤醒其中一个。
//	与 P() 操作相同，此操作必须是原子的，因此需要先禁用中断。
//	Scheduler::ReadyToRun() 函数要求调用时中断处于禁用状态。
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove(); // 从等待队列取出第一个线程
    if (thread != NULL)	   // 若存在等待线程，将其设为就绪状态（立即消耗 V 操作的唤醒信号）
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}


//----------------------------------------------------------------------
// Lock::Lock
// 	初始化锁，使其可用于线程同步。
//
//	"debugName" 是自定义的调试名称，仅用于调试场景。
//----------------------------------------------------------------------


Lock::Lock(char* debugName) 
{
    name = debugName;
    owner = NULL;
    lock = new Semaphore(name,1);
}


//----------------------------------------------------------------------
// Lock::~Lock
// 	销毁锁，释放其占用的资源。
//	与信号量相同，调用此函数时需确保无线程仍在等待该锁。
//----------------------------------------------------------------------
Lock::~Lock() 
{
    delete lock;
}

//----------------------------------------------------------------------
// Lock::Acquire
//      基于二元信号量实现锁的获取操作。
//      记录获取锁的线程，确保只有该线程能释放此锁。
//----------------------------------------------------------------------
void Lock::Acquire() 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);  // disable interrupts

    lock->P();                           // 获取信号量（实现锁的占用）
    owner = currentThread;                // 记录锁的当前持有线程
    (void) interrupt->SetLevel(oldLevel); // re-enable interrupts
}

//----------------------------------------------------------------------
// Lock::Release
//      释放锁（即释放底层的二元信号量）。
//      校验当前线程是否有权释放此锁（必须是持有锁的线程）。
//----------------------------------------------------------------------
void Lock::Release() 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);  // disable interrupts

    // 断言校验：a) 锁处于被占用状态  b) 当前线程是获取锁的线程
    ASSERT(currentThread == owner);        
    owner = NULL;                          // 清空锁的持有线程记录
    lock->V();                             // 释放信号量（实现锁的释放）
    (void) interrupt->SetLevel(oldLevel);
}


//----------------------------------------------------------------------
// Lock::isHeldByCurrentThread
//      检查当前线程是否持有该锁，返回布尔值。
//----------------------------------------------------------------------
bool Lock::isHeldByCurrentThread()
{
    bool result;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    result = currentThread == owner;// 判断当前线程是否为锁的持有者
    (void) interrupt->SetLevel(oldLevel);
    return(result);
}

//----------------------------------------------------------------------
// Condition::Condition
// 	初始化条件变量，使其可用于线程同步。
//
//	"debugName" 是自定义的调试名称，仅用于调试场景。
//----------------------------------------------------------------------
Condition::Condition(char* debugName) 
{ 
    name = debugName;
    queue = new List;
    lock = NULL;
}

//----------------------------------------------------------------------
// Condition::~Condition
// 	销毁条件变量，释放其占用的资源。
//	与信号量相同，调用此函数时需确保无线程仍在等待该条件变量。
//----------------------------------------------------------------------

Condition::~Condition() 
{ 
    delete queue;
}

//----------------------------------------------------------------------
// Condition::Wait
//
//      释放锁并放弃 CPU，直至被唤醒；唤醒后重新获取该锁。
//
//      前置条件：
//      1. 当前线程持有传入的 conditionLock 锁；
//      2. 等待队列中的所有线程都在等待同一个锁。
//----------------------------------------------------------------------
void Condition::Wait(Lock* conditionLock) 
{ 
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    // 断言校验前置条件：当前线程持有该锁
    ASSERT(conditionLock->isHeldByCurrentThread());  
    if(queue->IsEmpty()) {
	lock = conditionLock; // 队列为空时，绑定条件变量与当前锁（用于后续校验）
    } 
    // 断言校验：所有操作该条件变量的线程必须使用同一个锁
    ASSERT(lock == conditionLock); 
    queue->Append(currentThread);  // 将当前线程加入条件变量的等待队列
    conditionLock->Release();      // 释放锁（让其他线程可获取）
    currentThread->Sleep();        // goto sleep
    conditionLock->Acquire();      // 被唤醒后，重新获取锁
    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Condition::Signal
//      唤醒一个等待该条件变量的线程（若存在）。
//   
//      前置条件：
//      1. 当前线程持有传入的 conditionLock 锁；
//      2. 等待队列中的所有线程都在等待同一个锁。
//----------------------------------------------------------------------
void Condition::Signal(Lock* conditionLock) 
{ 
    Thread *nextThread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    // 断言校验：当前线程持有该锁
    ASSERT(conditionLock->isHeldByCurrentThread());
    if(!queue->IsEmpty()) {
    // 断言校验：操作的锁与条件变量绑定的锁一致
	ASSERT(lock == conditionLock);
	nextThread = (Thread *)queue->Remove();// 取出等待队列中的第一个线程
	scheduler->ReadyToRun(nextThread);      // wake up the thread
    } 
    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Condition::Broadcast
//      唤醒所有等待该条件变量的线程。
//
//      前置条件：
//      1. 当前线程持有传入的 conditionLock 锁；
//      2. 等待队列中的所有线程都在等待同一个锁。
//----------------------------------------------------------------------
void Condition::Broadcast(Lock* conditionLock) 
{ 
    Thread *nextThread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    // 断言校验：当前线程持有该锁
    ASSERT(conditionLock->isHeldByCurrentThread());
    if(!queue->IsEmpty()) {
    // 断言校验：操作的锁与条件变量绑定的锁一致
	ASSERT(lock == conditionLock);
    // 循环取出所有等待线程并唤醒
	while(nextThread = (Thread *)queue->Remove()) {
	    scheduler->ReadyToRun(nextThread);  // wake up the thread
	}
    } 
    (void) interrupt->SetLevel(oldLevel);
}
