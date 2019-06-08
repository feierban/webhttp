#ifndef FANHTTPSERVER_H
#define FANHTTPSERVER_H

#include <QObject>
#include <string>
#include <string.h>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include "../common/mongoose.h"

// 定义http返回callback
typedef void OnRspCallback(mg_connection *c, std::string);
// 定义http请求handler
using ReqHandler = std::function<bool (std::string, std::string, mg_connection *c, OnRspCallback)>;

class FanHttpServerHelper: public QObject
{
    Q_OBJECT
public:
    FanHttpServerHelper(QObject *parent = NULL);
    ~FanHttpServerHelper();

    void emitHandleHttpEvent(mg_connection *connection, http_message *http_req);
    void emitHandleWebsocketMessage(mg_connection *connection, int event_type, websocket_message *ws_msg);
signals:
    void sigHandleHttpEvent(mg_connection *connection, http_message *http_req);
    void sigHandleWebsocketMessage(mg_connection *connection, int event_type, websocket_message *ws_msg);
};

class FanHttpServer : public QObject
{
    Q_OBJECT
public:
    explicit FanHttpServer(const QString addr = "");
    void start();

    void sendHttpRsp(mg_connection *c, std::string rsp);
    void sendWebsocketMsg(mg_connection *connection, std::string msg);
    bool isWebsocket(const mg_connection *connection);
signals:
    bool sigHandleHttpEvent(QString url, QString body, mg_connection *c);

private:
    void HandleHttpEvent(mg_connection *connection, http_message *http_req);
    void HandleWebsocketMessage(mg_connection *connection, int event_type, websocket_message *ws_msg);
    bool routeCheck(http_message *http_msg, char *route_prefix);

    static void OnHttpWebsocketEvent(mg_connection *connection, int event_type, void *event_data);
private:
    QString m_addr;
    mg_serve_http_opts m_serverOption;
    QString m_webDir;
    mg_mgr m_mgr;          // 连接管理器
    static std::unordered_set<mg_connection *> m_websocketSessionSet; // 缓存websocket连接
    static FanHttpServerHelper m_serverHelper;
};

#endif // FANHTTPSERVER_H
