#pragma once 
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstring>
#include <cstdlib>


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/sendfile.h>

#include "HttpServer.hpp"
#include "Util.hpp"
using std::vector;
using std::string;

#define DEFAULTPATH "wwwroot"
#define FRONTPAGE "index.html"
#define HTTPVERSION "HTTP/1.0"
#define NEWLINE "\r\n"
class HttpRequest
{
    public:
        string _requestLine;
        vector<string>_requestHeader;//请求头
        string _requestBlankLine = "\r\n";//空行
        string _requestBody;//请求正文

        string _requestMethod;//请求方法
        string _url;
        string _version;

        string _path;
        string _parameter;
        int _contentLength = 0;
        std::unordered_map<string,string>_headerKV;
        bool _cgi = false;

};

class HttpResponse
{
    public:
        string _statusLine;//状态行
        vector<string>_responseHeader;
        string _responseBlankLine ="\r\n";
        string _respanseBody;

        int _statusCode = 200;//状态码
        int _size;//发送的正文大小
        int _fd;//要发送是正文的文件描述符
};


class HttpConnect
{
    private:
        int _sock;
        HttpRequest _httpRequest;
        HttpResponse _httpResponse;
    private:
        void RecvHttpRequestLine()
        {
            Utill::ReadLine(_sock,_httpRequest._requestLine);
        }

        void RecvHttpRequestHeader()
        {
            log(INFO,"begin recv request header");
            string tmp;
            while(true)
            {
                tmp.clear();
                Utill::ReadLine(_sock,tmp);
                log(INFO,tmp);
                if(tmp[0] == '\n'){
                    _httpRequest._requestBlankLine = tmp;
                    break;
                }
                _httpRequest._requestHeader.push_back(tmp);
                
            }
            log(INFO,"recv request header");
        }
        //判断是否需要读取请求正文
        bool IsNeedRecvHttpRequestBody()
        {
            const auto& headerKV = _httpRequest._headerKV;
            auto it = headerKV.find("Content-Length");
            if(it != headerKV.end()){
                _httpRequest._contentLength = stoi(it->second);
                log(INFO,"存在MAP中");
                return true;
            }
            log(INFO,"map中未找到");
            return false;
        }

        void RecvHttpRequestBody()
        {
            char ch;
            if(IsNeedRecvHttpRequestBody()){
                for(int i = 0;i < _httpRequest._contentLength;i++)
                {
                    recv(_sock,&ch,1,0);
                    _httpRequest._requestBody.push_back(ch);
                }
            }
            log(INFO,_httpRequest._requestBody);
        }
        
        void ParseHttpRequestLine()
        {
            log(INFO,_httpRequest._requestLine);
            std::stringstream ss(_httpRequest._requestLine);
            ss>>_httpRequest._requestMethod >> _httpRequest._url>>_httpRequest._version;
            log(INFO,_httpRequest._requestMethod);
            log(INFO,_httpRequest._url);
            log(INFO,_httpRequest._version);
        }

        //解析请求报头
        void ParseHttpRequestHeader()
        {
            string k,v;
            string delimiter =": ";
            for(const auto& e: _httpRequest._requestHeader)
            {
                if(Utill::CutString(e,delimiter,k,v) == true){
                    log(INFO,k);
                    log(INFO,v);
                    _httpRequest._headerKV.insert({k,v});
                }
            }
        }
    public:
        HttpConnect(int sock)
            :_sock(sock)
        {}

        ~HttpConnect()
        {
            close(_sock);
        }

        void RecvHttpRequest()
        {
            RecvHttpRequestLine();
            RecvHttpRequestHeader();
            ParseHttpRequest();
            RecvHttpRequestBody();

        }

        void ParseHttpRequest()
        {
            ParseHttpRequestLine();
            ParseHttpRequestHeader();
        }

        void MakeHttpResponse()
        {
            if(_httpRequest._requestMethod !="GET"&&_httpRequest._requestMethod!="POST"){
                return ;
            }
            //如果是get方法判断是否携带参数
            if(_httpRequest._requestMethod == "GET"){
                auto it = _httpRequest._url.find("?");
                _httpRequest._path = _httpRequest._url;
                if(it != string::npos){
                    //解析参数和路径
                    _httpRequest._path.clear();
                    Utill::CutString(_httpRequest._url,"?",_httpRequest._path,_httpRequest._parameter);
                    _httpRequest._cgi = true;
                    log(INFO,"请求中带参数");
                }
                log(INFO,_httpRequest._path);
            }
            else if(_httpRequest._requestMethod == "POST"){
                _httpRequest._cgi = true;
                _httpRequest._path = _httpRequest._url;
                log(INFO,"POST请求");
            }
            else{
            }
            //补全路径
            string tmp = _httpRequest._path;
            _httpRequest._path = DEFAULTPATH;
            _httpRequest._path += tmp;


            //访问的是一个目录
            if(_httpRequest._path[_httpRequest._path.size()-1] == '/'){
                _httpRequest._path += FRONTPAGE;
            }

            //判断请求的路径是否存在
            struct stat buf;
            log(INFO,_httpRequest._path);
            if(stat(_httpRequest._path.c_str(),&buf) == 0){
                //判断请求的是一个目录还是文件
                if(S_ISDIR(buf.st_mode)){
                    //当访问的是目录时访问目录首页
                    _httpRequest._path+='/';
                    _httpRequest._path+=FRONTPAGE;
                    stat(_httpRequest._path.c_str(),&buf);
                }
                if(buf.st_mode&S_IXGRP || buf.st_mode&S_IXOTH || buf.st_mode&S_IXUSR){
                    //判断请求的资源是否是一个可执行程序
                    _httpRequest._cgi = true;
                    log(INFO,"请求的是一个可执行程序");
                }
            }
            else{
                //访问的路径不存在
                log(WARN,"NOT FOUNT");
                goto END;
            }
            if(_httpRequest._cgi == true){
                log(INFO,"PROCCES CGI");
                ProccesCgi();
            }
            else{
                log(INFO,_httpRequest._path+"noCGI");
                ProccesNonCgi(buf.st_size);
            }
END:
            return ;
        }

