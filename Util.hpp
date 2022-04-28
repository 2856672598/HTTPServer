#pragma  once 

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include "Log.hpp"
using std::string;
using std::cout;
using std::endl;
class Utill
{
    public:
        static void ReadLine(int sock,string& buff)
        {
            char ch = 'a';
            while(ch != '\n')
            {
                ssize_t sz = recv(sock,&ch,1,0);
                if(sz > 0){
                    if(ch == '\r'){
                        //判断下一个位置是否为\n
                        //使用窥探读取,我只是看看下一个是什么不拿走
                        ch = recv(sock,&ch,1,MSG_PEEK);
                        if(ch == '\n'){
                            ch = recv(sock,&ch,1,0);
                        }
                        else{
                            //结尾统一用\n
                            ch = '\n';
                        }
                    }
                    log(INTF,buff);
                    buff.push_back(ch);
                }
                else if(sz == 0){
                    //远端关闭
                }
                else {
                    perror("redv");
                    exit(1);
                }
            }
            log(INTF,buff);
        }
        //切割字符串 kOut，vOut输出型参数
        static bool CutString(const string& str,const string& delimiter,string& kOut,string& vOut)
        {

            auto it= str.find(delimiter);
            if(it != string::npos){
                //存在
                kOut = str.substr(0,it);
                vOut = str.substr(it+delimiter.size());
                return true;
            }
            return false;
        }
};
