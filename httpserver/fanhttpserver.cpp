#include <utility>
#include "QDebug"
#include "fanhttpserver.h"

FanHttpServerHelper FanHttpServer::m_serverHelper;
std::unordered_set<mg_connection *> FanHttpServer::m_websocketSessionSet;
FanHttpServerHelper::FanHttpServerHelper(QObject *parent)
{
}

FanHttpServerHelper::~FanHttpServerHelper()
{
}

void FanHttpServerHelper::emitHandleHttpEvent(mg_connection *connection, http_message *http_req)
{
    emit sigHandleHttpEvent(connection, http_req);
}

void FanHttpServerHelper::emitHandleWebsocketMessage(mg_connection *connection, int event_type, websocket_message *ws_msg)
{
    emit sigHandleWebsocketMessage(connection, event_type, ws_msg);
}

FanHttpServer::FanHttpServer(const QString addr)
    :m_addr(addr)
{
    m_webDir = QString();
    connect(&m_serverHelper, &FanHttpServerHelper::sigHandleHttpEvent,
            this, &FanHttpServer::HandleHttpEvent);
    connect(&m_serverHelper, &FanHttpServerHelper::sigHandleWebsocketMessage,
            this, &FanHttpServer::HandleWebsocketMessage);

    m_serverOption.enable_directory_listing = "no";
    m_serverOption.document_root = m_webDir.toLatin1().data();

    // 其他http设置

    // 开启 CORS，本项只针对主页加载有效
    // s_server_option.extra_headers = "Access-Control-Allow-Origin: *";
}

void FanHttpServer::start()
{
    mg_mgr_init(&m_mgr, NULL);
    mg_connection *connection = mg_bind(&m_mgr, m_addr.toLatin1().data(), &FanHttpServer::OnHttpWebsocketEvent);
    if (connection == NULL)
        return;
    // for both http and websocket
    mg_set_protocol_http_websocket(connection);

    qDebug() << "starting http server at port: " << m_addr;

    // loop
    while (true)
        mg_mgr_poll(&m_mgr, 500); // ms

    return;
}

void FanHttpServer::sendHttpRsp(mg_connection *connection, std::string rsp)
{
    // --- 未开启CORS
    // 必须先发送header, 暂时还不能用HTTP/2.0
    mg_printf(connection, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
    // 以json形式返回
    mg_printf_http_chunk(connection, "{ \"result\": %s }", rsp.c_str());
    // 发送空白字符快，结束当前响应
    mg_send_http_chunk(connection, "", 0);

    // --- 开启CORS
    /*mg_printf(connection, "HTTP/1.1 200 OK\r\n"
              "Content-Type: text/plain\n"
              "Cache-Control: no-cache\n"
              "Content-Length: %d\n"
              "Access-Control-Allow-Origin: *\n\n"
              "%s\n", rsp.length(), rsp.c_str()); */
}

void FanHttpServer::sendWebsocketMsg(mg_connection *connection, std::string msg)
{
    mg_send_websocket_frame(connection, WEBSOCKET_OP_TEXT, msg.c_str(), strlen(msg.c_str()));
}

bool FanHttpServer::isWebsocket(const mg_connection *connection)
{
    return connection->flags & MG_F_IS_WEBSOCKET;
}

void FanHttpServer::HandleHttpEvent(mg_connection *connection, http_message *http_req)
{
    QString req_str = QString(http_req->message.p).mid(http_req->message.len);
    qDebug() << "got request: " << req_str;

    // 先过滤是否已注册的函数回调
    QString url = QString(http_req->uri.p).mid(http_req->uri.len);
    QString body = QString(http_req->body.p).mid(http_req->body.len);

    emit sigHandleHttpEvent(url, body, connection);

    // 其他请求
    if (routeCheck(http_req, "/")) // index page
        mg_serve_http(connection, http_req, m_serverOption);
    else if (routeCheck(http_req, "/api/hello"))
    {
        // 直接回传
        sendHttpRsp(connection, "welcome to httpserver");
    }
    else if (routeCheck(http_req, "/api/sum"))
    {
        // 简单post请求，加法运算测试
        char n1[100], n2[100];
        double result;

        /* Get form variables */
        mg_get_http_var(&http_req->body, "n1", n1, sizeof(n1));
        mg_get_http_var(&http_req->body, "n2", n2, sizeof(n2));

        /* Compute the result and send it back as a JSON object */
        result = strtod(n1, NULL) + strtod(n2, NULL);
        sendHttpRsp(connection, std::to_string(result));
    }
    else
    {
        mg_printf(
            connection,
            "%s",
            "HTTP/1.1 501 Not Implemented\r\n"
            "Content-Length: 0\r\n\r\n");
    }
}

void FanHttpServer::HandleWebsocketMessage(mg_connection *connection, int event_type, websocket_message *ws_msg)
{
    if (event_type == MG_EV_WEBSOCKET_HANDSHAKE_DONE) {
        printf("client websocket connected\n");
        // 获取连接客户端的IP和端口
        char addr[32];
        mg_sock_addr_to_str(&connection->sa, addr, sizeof(addr), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
        printf("client addr: %s\n", addr);

        // 添加 session
        m_websocketSessionSet.insert(connection);

        sendWebsocketMsg(connection, "client websocket connected");
    } else if (event_type == MG_EV_WEBSOCKET_FRAME) {
        mg_str received_msg = {
            (char *)ws_msg->data, ws_msg->size
        };

        char buff[1024] = {0};
        strncpy(buff, received_msg.p, received_msg.len); // must use strncpy, specifiy memory pointer and length

        // do sth to process request
        printf("received msg: %s\n", buff);
        sendWebsocketMsg(connection, "send your msg back: " + std::string(buff));
        //BroadcastWebsocketMsg("broadcast msg: " + std::string(buff));
    } else if (event_type == MG_EV_CLOSE) {
        if (isWebsocket(connection)) {
            printf("client websocket closed\n");
            // 移除session
            if (m_websocketSessionSet.find(connection) != m_websocketSessionSet.end())
                m_websocketSessionSet.erase(connection);
        }
    }
}

bool FanHttpServer::routeCheck(http_message *http_msg, char *route_prefix)
{
    if (mg_vcmp(&http_msg->uri, route_prefix) == 0)
        return true;

    return false;

    // TODO: 还可以判断 GET, POST, PUT, DELTE等方法
    //mg_vcmp(&http_msg->method, "GET");
    //mg_vcmp(&http_msg->method, "POST");
    //mg_vcmp(&http_msg->method, "PUT");
    //mg_vcmp(&http_msg->method, "DELETE");
}

void FanHttpServer::OnHttpWebsocketEvent(mg_connection *connection, int event_type, void *event_data)
{
    // 区分http和websocket
    if (event_type == MG_EV_HTTP_REQUEST)
    {
        http_message *http_req = (http_message *)event_data;
        m_serverHelper.emitHandleHttpEvent(connection, http_req);
    }
    else if (event_type == MG_EV_WEBSOCKET_HANDSHAKE_DONE ||
             event_type == MG_EV_WEBSOCKET_FRAME ||
             event_type == MG_EV_CLOSE)
    {
        websocket_message *ws_message = (struct websocket_message *)event_data;
        m_serverHelper.emitHandleWebsocketMessage(connection, event_type, ws_message);
    }
}

