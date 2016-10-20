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
#include <cerrno>

#include "S9sRegExp"
#include "S9sOptions"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sRpcClientPrivate::S9sRpcClientPrivate() :
    m_referenceCounter(1),
    m_socketFd(-1),
    m_port(0),
    m_useTls(false),
    m_buffer(0),
    m_bufferSize(0),
    m_dataSize(0),
    m_sslContext(0),
    m_ssl(0)
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
        return false;
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
        return false;
    }

    PRINT_VERBOSE("Connected.");

    if (m_useTls)
    {
        PRINT_VERBOSE ("Initiate TLS...");

        static bool openSslInitialized;
        if (!openSslInitialized)
        {
            openSslInitialized = true;
            SSL_load_error_strings ();
            SSL_library_init ();
        }

        #if (OPENSSL_VERSION_NUMBER >= 0x10100000L)
        m_sslContext = SSL_CTX_new(TLS_client_method());
        #else
        m_sslContext = SSL_CTX_new(SSLv23_client_method());
        #endif

        if (!m_sslContext)
        {
            m_errorString = "Couldn't create SSL context.";
            close();
            return false;
        }

        SSL_CTX_set_verify(m_sslContext, SSL_VERIFY_NONE, NULL);
        SSL_CTX_set_options(m_sslContext,
                SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
        SSL_CTX_set_mode(m_sslContext, SSL_MODE_AUTO_RETRY);

        m_ssl = SSL_new(m_sslContext);

        if (!m_ssl)
        {
            m_errorString = "Couldn't create SSL.";
            close();
            return false;
        }

        SSL_set_fd(m_ssl, m_socketFd);
        SSL_set_connect_state(m_ssl);
        SSL_set_tlsext_host_name(m_ssl, STR(m_hostName));

        if (SSL_connect(m_ssl) <= 0 || SSL_do_handshake(m_ssl) <= 0)
        {
            m_errorString = "SSL handshake failed.";
            close();
            return false;
        }

        PRINT_VERBOSE("TLS handshake finished (version: %s, cipher: %s).",
            SSL_get_version(m_ssl), SSL_get_cipher(m_ssl));
    }

    return true;
}

void
S9sRpcClientPrivate::close()
{
    if (m_socketFd < 0)
        return;

    if (m_ssl)
    {
        SSL_shutdown(m_ssl);
        SSL_free(m_ssl);
        m_ssl = 0;
    }

    if (m_sslContext)
    {
        SSL_CTX_free(m_sslContext);
        m_sslContext = 0;
    }

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

    if (m_ssl)
        return SSL_write(m_ssl, data, length);

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

    if (m_ssl)
        return SSL_read(m_ssl, buffer, bufSize);

    do {
        retval = ::read (m_socketFd, buffer, bufSize);
    } while (retval == -1 && errno == EINTR);

    return retval;
}

/**
 * A bit higher level method, to parse out the cookies
 * (HTTP session data) from the read data (m_buffer)
 */
void
S9sRpcClientPrivate::parseHeaders()
{
    if (! m_buffer || m_dataSize < 12)
        return;

    S9sRegExp regexp ("Set-Cookie: ([^=]*)=([^,;\r\n]*)");
    regexp.setIgnoreCase(true);

    S9sString buffer;
    buffer.assign(m_buffer, m_dataSize);

    int lastIdx = 0;
    while (lastIdx < (int) buffer.size() && 
           regexp == buffer.substr(lastIdx))
    {
        m_cookies[regexp[1]] = regexp[2];
        lastIdx += regexp.firstIndex()+1;
    }
}

/**
 * The HTTP cookie header lines must be sent on HTTP requests to the server
 */
S9sString
S9sRpcClientPrivate::cookieHeaders() const
{
    if (m_cookies.empty())
        return "";

    S9sString cookieHeader = "Cookie: ";

    S9sVariantMap::const_iterator it;
    for (it = m_cookies.begin(); it != m_cookies.end(); ++it)
    {
        if (cookieHeader != "Cookie: ")
            cookieHeader += "; ";

        S9sString keyVal;
        keyVal.sprintf("%s=%s", STR(it->first), STR(it->second.toString()));

        cookieHeader += keyVal;
    }

    cookieHeader += "\r\n";
    return cookieHeader;
}

