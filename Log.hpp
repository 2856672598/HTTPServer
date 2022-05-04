#pragma once
#include <iostream>
#include <string>
#include <ctime>
using std::string;

#define INFO 
#define WAGN 
#define ERROR 
#define FATAT

#define LOG(Level,info) Log(#Level,info,__FILE__,__LINE__)

void Log(string Level,string info,string fileName,int lineNumber)
{
#ifdef open
    std::cout<<"["<<Level<<"]["<<time(nullptr)<<"]["<<fileName<<"]"<<"["<<lineNumber<<"]["<<info<<"]"<<std::endl;
#endif
}
