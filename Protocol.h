#pragma once 
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <algorithm>
#include <cstring>
#include <cstdlib>


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <pthread.h>
#include <string.h>

#include "HttpServer.hpp"
#include "Util.hpp"
using std::vector;
using std::string;

#define DEFAULTPATH "wwwroot"
#define FRONTPAGE "index.html"
#define HTTPVERSION "HTTP/1.0"
#define NEWLINE "\r\n"
#define ERRORPAGE "wwwroot/index.html"
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

        string _contentType = "text/html";
        int _statusCode = 200;//状态码
        int _size;//发送的正文大小
        int _fd;//要发送是正文的文件描述符
};

static string CodetoDesc(const int code)
{
    switch(code)
    {
        case 200:
            return "OK";
            break;
        case 400:
            return "Bad Request";
            break;
        case 404:
            return "Not Found";
            break;
        case 500:
            return "Internal Server Error";
            break;
        default:
            break;
    }
}


class ContentType
{
    private:
        std::unordered_map<string,string>_type;
        static ContentType* _instance;
        ContentType()
        {
            _type.insert({".html","text/html"});
            _type.insert({".jpg","application/x-jpg"});
            _type.insert({".css","text/css"});
        }
        ContentType(const ContentType&)= delete;
    public:
        static ContentType* GetInstance()
        {
            if(_instance == nullptr){
                pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;
                pthread_mutex_lock(&mlock);
                if(_instance == nullptr){
                    _instance = new ContentType();
                }
                pthread_mutex_unlock(&mlock);
            }
            return _instance;
        }

        string SuffixToType(const string& suffix)
        {
            auto it = _type.find(suffix);
            if(it!=_type.end()){
                return it->second;
            }
            return {};
        }
};

ContentType* ContentType::_instance = nullptr;

class HttpConnect
{
    private:
        bool _stop;//用于判断程序是否异常
        int _sock;
        HttpRequest _httpRequest;
        HttpResponse _httpResponse;
    private:
        bool RecvHttpRequestLine()
        {
            if( Utill::ReadLine(_sock,_httpRequest._requestLine) <= 0)
                _stop = true;
            return _stop;
        }

        bool RecvHttpRequestHeader()
        {
            LOG(INFO,"begin recv request header");
            string tmp;
            while(true)
            {
                tmp.clear();
                if(Utill::ReadLine(_sock,tmp) <=0){
                    _stop = true;
                    break;
                }
                if(tmp[0] == '\n'){
                    _httpRequest._requestBlankLine = tmp;
                    break;
                }
                _httpRequest._requestHeader.push_back(tmp); 
            }
            return _stop;
        }
        //判断是否需要读取请求正文
        bool IsNeedRecvHttpRequestBody()
        {
            const auto& headerKV = _httpRequest._headerKV;
            auto it = headerKV.find("Content-Length");
            if(it != headerKV.end()){
                _httpRequest._contentLength = stoi(it->second);
                LOG(INFO,"ContentLength:"+ it->second);
                return true;
            }
            LOG(INFO,"map中未找到");
            return false;
        }

        bool RecvHttpRequestBody()
        {
            char ch;
            if(IsNeedRecvHttpRequestBody()){
                for(int i = 0;i < _httpRequest._contentLength;i++)
                {
                    if(recv(_sock,&ch,1,0 )<=0){
                        LOG(WARN,"rcv 读取错误");
                        _stop = true;
                        break;
                    }
                    _httpRequest._requestBody.push_back(ch);
                }
            }
            LOG(INFO,_httpRequest._requestBody);
            return _stop;
        }
        
        void ParseHttpRequestLine()
        {
            LOG(INFO,_httpRequest._requestLine);
            std::stringstream ss(_httpRequest._requestLine);
            ss>>_httpRequest._requestMethod >> _httpRequest._url>>_httpRequest._version;
            std::transform(_httpRequest._requestMethod.begin(),_httpRequest._requestMethod.end(),_httpRequest._requestMethod.begin(),::toupper);
            LOG(INFO,_httpRequest._requestMethod);
            LOG(INFO,_httpRequest._url);
            LOG(INFO,_httpRequest._version);
        }

