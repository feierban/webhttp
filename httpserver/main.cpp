#include <QCoreApplication>
#include <iostream>
#include <memory>
#include "http_server.h"

// 初始化HttpServer静态类成员

bool handle_fun1(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback)
{
    // do sth
    std::cout << "handle fun1" << std::endl;
    std::cout << "url: " << url << std::endl;
    std::cout << "body: " << body << std::endl;

    rsp_callback(c, "rsp1");

    return true;
}

bool handle_fun2(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback)
{
    // do sth
    std::cout << "handle fun2" << std::endl;
    std::cout << "url: " << url << std::endl;
    std::cout << "body: " << body << std::endl;

    rsp_callback(c, "rsp2");

    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    std::string port = "7999";
    auto http_server = std::shared_ptr<HttpServer>(new HttpServer);
    http_server->Init(port);
    // add handler
    http_server->AddHandler("/api/fun1", handle_fun1);
    http_server->AddHandler("/api/fun2", handle_fun2);
    http_server->Start();

    return a.exec();
}
