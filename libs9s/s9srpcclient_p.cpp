/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#include "s9srpcclient_p.h"
 
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include "S9sOptions"

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

S9sRpcClientPrivate::S9sRpcClientPrivate() :
    m_referenceCounter(1)
{
}

S9sRpcClientPrivate::~S9sRpcClientPrivate()
{
}

void 
S9sRpcClientPrivate::ref()
{
	++m_referenceCounter;
}

int 
S9sRpcClientPrivate::unRef()
{
	return --m_referenceCounter;
}

/**
 * \returns The socketFd or -1 on error.
 */
int
S9sRpcClientPrivate::connectSocket()
{
    struct hostent *hp;
    struct timeval timeout;
    struct sockaddr_in server;
    int    socketFd;

    PRINT_VERBOSE("Connecting...");

    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd == -1)
    {
        S9S_WARNING("Error creating socket: %m");
        return -1; 
    }

    /*
     * Limiting the connection.
     * FIXME: Why do we need this?
     */
    timeout.tv_sec  = 60;
    timeout.tv_usec = 0;
    setsockopt (socketFd, SOL_SOCKET, SO_RCVTIMEO,
                (char*) &timeout, sizeof(timeout));
    setsockopt (socketFd, SOL_SOCKET, SO_SNDTIMEO,
                (char*) &timeout, sizeof(timeout));

    /*
     * lookup
     */
    hp = gethostbyname(STR(m_hostName));
    if (hp == NULL)
    {
        S9S_WARNING("Host '%s' not found: %m.", STR(m_hostName));
        //_cmon_rpc_private_socket_close(priv, socketFd);
        return -1;
    }

    /*
     * connect
     */
    memcpy((char *) &server.sin_addr, (char *) hp->h_addr, hp->h_length);
    server.sin_family = AF_INET;
    server.sin_port = htons(m_port);

    if (connect(socketFd, (struct sockaddr *) &server, sizeof server) == -1)
    {
        S9S_WARNING("Connect to %s:%d failed: %m", STR(m_hostName), m_port);
       
        //_cmon_rpc_private_socket_close(priv, socketFd);
        return -1;
    }

    return socketFd;
}

void
S9sRpcClientPrivate::closeSocket(
        int socketFd)
{
    if (socketFd < 0)
        return;

    ::shutdown(socketFd, SHUT_RDWR);
    ::close(socketFd);
}

/**
 * write safely to a socket
 */
ssize_t
S9sRpcClientPrivate::writeSocket(
        int         socketFd, 
        const char *data, 
        size_t      length)
{
    ssize_t retval = -1;

    do {
        retval = ::write(socketFd, data, length);
    } while (retval == -1 && errno == EINTR);

    return retval;
}