        //解析请求报头
        void ParseHttpRequestHeader()
        {
            string k,v;
            string delimiter =": ";
            for(const auto& e: _httpRequest._requestHeader)
            {
                if(Utill::CutString(e,delimiter,k,v) == true){
                    LOG(INFO,k);
                    LOG(INFO,v);
                    _httpRequest._headerKV.insert({k,v});
                }
            }
        }

        void _MakeHttpResponse()
        {
            //状态行
            string statusLine = HTTPVERSION;
            statusLine += " ";
            statusLine += std::to_string(_httpResponse._statusCode);
            statusLine += " ";
            statusLine += CodetoDesc(_httpResponse._statusCode);
            statusLine += NEWLINE;
            _httpResponse._statusLine = statusLine;
            int code = _httpResponse._statusCode;
            switch(code)
            {
                case 200:
                    MakeOkResponse();
                    break;
                case 404:
                    MakeErrorRespose(ERRORPAGE);
                    break;
                default:
                    break;
            }
        }

        void MakeOkResponse()
        {
            // cgi 和静态页面
            string head = "Content-Length: ";
            if(_httpRequest._cgi){
                head += std::to_string(_httpResponse._respanseBody.size());
            }
            else{
                head +=std::to_string(_httpResponse._size);
            }
            head +=  NEWLINE;

            _httpResponse._responseHeader.push_back(head);
            head = "Content-Type: ";
            head += _httpResponse._contentType;
            head += NEWLINE;
            _httpResponse._responseHeader.push_back(head);
        }

        void MakeErrorRespose(const std::string& fileName)
        {
            _httpRequest._cgi = false;
            int fd = open(fileName.c_str(),O_RDONLY);
            if(fd < 0){
                LOG(WARN,fileName+"打开失败");
                _httpResponse._statusCode = 500;
                return ;
            }

            struct stat buff;
            if(stat(fileName.c_str(),&buff) == -1){
                LOG(WARN,fileName+"stat error");
                _httpResponse._statusCode = 500;
                return;
            }
            _httpResponse._fd = fd;
            _httpResponse._size = buff.st_size;
            string head = "Content-Length: ";
            head += std::to_string(buff.st_size);
            head += NEWLINE;
            _httpResponse._responseHeader.push_back(head);
            head = "Content-Type: ";
            head += "text/html";
            head += NEWLINE;
            _httpResponse._responseHeader.push_back(head);
        }

