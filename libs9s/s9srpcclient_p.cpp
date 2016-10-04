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
    m_socketFd(-1),
    m_port(0),
    m_buffer(0),
    m_bufferSize(0),
    m_dataSize(0)
{
}

S9sRpcClientPrivate::~S9sRpcClientPrivate()
{
    close();
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
 * \returns whether it connected successfully
 */
bool
S9sRpcClientPrivate::connect()
{
    struct hostent *hp;
    struct timeval timeout;
    struct sockaddr_in server;

    /*
     * disconnect first if there is a previous connection
     */
    if (m_socketFd > 0)
        close();

    if (m_hostName.empty())
    {
        m_errorString = "Controller host name is not set.";
        return false;
    }

    if (m_port <= 0)
    {
        m_errorString = "Controller port is not set.";
        return false;
    }

    PRINT_VERBOSE("Connecting to %s:%d...", STR(m_hostName), m_port);
    m_socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socketFd == -1)
    {
        m_errorString.sprintf("Error creating socket: %m");
        return false;
    }

    /*
     * Setting up a read and write timeout values
     * (otherwise it hangs on interrupted connection)
     */
    timeout.tv_sec  = 60;
    timeout.tv_usec = 0;
    setsockopt (m_socketFd, SOL_SOCKET, SO_RCVTIMEO,
                (char*) &timeout, sizeof(timeout));
    setsockopt (m_socketFd, SOL_SOCKET, SO_SNDTIMEO,
                (char*) &timeout, sizeof(timeout));

    /*
     * lookup
     */
    hp = gethostbyname(STR(m_hostName));
    if (hp == NULL)
    {
        m_errorString.sprintf("Host '%s' not found.", STR(m_hostName));
        close();
        return -1;
    }

    /*
     * Connecting to the server.
     * (TODO: IPv6)
     */
    memcpy((char *) &server.sin_addr, (char *) hp->h_addr, hp->h_length);
    server.sin_family = AF_INET;
    server.sin_port = htons(m_port);

    if (::connect(m_socketFd, (struct sockaddr *) &server, sizeof server) == -1)
    {
        m_errorString.sprintf(
                "Connect to %s:%d failed: %m.", 
                STR(m_hostName), m_port);
      
        close();
        return -1;
    }

    PRINT_VERBOSE("Connected.");

    return true;
}

void
S9sRpcClientPrivate::close()
{
    if (m_socketFd < 0)
        return;

    ::shutdown(m_socketFd, SHUT_RDWR);
    ::close(m_socketFd);
}

/**
 * write safely to a socket
 */
ssize_t
S9sRpcClientPrivate::write(
        const char *data, 
        size_t      length)
{
    ssize_t retval = -1;

    do {
        retval = ::write(m_socketFd, data, length);
    } while (retval == -1 && errno == EINTR);

    return retval;
}

/**
 * read safely from a socket
 */
ssize_t
S9sRpcClientPrivate::read(
        char   *buffer, 
        size_t  bufSize)
{
    ssize_t retval = -1;

    do {
        retval = ::read (m_socketFd, buffer, bufSize);
    } while (retval == -1 && errno == EINTR);

    return retval;
}

