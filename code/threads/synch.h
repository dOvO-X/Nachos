// synch.h 
//	用于线程同步的数据结构。
//
//	这里定义了三种同步机制：（semaphores）、锁（locks）和条件变量（condition variables）。
//	信号量的实现已给出；后两者只给出了接口，需在实验中实现。
//
//	注意所有同步对象初始化时都要传入一个"名称"（name），仅用于调试。
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// synch.h -- synchronization primitives.  

#ifndef SYNCH_H
#define SYNCH_H

#include "copyright.h"
#include "thread.h"
#include "list.h"


// 以下类定义了一个"信号量"，其值为非负整数。
// 信号量仅支持两种操作：P() 和 V()：
//
//	P() -- 等待，直到信号量值 > 0，然后将值减 1
//
//	V() -- 将值加 1，若有线程在 P() 中等待，则唤醒其中一个
// 
// 注意：该接口*不允许*线程直接读取信号量的值——即使读取了，
// 也只能知道该值的历史状态。无法获取当前的真实值，
// 因为当你将值读取到寄存器后，可能会发生上下文切换，
// 其他线程可能已调用 P 或 V 操作，导致真实值发生变化。

class Semaphore {
  public:
    Semaphore(char* debugName, int initialValue);	// 设置信号量初始值
    ~Semaphore();   					// 释放信号量资源
    char* getName() { return name;}			// 调试辅助函数
    
    void P();	 // 信号量的唯一操作接口
    void V();	 // 两个操作均为*原子操作*
    
  private:
    char* name;        // useful for debugging
    int value;         // 信号量值，始终 >= 0
    List *queue;       // 因信号量值 <= 0 而在 P() 中等待的线程队列
};

// 以下类定义了一个"锁"。锁有两种状态：忙（BUSY）或空闲（FREE）。
// 锁仅支持两种操作：
//
//	Acquire（获取）-- 等待直到锁为空闲状态，然后将其设为忙状态
//
//	Release（释放）-- 将锁设为空闲状态，若有线程在 Acquire 中等待则唤醒其一
//
// 此外，按照约定，只有获取锁的线程才能释放它。与信号量类似，
// 你无法读取锁的状态（因为读取后状态可能立即变化）。  

class Lock {
  public:
    Lock(char* debugName);  		// 初始化锁为空闲（FREE）状态
    ~Lock();				// 释放锁资源
    char* getName() { return name; }	// debugging assist

    void Acquire(); // these are the 唯一操作接口 on a lock
    void Release(); // they are both *原子操作*

    bool isHeldByCurrentThread();	// 若当前线程持有该锁则返回 true
					// 常用于 Release 时的校验，以及下文条件变量的操作中。

  private:
    char* name;				// for debugging
    Thread *owner;                      // 记录获取锁的线程
    Semaphore *lock;                    // 底层使用信号量实现锁的核心逻辑
};

// 以下类定义了一个"条件变量"。条件变量本身无状态值，
// 但线程可以被排队等待该变量。条件变量仅支持以下操作：
//
//	Wait() -- 释放锁，放弃 CPU 直至被唤醒，
//		随后重新获取锁
//
//	Signal() -- 唤醒一个等待该条件变量的线程（若存在）
//
//	Broadcast() -- 唤醒所有等待该条件变量的线程
//
// 所有对条件变量的操作都必须在当前线程已获取锁的前提下执行。
// 事实上，对某个条件变量的所有访问都必须由同一个锁保护。
// 换句话说，调用条件变量操作的线程之间必须保证互斥。
//
// 在 Nachos 中，条件变量遵循*Mesa 风格*（Mesa-style）的语义。
// 当 Signal 或 Broadcast 唤醒另一个线程时，仅将该线程放入就绪队列，
// 被唤醒的线程需要自行重新获取锁（Wait() 内部已处理重新获取锁的逻辑）。
// 相比之下，有些实现遵循*Hoare 风格*（Hoare-style）的语义——
// 发送信号的线程会将锁的控制权和 CPU 让给被唤醒的线程，
// 被唤醒的线程立即运行，当其离开临界区时，再将锁的控制权交还给
// 发送信号的线程。
//
// 使用 Mesa 风格语义的结果是：被唤醒的线程获得运行机会前，
// 其他线程可能已获取锁并修改数据结构。

class Condition {
  public:
    Condition(char* debugName);		// 初始化条件变量为
					// "无等待线程"状态
    ~Condition();			// deallocate the condition
    char* getName() { return (name); }
    
    void Wait(Lock *conditionLock); 	// 条件变量的三个核心操作；
					// Wait() 中释放锁和进入睡眠是*原子操作*
    void Signal(Lock *conditionLock);   // 调用以下所有操作时，
    void Broadcast(Lock *conditionLock);// 当前线程必须持有 conditionLock 锁

  private:
    char* name;
    List* queue;  // 等待该条件变量的线程队列
    Lock* lock;   // 调试辅助：用于校验 Wait、Signal 和 Broadcast
                  // 传入参数的合法性
};
#endif // SYNCH_H
