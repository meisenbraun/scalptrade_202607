#pragma once

class TcpConnection;

struct ConnectionContext
{
    int fd;
    TcpConnection* conn;
};
