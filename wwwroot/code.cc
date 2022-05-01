#include <iostream>
#include <string>
#include <cstdlib>
using namespace std;

int main()
{
    cerr<<"***********************************"<<endl;
    string  method = getenv("REQUESTMETHOD");
    cerr<<"dubug："<<method<<endl;

    if(method == "GET"){
        char* queryString = getenv("QUERY_STRING");
        if(queryString != nullptr){
            cerr<<queryString<<endl;
        }
    }
    else if(method == "POST"){
        cerr<<getenv("CONTENT_LENGTH")<<endl;
    }
    cerr<<"***********************************"<<endl;
    return 0;
}