        int ProccesCgi()
        {
            log(INFO,"begin Proccss Cgi");
            //通过创建子进程，让子进程进行程序替换，完成请求的任务
            //程序替换的是 代码的数据和代码，所以在替换后父进程继承的数据就被清空了--就需要用管道完成通信
            //以父进程的角度
            int readArr[2];
            int writeArr[2];
            const string& requestMethod = _httpRequest._requestMethod;

            if(pipe(readArr) == -1){
                //创建管道失败
                log(WARN,"create pipe error");
                return 404;
            }
            if(pipe(writeArr) == -1){
                //创建管道失败
                log(WARN,"create pipe error");
                return 404;
            }
            pid_t pid = fork();
            if(pid == 0){
                string contentLengthEnv = "CONTENT_LENGTH=";
                string  requestMethodEnv = "REQUESTMETHOD=";
                requestMethodEnv += _httpRequest._requestMethod;
                putenv((char*)requestMethodEnv.c_str());

                close(readArr[0]);
                close(writeArr[1]);

                std::cerr<<requestMethod<<endl;
                if(requestMethod == "GET"){
                    string queryString = "QUERY_STRING=";
                    queryString += _httpRequest._parameter;
                    putenv((char*)queryString.c_str());
                    std::cerr<<queryString<<endl;
                }
                else if(requestMethod == "POST"){
                    contentLengthEnv += std::to_string(_httpRequest._contentLength);
                    if(putenv((char*)contentLengthEnv.c_str()) != 0){
                        log(WARN,"putenv error");
                    }
                    std::cerr<<contentLengthEnv<<endl;
                }
                //读端
                dup2(writeArr[0],0);
                //写端
                dup2(readArr[1],1);
                log(INFO,requestMethod);
               execl(_httpRequest._path.c_str(),_httpRequest._path.c_str(),nullptr);
               std::cerr<<_httpRequest._path.c_str()<<endl;
               std::cerr<<"程序替换失败"<<endl;
            }
            else if(pid > 0){
                close(readArr[1]);
                close(writeArr[0]);
                if(requestMethod == "POST"){
                    int size = _httpRequest._requestBody.size();
                    int begin = 0;
                    const char* buff = _httpRequest._requestBody.c_str();
                    ssize_t sz = 0;
                    while((sz = write(writeArr[1],(char*)buff + begin,size - begin))>0)
                    {
                        begin+=sz;
                    }
                }
                waitpid(pid,nullptr,0);
            }
            else{
                //创建子进程失败
            }
            log(INFO,"exit CGI");
            return 200;
        }

        void ProccesNonCgi(int size)
        {
            int fd = open(_httpRequest._path.c_str(),O_RDONLY);
            if(fd < 0){
                log(WARN,"open error");
                return;
            }
            //构建响应
            _httpResponse._statusLine = HTTPVERSION;
            _httpResponse._statusLine += " ";
            _httpResponse._statusLine += std::to_string(_httpResponse._statusCode);
            _httpResponse._statusLine += " ";
            _httpResponse._statusLine += "OK";//待修改
            _httpResponse._statusLine += NEWLINE;
            _httpResponse._size = size;
            _httpResponse._fd = fd;
            log(INFO,_httpResponse._statusLine);
        }

        void SentHttpProcces()
        {
            log(INFO,std::to_string(_sock));
            send(_sock,_httpResponse._statusLine.c_str(),_httpResponse._statusLine.size(),0);
            for(const auto& e:_httpResponse._responseHeader)
            {
                send(_sock,e.c_str(),e.size(),0);
            }
            send(_sock,_httpResponse._responseBlankLine.c_str(),_httpResponse._responseBlankLine.size(),0);
            sendfile(_sock,_httpResponse._fd,NULL,_httpResponse._size);
            close(_httpResponse._fd);
           log(INFO,std::to_string(_sock)); 
        }
};



class Entrance
{
    public:
        static void* HandlerRequest(void* sock)
        {
            int socket = *((int*)sock);
            log(INFO,std::to_string(socket));
            HttpConnect* en = new HttpConnect(socket);
            en->RecvHttpRequest();
            en->MakeHttpResponse();
            en->SentHttpProcces();
            log(INFO,"请求处理完毕");
            delete en;
            delete (int*)sock;
        }

};
