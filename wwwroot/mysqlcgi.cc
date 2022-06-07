#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include "mysql.h"
using namespace std;

bool CutString(const string& str,const string& delimiter,string& kOut,string& vOut)
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

bool GetQueryString(string& queryString)
{
    bool flag = true;
    queryString.clear();
    string  method = getenv("REQUESTMETHOD");
    if(method == "GET"){
        queryString = getenv("QUERY_STRING");
    }
    else if(method == "POST"){
        int length =atoi(getenv("CONTENT_LENGTH"));
        while(length--)
        {
            char ch;
            read(0,&ch,1);
            queryString.push_back(ch);
        }
    }
    else 
        flag = false;
    return flag;
}
void Insert(string& sql)
{
    MYSQL* conn = mysql_init(nullptr);
    if(NULL==mysql_real_connect(conn,"127.0.0.1","root",nullptr,"http",3306,nullptr,0)){
        return;
    }

    mysql_query(conn,sql.c_str()); 

    mysql_close(conn);

}

int main()
{
    string queryString;
    GetQueryString(queryString);
    cout<<queryString<<endl;
    string str1,str2;
    //先按照&进行分隔
    CutString(queryString,"&",str1,str2);
    string name,password,tmp;
    CutString(str1,"=",tmp,name);
    CutString(str2,"=",tmp,password);
    string sql="insert into user (name,password) values(\'";
    sql+=name;
    sql+="\',\'";
    sql+=password;
    sql+="\')";
    Insert(sql);
    return 0;
}
