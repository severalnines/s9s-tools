/* 
 * Copyright (C) 2016 severalnines.com
 */
#include "s9srpcclient.h"
#include "s9srpcclient_p.h"

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

S9sRpcClient::S9sRpcClient() :
    m_priv(new S9sRpcClientPrivate)
{
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

