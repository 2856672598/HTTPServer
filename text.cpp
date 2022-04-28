#include "HttpServer.hpp"
int main()
{
    HttpServer* server = HttpServer::GetInstance(8081);
    server->Run();

    return 0;
}
