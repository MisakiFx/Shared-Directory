#pragma once
#include <iostream>
#include <sstream>
#include <thread>
#include <queue>
#include <vector>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define MAX_THREAD  5
#define MAX_QUEUE 10

typedef void (*handler_t)(int);

/******************************************************/
//任务类
class ThreadTask 
{

  private:
    int _data;              //参数
    handler_t _handler;     //任务

  public:
    //构造
    ThreadTask(int data, handler_t handle);

    //设置任务
    void SetTask(int data, handler_t handle);
    
    //执行任务
    void TaskRun();
};
/*********************************************************/

/*********************************************************/
//线程池类
class ThreadPool
{

  private:
    std::queue<ThreadTask> _queue;  //任务队列
    size_t _capacity;               //线程池容量，可以进入的最大任务数
    pthread_mutex_t _mutex;         //互斥锁
    pthread_cond_t _cond_pro;       //生产者条件变量
    pthread_cond_t _cond_con;       //消费者条件变量
    int _thr_max;                   //用于控制线程的最大数量

  private:
    //线程入口函数
    void thr_start();

  public: 
    //构造函数
    ThreadPool (int maxq = MAX_QUEUE, int maxt = MAX_THREAD);

    //析构函数
    ~ThreadPool();

    //初始化线程池
    bool PoolInit();

    //添加任务
    bool TaskPush(ThreadTask &tt);
};
/**********************************************************/

ThreadTask::ThreadTask(int data, handler_t handle)
  :_data(data)
   ,_handler(handle) 
{

}
ThreadPool::~ThreadPool() 
{
  pthread_mutex_destroy(&_mutex);
  pthread_cond_destroy(&_cond_con);
  pthread_cond_destroy(&_cond_pro);
}
ThreadPool::ThreadPool(int maxq, int maxt)
  :_capacity(maxq)
   , _thr_max(maxt)
{
  pthread_mutex_init(&_mutex, NULL);
  pthread_cond_init(&_cond_con, NULL);
  pthread_cond_init(&_cond_pro, NULL);
}
void ThreadPool::thr_start()
{
  while(1) 
  {
    pthread_mutex_lock(&_mutex);
    while(_queue.empty()) 
    {
      pthread_cond_wait(&_cond_con, &_mutex);
    }
    ThreadTask tt = _queue.front();
    _queue.pop();
    pthread_mutex_unlock(&_mutex);
    pthread_cond_signal(&_cond_pro);
    tt.TaskRun();
  }
  return;
}
bool ThreadPool::PoolInit() 
{
  for (int i = 0; i < _thr_max; i++) 
  {
    std::thread thr(&ThreadPool::thr_start, this);
    thr.detach();
  }
  return true;
}
bool ThreadPool::TaskPush(ThreadTask &tt) 
{
  pthread_mutex_lock(&_mutex);
  while(_queue.size() == _capacity) 
  {
    pthread_cond_wait(&_cond_pro, &_mutex);
  }
  _queue.push(tt);
  pthread_mutex_unlock(&_mutex);
  pthread_cond_signal(&_cond_con);
  return true;
}
void ThreadTask::SetTask(int data, handler_t handle) 
{ 
  _data = data; 
  _handler = handle; 
  return;
}
void ThreadTask::TaskRun() 
{   
  return _handler(_data);  
}
