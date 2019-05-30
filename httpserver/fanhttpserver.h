#ifndef FANHTTPSERVER_H
#define FANHTTPSERVER_H

#include <QObject>
#include <string>
#include <string.h>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include "../common/mongoose.h"

class FanHttpServer : public QObject
{
    Q_OBJECT
public:
    explicit FanHttpServer(const QString addr = "");
    void start();
signals:

private:
    void HandleHttpEvent(mg_connection *connection, http_message *http_req);
    void HandleWebsocketMessage(mg_connection *connection, int event_type, websocket_message *ws_msg);

private slots:
    void OnHttpWebsocketEvent(mg_connection *connection, int event_type, void *event_data);
private:
    QString m_addr;
    mg_serve_http_opts m_serverOption;
    mg_mgr m_mgr;          // 连接管理器
};

#endif // FANHTTPSERVER_H
