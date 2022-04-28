#include <iostream>
#include <string>
#include <ctime>
using std::string;
using std::cout;
using std::endl;

#define INFO 
#define WAGN 
#define ERROR 
#define FATAT

#define log(Level,info) Log(#Level,info,__FILE__,__LINE__)


void Log(string Level,string info,string fileName,int lineNumber)
{
    cout<<"["<<Level<<"]["<<time(nullptr)<<"]["<<fileName<<"]"<<"["<<lineNumber<<"]["<<info<<"]"<<endl;
}
