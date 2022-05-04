#pragma once 

#include <istream>
#include <sys/types.h>
#include <sys/socket.h>

#include "TcpSocket.hpp"
#include "ThreadPool.h"
#include "Log.hpp"
#define PORT 8080
class HttpServer
{
    private:
        int _port;
        //TcpSocket* _tcpServer;
        static HttpServer* _httpServer;
    private:
        HttpServer(int port = PORT)
            :_port(port)
        {
           // _tcpServer = TcpSocket::GetInstance(port);
            signal(SIGPIPE,SIG_IGN);
        }
    public:

        static HttpServer* GetInstance(int port)
        {
            if(_httpServer == nullptr){
                pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;
                pthread_mutex_lock(&mlock);
                if(_httpServer == nullptr){
                    _httpServer = new HttpServer(port);
                }
                pthread_mutex_unlock(&mlock);
            }
            return _httpServer;
        }

        void Run()
        {
            int listSock = TcpSocket::GetInstance(_port)->GetSocket();
            ThreadPool* threadPool = ThreadPool::GetInstance();
            while(true)
            {
                struct sockaddr_in remote;
                socklen_t sz = sizeof(remote);
                int sock = accept(listSock,(sockaddr*)&remote,&sz);
                if(sock < 0){
                    LOG(WARN,"accept error");
                    continue;
                }
                //int* _sock = new int(sock);
                ////创建线程处理任务
                //pthread_t tid;
                //pthread_create(&tid,NULL,Entrance::HandlerRequest,_sock);
                //pthread_detach(tid);
                
                LOG(INFO,"获取链接成功");
                Task* t = new Task(sock);
                threadPool->Push(t);
                LOG(INFO,"塞入ThreadPool");
            }
        }
};
HttpServer*  HttpServer::_httpServer = nullptr;
