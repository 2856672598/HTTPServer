#pragma once
#include <queue>
#include "Protocol.h"
#include <pthread.h>

class Task
{
    private:
        int _sock;
        void* (*_handler)(int);
    public:
        Task(int sock)
            :_sock(sock),_handler(Entrance::HandlerRequest)
        {}
        ~Task()
        {
            close(_sock);
        }
        void Run()
        {
            _handler(_sock);
        }
};

class ThreadPool
{
    private:
        int _num;
        static ThreadPool* _instance;
        pthread_mutex_t _mlock;
        pthread_cond_t _cond;
        std::queue<Task*>_queue;
    private:
        ThreadPool(int num = 10)
            :_num(num)
        {
            pthread_mutex_init(&_mlock,nullptr);
            pthread_cond_init(&_cond,nullptr);
        }
        ThreadPool(const ThreadPool&)=delete;

        void Init()
        {
            for(int i =0 ;i < _num;i++)
            {
                pthread_t tid;
                if(pthread_create(&tid,nullptr,Work,this) != 0){

                }
            }
            LOG(INFO,"ThreadPool Started Success");
        }

        void Lock()
        {
            pthread_mutex_lock(&_mlock);
        }

        void Unlock()
        {
            pthread_mutex_unlock(&_mlock);
        }

        Task* Pop()
        {
            Task* task = _queue.front();
            _queue.pop();
            return task;
        }
        bool IsEmpty()
        {
            return _queue.empty();
        }
    public:
        ~ThreadPool()
        {
            pthread_mutex_destroy(&_mlock);
            pthread_cond_destroy(&_cond);
        }

        static ThreadPool* GetInstance()
        {
            static pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;
            if(_instance == nullptr){
                pthread_mutex_lock(&mlock);
                if(_instance == nullptr){
                    _instance = new ThreadPool();
                    _instance->Init();
                }
                pthread_mutex_unlock(&mlock);
            }
            return _instance;
        }
        static void* Work(void* obj)
        {
            ThreadPool* pool = (ThreadPool*)obj;
            while(true)
            {
                pool->Lock();
                while(pool->IsEmpty())
                {
                    pthread_cond_wait(&pool->_cond,&pool->_mlock);
                }
                //从队列中取出一个任务
                Task* task = pool->Pop();
                pool->Unlock();
                LOG(INFO,"获取到任务");
                task->Run();
                delete task;
            }
            return nullptr;
        }

        void Push(Task* task)
        {
            Lock();
            _queue.push(task);
            Unlock();
            pthread_cond_signal(&_cond);
        }
};
ThreadPool* ThreadPool::_instance = nullptr;
