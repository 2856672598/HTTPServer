#pragma once 
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include "HttpServer.hpp"
#include "Util.hpp"

using std::vector;
using std::string;
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
            _httpRequest._requestLine.resize(_httpRequest._requestLine.size()-1);
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
                if(tmp == "\n"){
                    _httpRequest._requestBlankLine = tmp;
                    break;
                }
                _httpRequest._requestHeader.push_back(tmp);
                
            }
            log(INFO,"recv request header");
        }

        void ParseHttpRequestLine()
        {
            std::stringstream ss(_httpRequest._requestLine);
            ss>>_httpRequest._requestMethod >> _httpRequest._url>>_httpRequest._version;
            log(INFO,_httpRequest._requestMethod);
            log(INFO,_httpRequest._url);
            log(INFO,_httpRequest._version);
        }

        //解析报头
        void ParseHttpRequestHeader()
        {
            string k,v;
            string delimiter;
            for(const auto& e: _httpRequest._requestHeader)
            {
                if(Utill::CutString(e,delimiter,k,v) == true){
                    _httpRequest._headerKV.insert({k,v});
                    log(INFO,k);
                    log(INFO,v);
                }
                log(INFO,"解析失败");
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
        }

        void ParseHttpRequest()
        {
            ParseHttpRequestLine();
            ParseHttpRequestHeader();
        }
        static void* HandlerRequest(void* sock)
        {
            int socket = *((int*)sock);
            string buff;
            //ssize_t sz = recv(socket,&buff,sizeof(buff),0);
            //Utill::ReadLine(socket,buff);
            //std::cout<<"----------------begin-------------------"<<std::endl;
            //cout<<buff<<endl;
            Entrance* en = new Entrance(socket);
            en->RecvHttpRequest();
            en->ParseHttpRequest();
            //std::cout<<"-------------------endl---------------------"<<std::endl;
        }

};
