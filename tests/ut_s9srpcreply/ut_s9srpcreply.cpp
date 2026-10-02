/*
 * Severalnines Tools
 * Copyright (C) 2026  Severalnines AB
 *
 * This file is part of s9s-tools.
 *
 * s9s-tools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * s9s-tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with s9s-tools. If not, see <http://www.gnu.org/licenses/>.
 */
#include "ut_s9srpcreply.h"

#include "s9soptions.h"
#include "s9srpcreply.h"

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

namespace
{

S9sVariantMap
frontEnd(int controllerId, const char *hostname, const char *site, const char *status)
{
    S9sVariantMap service;
    service["status"] = "active";

    S9sVariantMap services;
    services["cmon-proxy"]  = service;
    services["cmon-ssh"]    = service;
    services["cmon-events"] = service;
    services["cmon-cloud"]  = service;

    S9sVariantMap entry;
    entry["class_name"]    = "CmonFrontEndHost";
    entry["controller_id"] = controllerId;
    entry["hostname"]      = hostname;
    entry["site"]          = site;
    entry["status"]        = status;
    entry["proxy_mode"]    = "fleet";
    entry["ui_version"]    = "2.5.0-1201";
    S9sString url;
    url.sprintf("https://%s:443/", hostname);
    entry["ui_url"]        = url;
    entry["services"]      = services;
    return entry;
}

S9sVariantList
twoFrontEnds()
{
    S9sVariantList list;
    list << frontEnd(1, "10.0.0.11", "site-a", "online");
    list << frontEnd(2, "10.0.0.12", "site-b", "degraded");
    return list;
}

/*
 * Runs \p print with stdout redirected to a file and returns what it wrote.
 */
template <typename Function>
S9sString
captureStdout(Function print)
{
    char path[] = "/tmp/ut_s9srpcreply_XXXXXX";
    const int fd = mkstemp(path);
    if (fd < 0)
        return S9sString();

    fflush(stdout);
    const int saved = dup(STDOUT_FILENO);
    dup2(fd, STDOUT_FILENO);

    print();

    fflush(stdout);
    dup2(saved, STDOUT_FILENO);
    close(saved);

    S9sString content;
    char buffer[4096];
    lseek(fd, 0, SEEK_SET);
    for (;;)
    {
        const ssize_t n = read(fd, buffer, sizeof(buffer));
        if (n <= 0)
            break;
        content += std::string(buffer, n);
    }

    close(fd);
    unlink(path);
    return content;
}

} // namespace

UtS9sRpcReply::UtS9sRpcReply()
{
}

UtS9sRpcReply::~UtS9sRpcReply()
{
}

bool
UtS9sRpcReply::runTest(const char *testName)
{
    bool retval = true;

    PERFORM_TEST(testCcFrontendsShort,    retval);
    PERFORM_TEST(testCcFrontendsLong,     retval);
    PERFORM_TEST(testCcFrontendsStale,    retval);
    PERFORM_TEST(testCcFrontendsJsonOnly, retval);
    PERFORM_TEST(testCcFrontendsError,    retval);

    return retval;
}

/**
 * The default "pool-controllers --list-frontends" table.
 */
bool
UtS9sRpcReply::testCcFrontendsShort()
{
    const S9sString table = S9sRpcReply::ccFrontendsTable(twoFrontEnds(), false, false);

    S9S_COMPARE(table,
            "HOSTNAME  SITE   STATUS\n"
            "10.0.0.11 site-a online\n"
            "10.0.0.12 site-b degraded\n");

    S9S_COMPARE(S9sRpcReply::ccFrontendsTable(twoFrontEnds(), false, true),
            "10.0.0.11 site-a online\n"
            "10.0.0.12 site-b degraded\n");

    S9S_COMPARE(S9sRpcReply::ccFrontendsTable(S9sVariantList(), false, false),
            "HOSTNAME SITE STATUS\n");
    return true;
}

/**
 * The --long table: controller id, mode, UI version, the four services and
 * the UI URL.
 */
bool
UtS9sRpcReply::testCcFrontendsLong()
{
    const S9sString table = S9sRpcReply::ccFrontendsTable(twoFrontEnds(), true, false);

    S9S_COMPARE(table,
            "CID HOSTNAME  SITE   STATUS   MODE  VERSION    PROXY  SSH    EVENTS CLOUD  URL\n"
            "1   10.0.0.11 site-a online   fleet 2.5.0-1201 active active active active https://10.0.0.11:443/\n"
            "2   10.0.0.12 site-b degraded fleet 2.5.0-1201 active active active active https://10.0.0.12:443/\n");
    return true;
}

/**
 * A frontend whose report is stale or missing: the controller sends
 * "unknown" states (or nothing), and the table shows "unknown".
 */
bool
UtS9sRpcReply::testCcFrontendsStale()
{
    S9sVariantMap stale;
    stale["controller_id"] = 3;
    stale["hostname"]      = "10.0.0.13";
    stale["site"]          = "site-c";

    S9sVariantList list;
    list << stale;

    S9S_COMPARE(S9sRpcReply::ccFrontendsTable(list, false, true),
            "10.0.0.13 site-c unknown\n");
    // Columns keep the width of their headers without --no-header too.
    S9S_COMPARE(S9sRpcReply::ccFrontendsTable(list, true, true),
            "3   10.0.0.13 site-c unknown unknown -       unknown unknown unknown unknown -\n");
    return true;
}

/**
 * With --print-json only the JSON reply is printed, not the table.
 */
bool
UtS9sRpcReply::testCcFrontendsJsonOnly()
{
    S9sOptions::uninit();
    S9sOptions *options = S9sOptions::instance();
    options->m_options["print_json"] = true;

    S9sRpcReply reply;
    reply["request_status"] = "Ok";
    reply["cc_frontends"]   = twoFrontEnds();
    reply["total"]          = 2;

    const S9sString output = captureStdout([&reply]() { reply.printCcFrontends(); });
    S9S_VERIFY(output.contains("\"cc_frontends\""));
    S9S_VERIFY(output.contains("10.0.0.12"));
    S9S_VERIFY(!output.contains("HOSTNAME"));
    S9S_COMPARE(options->exitStatus(), 0);

    S9sOptions::uninit();
    return true;
}

/**
 * An error reply makes the command fail.
 */
bool
UtS9sRpcReply::testCcFrontendsError()
{
    S9sOptions::uninit();
    S9sOptions *options = S9sOptions::instance();

    S9sRpcReply reply;
    reply["request_status"] = "InvalidRequest";
    reply["error_string"]   = "Controller not in pool mode. Operation not allowed.";

    const S9sString output = captureStdout([&reply]() { reply.printCcFrontends(); });
    S9S_VERIFY(!output.contains("HOSTNAME"));
    S9S_VERIFY(options->exitStatus() != 0);

    S9sOptions::uninit();
    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sRpcReply)
