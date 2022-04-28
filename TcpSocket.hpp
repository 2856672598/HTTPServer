#pragma once 

#include <iostream>
#include <cstdlib>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "Util.hpp"
#define  BACKLOG 5
class TcpSocket
{
    private:
        int _port;
        int _lsockfd;//监听套接字
        static TcpSocket* _instance;

    private:
        TcpSocket(int port)
            :_port(port)
             ,_lsockfd(-1)
        {
            Init();
        }
        TcpSocket(TcpSocket&)=delete;
    public:

        static TcpSocket* GetInstance(int port)
        {
            static pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;
            if(_instance == nullptr){
                pthread_mutex_lock(&mlock);
                if(_instance == nullptr){
                    _instance = new TcpSocket(port);
                    return _instance;
                }
                pthread_mutex_unlock(&mlock);
            }
            return _instance;
        }

        void Init()
        {
            Socket();
            Bind();
            Listen();
        }

        void Socket()
        {
            _lsockfd = socket(AF_INET,SOCK_STREAM,0);
            if(_lsockfd == -1){
                log(FATAL,"socker error");
                exit(1);
            }
        }

        void Bind()
        {
            int flag =1;
            setsockopt(_lsockfd,SOL_SOCKET,SO_REUSEADDR,&flag,sizeof(flag));
            struct sockaddr_in local;
            local.sin_family = AF_INET;
            local.sin_addr.s_addr  = INADDR_ANY;
            local.sin_port = htons(_port);

            if(bind(_lsockfd,(struct sockaddr*)&local,sizeof(local)) == -1){
                log(FATAL,"bind error");
                exit(4);
            }
        }

        void Listen()
        {
            if(listen(_lsockfd,BACKLOG) == -1){
                log(FATAL,"listen error");
                exit(3);
            }
        }

        int GetSocket()
        {
            return _lsockfd;
        }
};

 
TcpSocket* TcpSocket::_instance = nullptr;
