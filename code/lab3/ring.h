// ring++.h
//	Data structures for a ring buffer to be used in producer and
//      consumer problem
//
// Copyright (c) 1995 The Regents of the University of Southern Queensland.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.


// 以下代码定义了环形缓冲区类。该类的函数实现位于 ring.cc 文件中。
//
// 环形缓冲区的构造函数（初始化函数）接收一个整数参数，
// 用于指定缓冲区的大小（即槽位数量）。

// 环形缓冲区中单个槽位的类
class slot {
    public:
    slot(int id, int number);
    slot() { thread_id = 0; value = 0;};
    
    int thread_id;
    int value;
    };


class Ring {
  public:
    Ring(int sz);    // Constructor:  initialize variables, allocate space.
    ~Ring();         // Destructor:   deallocate space allocated above.

    
    void Put(slot *message); // 将消息放入下一个空的槽位。
    
    void Get(slot *message); // 从下一个已填充的槽位中取出消息。
                                            
    int Full();       // 若环形缓冲区已满则返回非 0 值，否则返回 0。
    int Empty();      // 若环形缓冲区为空则返回非 0 值，否则返回 0。
    
  private:
    int size;         // The size of the ring buffer.
    int in, out;      // 入/出缓冲区的索引
    slot *buffer;       // 指向环形缓冲区数组的指针。
};


