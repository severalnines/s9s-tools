/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#include "s9srpcclient_p.h"

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
