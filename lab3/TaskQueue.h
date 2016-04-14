#pragma once


#include "sthread.h"
#include <deque>
#include <cstdio>

typedef void (*handler_t) (void *); 

struct Task {
    handler_t handler;
    void* arg;
};

/*
 * ------------------------------------------------------------------
 * TaskQueue --
 * 
 *      A thread-safe task queue. This queue should be implemented
 *      as a monitor.
 *
 * ------------------------------------------------------------------
 */
class TaskQueue {
    private:
  smutex_t lock;
  scond_t queue_empty;
  std::deque<Task> dq;
    public:
    TaskQueue();
    ~TaskQueue();

    void enqueue(Task task);
    Task dequeue();

    int size();
    bool empty();
};

void Printf(char *);
