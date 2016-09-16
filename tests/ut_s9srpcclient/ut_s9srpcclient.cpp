/*
 * Severalnines Tools
 * Copyright (C) 2016  Severalnines AB
 *
 * This file is part of s9s-tools.
 *
 * s9s-tools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar. If not, see <http://www.gnu.org/licenses/>.
 */
#include "ut_s9srpcclient.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

/******************************************************************************
 *
 */
bool 
S9sRpcClientTester::executeRequest(
        const S9sString &uri,
        const S9sString &payload)
{
    S9S_DEBUG("*** ");
    S9S_DEBUG("*** uri     : %s", STR(uri));
    S9S_DEBUG("*** payload : \n%s\n", STR(payload));

    m_urls     << uri;
    m_payloads << payload;

    return true;
}

S9sString 
S9sRpcClientTester::uri(
        const uint index) const
{
    if (index >= m_urls.size())
        return S9sString();

    return m_urls[index].toString();
}

S9sString 
S9sRpcClientTester::payload(
        const uint index) const
{
    if (index >= m_payloads.size())
        return S9sString();

    return m_payloads[index].toString();
}

/******************************************************************************
 *
 */
UtS9sRpcClient::UtS9sRpcClient()
{
}

UtS9sRpcClient::~UtS9sRpcClient()
{
}

bool
UtS9sRpcClient::runTest(const char *testName)
{
    bool retval = true;

    PERFORM_TEST(test01,   retval);

    return retval;
}

bool
UtS9sRpcClient::test01()
{
    S9sRpcClientTester client;

    S9S_VERIFY(client.getClusters());
    S9S_COMPARE(client.uri(0u), "/0/clusters/");
    S9S_VERIFY(client.payload(0u).contains(
                "\"operation\": \"getAllClusterInfo\""));

    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sRpcClient)
