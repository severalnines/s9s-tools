/* 
 * Copyright (C) 2016 severalnines.com
 */
#include "s9srpcclient.h"
#include "s9srpcclient_p.h"

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

#define READ_SIZE 512

S9sRpcClient::S9sRpcClient() :
    m_priv(new S9sRpcClientPrivate)
{
}

S9sRpcClient::S9sRpcClient(
        const S9sString &hostName,
        const int        port,
        const S9sString &token) :
    m_priv(new S9sRpcClientPrivate)
{
    m_priv->m_hostName = hostName;
    m_priv->m_port     = port;
    m_priv->m_token    = token;
}


/**
 * Copy constructor. Nothing to see here.
 */
S9sRpcClient::S9sRpcClient (
		const S9sRpcClient &orig)
{
	m_priv = orig.m_priv;

	if (m_priv) 
		m_priv->ref ();
}

/**
 * 
 */
S9sRpcClient::~S9sRpcClient()
{
	if (m_priv && m_priv->unRef() == 0)
    {
        delete m_priv;
        m_priv = 0;
	}
}

/**
 * Assignment operator to utilize the implicit sharing.
 */
S9sRpcClient &
S9sRpcClient::operator= (
		const S9sRpcClient &rhs)
{
	if (this == &rhs)
		return *this;

	if (m_priv && m_priv->unRef() == 0)
    {
        delete m_priv;
        m_priv = 0;
	}

	m_priv = rhs.m_priv;
	if (m_priv) 
    {
		m_priv->ref ();
	}

	return *this;
}

int
S9sRpcClient::executeRequest(
        const S9sString &uri,
        const S9sString &payload)
{
    S9sString    header;
    int          socketFd = m_priv->connectSocket();
    ssize_t      readLength;

    if (socketFd < 0)
    {
        S9S_WARNING("Connect error.");
        return -1;
    }

    header.sprintf(
        "POST %s HTTP/1.0\r\n"
        "Host: %s:%d\r\n"
        "User-Agent: cmonjsclient/1.0\r\n"
        "Connection: close\r\n"
        "Accept: application/json\r\n"
        "Transfer-Encoding: identity\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %u\r\n"
        "\r\n",
        STR(uri), 
        STR(m_priv->m_hostName), 
        m_priv->m_port, 
        payload.length());

    /*
     * Sending teh HTTP request header.
     */
    if (m_priv->writeSocket(socketFd, STR(header), header.length()) < 0)
    {
        S9S_WARNING("Error writing socket %d: %m", socketFd);
       
        m_priv->closeSocket(socketFd);
        return -1;
    }

    /*
     * Sending the JSON payload.
     */
    if (!payload.empty())
    {
        if (m_priv->writeSocket(socketFd, STR(payload), payload.length()) < 0)
        {
            S9S_WARNING("Error writing socket %d: %m", socketFd);
       
            m_priv->closeSocket(socketFd);
            return -1;
        }
    }

    /*
     * Reading the reply from the server.
     */
    m_priv->clearBuffer();
    readLength = 0;
    do
    {
        m_priv->ensureHasBuffer(m_priv->m_dataSize + READ_SIZE);

        readLength = m_priv->readSocket(
                socketFd,
                m_priv->m_buffer + m_priv->m_dataSize, 
                READ_SIZE - 1);

        if (readLength > 0)
            m_priv->m_dataSize += readLength;

    } while (readLength > 0);

    // Closing the buffer with a null terminating byte.
    m_priv->ensureHasBuffer(m_priv->m_dataSize + 1);
    m_priv->m_buffer[m_priv->m_dataSize] = '\0';
    m_priv->m_dataSize += 1;

    // Closing the socket.
    m_priv->closeSocket(socketFd);
    
    return 0;
}
