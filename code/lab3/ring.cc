// ring.cc
//	Routines to implement a ring buffer for producer and consumer 
//      problem.
//	
// Copyright (c) 1995 The Regents of the University of Southern Queensland.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

extern "C" {
#include <stdio.h>
extern int exit(int st);
}

#include "ring.h"

//----------------------------------------------------------------------
// slot::slot
// 	The constructor for the slot class.  
//----------------------------------------------------------------------

slot::slot(int id, int number)
{
    thread_id = id;
    value = number;
}



//----------------------------------------------------------------------
// Ring::Ring
// 	Ring 类的构造函数。注意：构造函数没有返回类型。
//
// 	参数 "sz" —— 环形缓冲区在任意时刻可容纳的最大元素数量（即槽位数量）。
//----------------------------------------------------------------------

Ring::Ring(int sz)
{
    if (sz < 1) {
	fprintf(stderr, "Error: Ring: size %d too small\n", sz);
	exit(1);
    }

    // Initialize the data members of the ring object.
    size = sz;
    in = 0;
    out = 0;
    buffer = new slot[size]; // 分配一个 slot 类型的数组
}

//----------------------------------------------------------------------
// Ring::~Ring
// 	The destructor for the Ring class.  Just get rid of the array we
// 	allocated in the constructor.
//----------------------------------------------------------------------

Ring::~Ring()
{
    // Some compilers and books tell you to write this as:
    //     delete [size] stack;
    // but apparently G++ doesn't like that.

    delete [] buffer;
}

//----------------------------------------------------------------------
// Ring::Put
// 	将消息放入下一个可用的空槽位中。假设调用者已完成必要的同步操作。
//
// 	参数 "message" —— 要放入缓冲区的消息
//----------------------------------------------------------------------

void
Ring::Put(slot *message)
{
    buffer[in].thread_id = message->thread_id;
    buffer[in].value = message->value;
    in = (in + 1) % size;
}

//----------------------------------------------------------------------
// Ring::Get
// 	从下一个已满的槽位中取出消息。假设调用者已完成必要的同步操作。
//
// 	参数 "message" —— 从缓冲区取出的消息（输出参数）
//----------------------------------------------------------------------

void
Ring::Get(slot *message)
{
    message->thread_id = buffer[out].thread_id;
    message->value = buffer[out].value;
    out = (out + 1) % size;
}

int
Ring::Empty()
{
// to be implemented
}

int
Ring::Full()
{
// to be implemented
}


