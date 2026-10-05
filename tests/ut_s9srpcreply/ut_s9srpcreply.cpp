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
 * Runs \p print with \p stream (stdout or stderr) redirected to a file and
 * returns what it wrote.
 */
template <typename Function>
S9sString
captureStream(FILE *stream, Function print)
{
    char path[] = "/tmp/ut_s9srpcreply_XXXXXX";
    const int fd = mkstemp(path);
    if (fd < 0)
        return S9sString();

    const int streamFd = fileno(stream);
    fflush(stream);
    const int saved = dup(streamFd);
    dup2(fd, streamFd);

    print();

    fflush(stream);
    dup2(saved, streamFd);
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

template <typename Function>
S9sString
captureStdout(Function print)
{
    return captureStream(stdout, print);
}

template <typename Function>
S9sString
captureStderr(Function print)
{
    return captureStream(stderr, print);
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
    PERFORM_TEST(testCcFrontendsConnectionError, retval);
    PERFORM_TEST(testSetPoolModeWarnings, retval);

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

/**
 * When the controller can't be reached the client puts the error into the
 * reply: it is printed to the standard error and the ConnectionError exit
 * status the client set is kept.
 */
bool
UtS9sRpcReply::testCcFrontendsConnectionError()
{
    S9sOptions::uninit();
    S9sOptions *options = S9sOptions::instance();
    options->setExitStatus(S9sOptions::ConnectionError);

    S9sRpcReply reply;
    reply["request_status"] = "ConnectError";
    reply["error_string"]   = "Connect to 127.0.0.1:9501 failed: Connection refused.";

    S9sString output;
    const S9sString errors = captureStderr([&reply, &output]() {
        output = captureStdout([&reply]() { reply.printCcFrontends(); });
    });

    S9S_VERIFY(errors.contains("Connection refused"));
    S9S_VERIFY(!output.contains("HOSTNAME"));
    S9S_COMPARE(options->exitStatus(), S9sOptions::ConnectionError);

    S9sOptions::uninit();
    return true;
}

/**
 * The "warnings" of a successful setPoolMode reply go to the standard error,
 * one per line, and do not change the exit status.
 */
bool
UtS9sRpcReply::testSetPoolModeWarnings()
{
    S9sOptions::uninit();
    S9sOptions *options = S9sOptions::instance();

    S9sVariantList warnings;
    warnings << "The CC frontend on 10.0.0.11 can't be used in pool mode: "
                "cmon-proxy 2.4.0 is older than 2.5.0. Upgrade clustercontrol-proxy "
                "and clustercontrol-mcc on 10.0.0.11 to 2.5.0 or later.";
    warnings << "Second warning.";

    S9sRpcReply reply;
    reply["request_status"] = "Ok";
    reply["warnings"]       = warnings;

    S9sString output;
    S9sString errors = captureStderr([&reply, &output]() {
        output = captureStdout([&reply]() { reply.printSetPoolModeWarnings(); });
    });

    S9S_COMPARE(errors,
            "Warning: The CC frontend on 10.0.0.11 can't be used in pool mode: "
            "cmon-proxy 2.4.0 is older than 2.5.0. Upgrade clustercontrol-proxy "
            "and clustercontrol-mcc on 10.0.0.11 to 2.5.0 or later.\n"
            "Warning: Second warning.\n");
    S9S_COMPARE(output, "");
    S9S_COMPARE(options->exitStatus(), 0);

    // A reply without warnings prints nothing.
    S9sRpcReply plain;
    plain["request_status"] = "Ok";
    errors = captureStderr([&plain]() { plain.printSetPoolModeWarnings(); });
    S9S_COMPARE(errors, "");

    S9sOptions::uninit();
    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sRpcReply)