        void ProccesCgi()
        {
            LOG(INFO,"begin Proccss Cgi");
            //通过创建子进程，让子进程进行程序替换，完成请求的任务
            //程序替换的是 代码的数据和代码，所以在替换后父进程继承的数据就被清空了--就需要用管道完成通信
            //以父进程的角度
            int readArr[2];
            int writeArr[2];
            const string& requestMethod = _httpRequest._requestMethod;

            if(pipe(readArr) == -1 || pipe(writeArr)== -1){
                //创建管道失败
                LOG(WARN,"create pipe error");
                _httpResponse._statusCode = 500;
                return ;
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
                    LOG(INFO,queryString);
                }
                else if(requestMethod == "POST"){
                    contentLengthEnv += std::to_string(_httpRequest._contentLength);
                    if(putenv((char*)contentLengthEnv.c_str()) != 0){
                        LOG(WARN,"putenv error");
                    }
                    std::cerr<<contentLengthEnv<<endl;
                }
                //读端
                dup2(writeArr[0],0);
                //写端
                dup2(readArr[1],1);
               execl(_httpRequest._path.c_str(),_httpRequest._path.c_str(),nullptr);
               std::cerr<<"程序替换失败"<<endl;
               exit(-1);
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
                int status = 0;
                waitpid(-1,&status,0);
                if(WIFEXITED(status)){
                    if(WEXITSTATUS(status)){
                        LOG(INFO,"子进程执行错误");
                        _httpResponse._statusCode = 400;
                    }
                    else{
                        //从管道中去读取数据
                        char ch;
                        while(read(readArr[0],&ch,1) > 0)
                        {
                            _httpResponse._respanseBody.push_back(ch);
                        }
                    }
                }
                else{
                    _httpResponse._statusCode = 500;
                }
            }
            else{
                _httpResponse._statusCode = 500;
            }
            close(readArr[0]);
            close(writeArr[1]);
        }

        void ProccesNonCgi()
        {
            int fd = open(_httpRequest._path.c_str(),O_RDONLY);
            if(fd < 0){
                LOG(WARN,"open error");
                _httpResponse._statusCode = 500;
                return;
            }
            _httpResponse._fd = fd;
        }

    public:
        HttpConnect(int sock)
            :_stop(false),_sock(sock)
        {}

        ~HttpConnect()
        {
            close(_sock);
        }

        bool IsStop()
        {
            return _stop;
        }

        void RecvHttpRequest()
        {
            if(!RecvHttpRequestLine()&&!RecvHttpRequestHeader()){
                ParseHttpRequest();
                RecvHttpRequestBody();
            }
        }

        void ParseHttpRequest()
        {
            ParseHttpRequestLine();
            ParseHttpRequestHeader();
        }

        void MakeHttpResponse()
        {
            string tmp;
            string suffix;
            size_t pos;
            if(_httpRequest._requestMethod !="GET"&&_httpRequest._requestMethod!="POST"){
                _httpResponse._statusCode = 400;
                goto END;
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
                    LOG(INFO,"请求中带参数");
                }
                LOG(INFO,_httpRequest._path);
            }
            else if(_httpRequest._requestMethod == "POST"){
                _httpRequest._cgi = true;
                _httpRequest._path = _httpRequest._url;
                LOG(INFO,"POST请求");
            }
            else{
            }
            //补全路径
            tmp = _httpRequest._path;
            _httpRequest._path = DEFAULTPATH;
            _httpRequest._path += tmp;


            if(_httpRequest._path[_httpRequest._path.size()-1] == '/'){
                _httpRequest._path += FRONTPAGE;
            }

            //判断请求的路径是否存在
            struct stat buf;
            LOG(INFO,_httpRequest._path);
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
                    LOG(INFO,"请求的是一个可执行程序");
                }
            }
            else{
                //访问的路径不存在
                LOG(WARN,"NOT FOUNT");
                _httpResponse._statusCode = 404;
                goto END;
            }

            pos = _httpRequest._path.rfind(".");
            if(pos != string::npos){
                suffix = _httpRequest._path.substr(pos);
                _httpResponse._contentType = ContentType::GetInstance()->SuffixToType(suffix);
            }

            if(_httpRequest._cgi == true){
                LOG(INFO,"PROCCES CGI");
                ProccesCgi();
            }
            else{
                _httpResponse._size = buf.st_size;
                LOG(INFO,_httpRequest._path+"noCGI");
                ProccesNonCgi();
            }
END:
            LOG(INFO,std::to_string(_httpResponse._statusCode));
            //响应处理
            _MakeHttpResponse();
            return ;
        }

        void SentHttpProcces()
        {
            LOG(INFO,std::to_string(_sock));
            send(_sock,_httpResponse._statusLine.c_str(),_httpResponse._statusLine.size(),0);
            for(const auto& e:_httpResponse._responseHeader)
            {
                send(_sock,e.c_str(),e.size(),0);
                LOG(INFO,e);
            }
            send(_sock,_httpResponse._responseBlankLine.c_str(),_httpResponse._responseBlankLine.size(),0);

            if(_httpRequest._cgi){
                int begin = 0;
                int finish =0;
                int size = _httpResponse._respanseBody.size();
                const char* buff = _httpResponse._respanseBody.c_str();

                while(begin < size && (finish = send(_sock,buff+begin,size - begin,0)) > 0)
                {
                    begin+=finish;
                }   
            }
            else{
                LOG(INFO,"发送正文");
                LOG(INFO,std::to_string(_sock)); 
                sendfile(_sock,_httpResponse._fd,NULL,_httpResponse._size);
                close(_httpResponse._fd);
            }
        }
};

class Entrance
{
    public:
        static void* HandlerRequest(int sock)
        {
            std::shared_ptr<HttpConnect>en (new HttpConnect(sock));
            en->RecvHttpRequest();
            if(!en->IsStop()){
                en->MakeHttpResponse();
                en->SentHttpProcces();
            }else{
                LOG(INFO,"异常");
            }
        }
};
