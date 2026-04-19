// prodcons++.cc
//	C++ version of producer and consumer problem using a ring buffer.
//
//	Create N_PROD producer threads and N_CONS consumer thread. 
//	生产者和消费者线程通过一个共享的环形缓冲区对象通信。
//  对共享环形缓冲区的操作通过信号量实现同步。
//	
//      
// Copyright (c) 1995 The Regents of the University of Southern Queensland.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include <unistd.h>   //exit()
#include <fcntl.h>    //creat()
#include <stdlib.h>   //write()

#include <stdio.h>
#include "copyright.h"
#include "system.h"

#include "synch.h"//信号量头文件
#include "ring.h"//环形缓冲区头文件

#define BUFF_SIZE 3  // 环形缓冲区的大小
#define N_PROD    3  // 生产者的数量
#define N_CONS    2  // 消费者的数量
#define N_MESSG   5  // 每个生产者产生的消息数量
#define MAX_NAME  16 // 线程名的最大长度

#define MAXLEN	48 // 消息字符串的最大长度
#define LINELEN	24 // 文件名字符串的最大长度


Thread *producers[N_PROD]; // 指向生产者线程的指针数组
Thread *consumers[N_CONS];  // 指向消费者线程的指针数组

char prod_names[N_PROD][MAX_NAME];  //生产者线程名的字符串数组
char cons_names[N_CONS][MAX_NAME];  //消费者线程名的字符串数组

Semaphore *nempty, *nfull; //two 信号量 for empty and full slots
Semaphore *mutex;          //信号量 for the 互斥访问 of the ring buffer
    
Ring *ring;



//----------------------------------------------------------------------
// Producer
//  循环N_MESSG次，每次生成一条消息并放入共享环形缓冲区。
//  "which"仅作为标识生产者线程的编号。
//      
//----------------------------------------------------------------------

void
Producer(_int which)
{
    int num;
    slot *message = new slot(0,0);// 新建一个消息槽（初始值为0）

//  该循环用于生成N_MESSG条消息并通过ring->Put(message)放入环形缓冲区。
//  每条消息包含一个由整数"num"表示的消息ID，该ID需存入槽的"value"字段。
//  消息还需携带生产者线程的ID（存入"thread_id"字段），以便后续消费者线程
//  知晓该消息由哪个生产者生成。你需要在ring->Put(message)调用前后添加同步代码。

    for (num = 0; num < N_MESSG ; num++) {
      // 在此处编写准备消息的代码
      // ...
      message->value = num;// 设置消息ID
      message->thread_id = which;// 设置生产者线程ID
      // 在此处编写ring->Put(message)前的同步代码
      // ...
      nempty->P(); // 申请空槽位，无空槽则阻塞
      mutex->P(); // 获取互斥锁

      ring->Put(message);// 将消息放入环形缓冲区

      // 在此处编写ring->Put(message)后的同步代码
      // ...
      mutex->V(); // 释放互斥锁
      nfull->V(); // 满槽位 + 1，通知有新消息，唤醒等待的消费者

    }
}

//----------------------------------------------------------------------
// Consumer
// 	无限循环从环形缓冲区获取消息，并将这些消息记录到对应的文件中。
//      
//----------------------------------------------------------------------

void
Consumer(_int which)
{
    char str[MAXLEN];// 用于存储消息字符串的缓冲区
    char fname[LINELEN];// 用于存储输出文件名的缓冲区
    int fd;// 文件描述符，用于写入消息到文件
    
    slot *message = new slot(0,0);// 新建一个消息槽（初始值为0）

    // 为当前消费者线程构造输出文件名
    // 该消费者接收的所有消息都会记录到这个文件中
    // //文件名格式为"tmp_X"，其中X是消费者线程的编号（即参数"which"的值）。
    sprintf(fname, "tmp_%d", which);

    //  创建文件（注：UNIX系统调用）
    if ( (fd = creat(fname, 0600) ) == -1) 
    {
	perror("creat: file create failed");// 如果文件创建失败，输出错误信息并退出
	exit(1);
    }
    
    for (; ; ) {// 无限循环，持续从环形缓冲区获取消息并记录到文件中

      // 在此处编写ring->Get(message)前的同步代码
      // ...
      nfull->P(); // 等待有消息
      mutex->P(); // 获取互斥锁

      ring->Get(message);

      // 在此处编写ring->Get(message)后的同步代码
      // ...
      mutex->V(); // 释放互斥锁
      nempty->V(); // 通知有空槽位


      // 构造记录消息的字符串
      sprintf(str,"producer id --> %d; Message number --> %d;\n", 
		message->thread_id,// 生产者线程ID
		message->value);// 消息ID
      //  将字符串写入当前消费者的输出文件（注：UNIX系统调用）
      if ( write(fd, str, strlen(str)) == -1 ) {
	    perror("write: write failed");// 如果写入文件失败，输出错误信息并退出
	    exit(1);
	  }
    }
}



//----------------------------------------------------------------------
// ProdCons
// 	为共享环形缓冲区初始化信号量，并创建/启动生产者和消费者线程
//----------------------------------------------------------------------

void
ProdCons()
{
    int i;
    DEBUG('t', "Entering ProdCons");

    // 在此处编写构造所有信号量的代码
    // ....
    nempty = new Semaphore("nempty", BUFF_SIZE); // 初始化空槽位信号量
    nfull = new Semaphore("nfull", 0); // 初始化满槽位信号量
    mutex = new Semaphore("mutex", 1); // 初始化互斥锁

    // 在此处编写构造大小为BUFF_SIZE的环形缓冲区对象的代码
    // ...    
    ring = new Ring(BUFF_SIZE); // 创建环形缓冲区对象


    // 创建并启动N_PROD个生产者线程
    for (i=0; i < N_PROD; i++) 
    {
      // 为生产者i构造线程名
      sprintf(prod_names[i], "producer_%d", i);

      // 在此处编写创建并启动新生产者线程的代码：
      //  - 使用prod_names[i]作为线程名
      //  - 将整数i作为Producer函数的参数
      //  ...
      producers[i] = new Thread(prod_names[i]); // 创建生产者线程
      producers[i]->Fork(Producer, i); // 启动生产者线程，传递线程编号作为参数

    };

    // 创建并启动N_CONS个消费者线程
    for (i=0; i < N_CONS; i++) 
    {
      // 为消费者i构造线程名
      sprintf(cons_names[i], "consumer_%d", i);
      // 在此处编写创建并启动新消费者线程的代码：
      //  - 使用cons_names[i]作为线程名
      //  - 将整数i作为Consumer函数的参数
      //  ...
      consumers[i] = new Thread(cons_names[i]); // 创建消费者线程
      consumers[i]->Fork(Consumer, i); // 启动消费者线程，传递线程编号作为参数

    };
}

