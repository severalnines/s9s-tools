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
//#define WARNING
#include "s9sdebug.h"

S9sRpcClientPrivate::S9sRpcClientPrivate() :
    m_referenceCounter(1),
    m_buffer(0),
    m_bufferSize(0),
    m_dataSize(0)
{
}

S9sRpcClientPrivate::~S9sRpcClientPrivate()
{
    clearBuffer();
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

void 
S9sRpcClientPrivate::ensureHasBuffer(
        size_t   size)
{
    if (size <= m_bufferSize)
        return;

    if (m_buffer == NULL)
    {
        m_buffer     = (char *)malloc(size);
        m_bufferSize = size;

        return;
    }

    m_buffer     = (char *) realloc(m_buffer, size);
    m_bufferSize = size;
}

void
S9sRpcClientPrivate::clearBuffer()
{
    if (m_buffer != 0)
        free(m_buffer);

    m_buffer     = 0;
    m_bufferSize = 0;
    m_dataSize   = 0;
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

    if (m_hostName.empty())
    {
        m_errorString = "Controller host name not set.";
        return true;
    }

    PRINT_VERBOSE("Connecting...");

    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd == -1)
    {
        m_errorString.sprintf("Error creating socket: %m");
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
        m_errorString.sprintf("Host '%s' not found.", STR(m_hostName));
        closeSocket(socketFd);
        return -1;
    }

    /*
     * Connecting to the server.
     */
    memcpy((char *) &server.sin_addr, (char *) hp->h_addr, hp->h_length);
    server.sin_family = AF_INET;
    server.sin_port = htons(m_port);

    if (connect(socketFd, (struct sockaddr *) &server, sizeof server) == -1)
    {
        m_errorString.sprintf(
                "Connect to %s:%d failed: %m.", 
                STR(m_hostName), m_port);
      
        closeSocket(socketFd);
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

/**
 * read safely from a socket
 */
ssize_t
S9sRpcClientPrivate::readSocket(
        int     socketFd, 
        char   *buffer, 
        size_t  bufSize)
{
    ssize_t retval = -1;

    do {
        retval = read (socketFd, buffer, bufSize);
    } while (retval == -1 && errno == EINTR);

    return retval;
}
