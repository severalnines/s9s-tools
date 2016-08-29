/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#pragma once

class S9sRpcClientPrivate
{
    public:
        S9sRpcClientPrivate();
        ~S9sRpcClientPrivate();

        void ref();
        int unRef();

    private:
        int             m_referenceCounter;
};
