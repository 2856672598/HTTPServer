#pragma once 
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include "HttpServer.hpp"
#include "Util.hpp"
#include <cstring>
using std::vector;
using std::string;

#define DEFAULTPATH "wwwroot"
#define FRONTPAGE "index.html"

class HttpRequest
{
    public:
        string _requestLine;
        vector<string>_requestHeader;//请求头
        string _requestBlankLine;//空行
        string _requestBody;//请求正文

        string _requestMethod;//读取方法
        string _url;
        string _version;

        string _path;
        string _parameter;
        int _contentLength = 0;
        std::unordered_map<string,string>_headerKV;

};

class HttpResponse
{
    public:
        string _statusLine;//状态行
        vector<string>_responseHeader;
        string _responseBlankLine;
        string _respanseBody;
};


class Entrance
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
                return true;
            }
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
                    _httpRequest._headerKV.insert({k,v});
                }
            }
        }
    public:

        Entrance(int sock)
            :_sock(sock)
        {}

        void RecvHttpRequest()
        {
            RecvHttpRequestLine();
            RecvHttpRequestHeader();
            RecvHttpRequestBody();

            ParseHttpRequest();
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
                }
            }
            //补全路径
            string tmp = _httpRequest._path;
            _httpRequest._path = DEFAULTPATH;
            _httpRequest._path += tmp;

            string ret (1,tmp.back());
            log(INFO,tmp);
            if(tmp.size() == 1 && tmp.back() == '/'){
                _httpRequest._path += FRONTPAGE;
            }

            cout<<"URL:"<<_httpRequest._url<<endl;
            cout<<"PATH:"<<_httpRequest._path<<endl;
            cout<<"PARAMETER："<<_httpRequest._parameter<<endl;
        }
        static void* HandlerRequest(void* sock)
        {
            int socket = *((int*)sock);
            std::cout<<"----------------begin-------------------"<<std::endl;
            Entrance* en = new Entrance(socket);
            en->RecvHttpRequest();
            en->MakeHttpResponse();
            std::cout<<"-------------------endl---------------------"<<std::endl;
        }

};
