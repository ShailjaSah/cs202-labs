#include "TaskQueue.h"

void Printf(char * str){
  puts(str);
  fflush(stdout);
}
TaskQueue::
TaskQueue()
{
  smutex_init(&lock);
  scond_init(&queue_empty);
}

TaskQueue::
~TaskQueue()
{
  Printf("I am Destroying");
  scond_destroy(&queue_empty);
  smutex_destroy(&lock);
}

/*
 * ------------------------------------------------------------------
 * size --
 *
 *      Return the current size of the queue.
 *
 * Results:
 *      The size of the queue.
 *
 * ------------------------------------------------------------------
 */
int TaskQueue::
size()
{
  smutex_lock(&lock);
  int size = dq.size();
  smutex_unlock(&lock);
  return size;
}

/*
 * ------------------------------------------------------------------
 * empty --
 *
 *      Return whether or not the queue is empty.
 *
 * Results:
 *      The true if the queue is empty and false otherwise.
 *
 * ------------------------------------------------------------------
 */
bool TaskQueue::
empty()
{
  smutex_lock(&lock);
  bool empty = dq.empty();
  smutex_unlock(&lock);
  return empty; // Keep compiler happy until routine done.
}

/*
 * ------------------------------------------------------------------
 * enqueue --
 *
 *      Insert the task at the back of the queue.
 *
 * Results:
 *      None.
 *
 * ------------------------------------------------------------------
 */
void TaskQueue::
enqueue(Task task)
{
  smutex_lock(&lock);
  dq.push_back(task);
  scond_signal(&queue_empty, &lock);
  smutex_unlock(&lock);
}

/*
 * ------------------------------------------------------------------
 * dequeue --
 *
 *      Remove the Task at the front of the queue and return it.
 *      If the queue is empty, block until a Task is inserted.
 *
 * Results:
 *      The Task at the front of the queue.
 *
 * ------------------------------------------------------------------
 */
Task TaskQueue::
dequeue()
{
  smutex_lock(&lock);
  while(dq.empty()){
    scond_wait(&queue_empty, &lock);
  }
  Task t = dq.front();
  dq.pop_front();
  smutex_unlock(&lock);
  return t;
}

