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
bundle(int controllerId, const char *hostname, const char *site, const char *status)
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
twoServicesBundles()
{
    S9sVariantList list;
    list << bundle(1, "10.0.0.11", "site-a", "online");
    list << bundle(2, "10.0.0.12", "site-b", "degraded");
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

    PERFORM_TEST(testServicesBundlesShort,    retval);
    PERFORM_TEST(testServicesBundlesLong,     retval);
    PERFORM_TEST(testServicesBundlesStale,    retval);
    PERFORM_TEST(testServicesBundlesJsonOnly, retval);
    PERFORM_TEST(testServicesBundlesError,    retval);
    PERFORM_TEST(testServicesBundlesConnectionError, retval);
    PERFORM_TEST(testConfigStorageMigration,  retval);
    PERFORM_TEST(testConfigStorageMigrationSkipped, retval);
    PERFORM_TEST(testPoolModeReadinessStorage, retval);

    return retval;
}

/**
 * The default "pool-controllers --list-services-bundles" table.
 */
bool
UtS9sRpcReply::testServicesBundlesShort()
{
    const S9sString table = S9sRpcReply::servicesBundlesTable(twoServicesBundles(), false, false);

    S9S_COMPARE(table,
            "HOSTNAME  SITE   STATUS\n"
            "10.0.0.11 site-a online\n"
            "10.0.0.12 site-b degraded\n");

    S9S_COMPARE(S9sRpcReply::servicesBundlesTable(twoServicesBundles(), false, true),
            "10.0.0.11 site-a online\n"
            "10.0.0.12 site-b degraded\n");

    S9S_COMPARE(S9sRpcReply::servicesBundlesTable(S9sVariantList(), false, false),
            "HOSTNAME SITE STATUS\n");
    return true;
}

/**
 * The --long table: controller id, mode, UI version, the four services and
 * the UI URL.
 */
bool
UtS9sRpcReply::testServicesBundlesLong()
{
    const S9sString table = S9sRpcReply::servicesBundlesTable(twoServicesBundles(), true, false);

    S9S_COMPARE(table,
            "CID HOSTNAME  SITE   STATUS   MODE  VERSION    PROXY  SSH    EVENTS CLOUD  URL\n"
            "1   10.0.0.11 site-a online   fleet 2.5.0-1201 active active active active https://10.0.0.11:443/\n"
            "2   10.0.0.12 site-b degraded fleet 2.5.0-1201 active active active active https://10.0.0.12:443/\n");
    return true;
}

/**
 * A services bundle whose report is stale or missing: the controller sends
 * "unknown" states (or nothing), and the table shows "unknown".
 */
bool
UtS9sRpcReply::testServicesBundlesStale()
{
    S9sVariantMap stale;
    stale["controller_id"] = 3;
    stale["hostname"]      = "10.0.0.13";
    stale["site"]          = "site-c";

    S9sVariantList list;
    list << stale;

    S9S_COMPARE(S9sRpcReply::servicesBundlesTable(list, false, true),
            "10.0.0.13 site-c unknown\n");
    // Columns keep the width of their headers without --no-header too.
    S9S_COMPARE(S9sRpcReply::servicesBundlesTable(list, true, true),
            "3   10.0.0.13 site-c unknown unknown -       unknown unknown unknown unknown -\n");
    return true;
}

/**
 * With --print-json only the JSON reply is printed, not the table.
 */
bool
UtS9sRpcReply::testServicesBundlesJsonOnly()
{
    S9sOptions::uninit();
    S9sOptions *options = S9sOptions::instance();
    options->m_options["print_json"] = true;

    S9sRpcReply reply;
    reply["request_status"] = "Ok";
    reply["services_bundles"]   = twoServicesBundles();
    reply["total"]          = 2;

    const S9sString output = captureStdout([&reply]() { reply.printServicesBundles(); });
    S9S_VERIFY(output.contains("\"services_bundles\""));
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
UtS9sRpcReply::testServicesBundlesError()
{
    S9sOptions::uninit();
    S9sOptions *options = S9sOptions::instance();

    S9sRpcReply reply;
    reply["request_status"] = "InvalidRequest";
    reply["error_string"]   = "Controller not in pool mode. Operation not allowed.";

    const S9sString output = captureStdout([&reply]() { reply.printServicesBundles(); });
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
UtS9sRpcReply::testServicesBundlesConnectionError()
{
    S9sOptions::uninit();
    S9sOptions *options = S9sOptions::instance();
    options->setExitStatus(S9sOptions::ConnectionError);

    S9sRpcReply reply;
    reply["request_status"] = "ConnectError";
    reply["error_string"]   = "Connect to 127.0.0.1:9501 failed: Connection refused.";

    S9sString output;
    const S9sString errors = captureStderr([&reply, &output]() {
        output = captureStdout([&reply]() { reply.printServicesBundles(); });
    });

    S9S_VERIFY(errors.contains("Connection refused"));
    S9S_VERIFY(!output.contains("HOSTNAME"));
    S9S_COMPARE(options->exitStatus(), S9sOptions::ConnectionError);

    S9sOptions::uninit();
    return true;
}

namespace
{

S9sVariantMap
migratedFile(const char *path, const char *status, const char *message)
{
    S9sVariantMap file;
    file["path"]    = path;
    file["status"]  = status;
    file["message"] = message;
    return file;
}

} // namespace

/**
 * A setPoolMode that moved cmon's configuration into OpenBao (or failed to)
 * prints the engine and one line per file, with the reason when there is one.
 */
bool
UtS9sRpcReply::testConfigStorageMigration()
{
    S9sVariantList files;
    files << migratedFile("/etc/cmon.d/cmon_1.cnf", "migrated", "");
    files << migratedFile("/etc/cmon.d/cmon_2.cnf", "failed",
            "the secret store already holds a different severalnines.cmon/etc/cmon.d/cmon_2.cnf");

    S9sVariantMap migration;
    migration["engine"]      = "file";
    migration["skipped"]     = false;
    migration["skip_reason"] = "";
    migration["files"]       = files;

    S9sRpcReply reply;
    reply["request_status"]           = "InvalidRequest";
    reply["config_storage_migration"] = migration;

    // With the error of the failed call, on the standard error.
    const S9sString output = captureStderr([&reply]() { reply.printConfigStorageMigration(stderr); });
    S9S_COMPARE(output,
            "Secret storage engine: file\n"
            "  migrated /etc/cmon.d/cmon_1.cnf\n"
            "  failed   /etc/cmon.d/cmon_2.cnf: the secret store already holds a different "
            "severalnines.cmon/etc/cmon.d/cmon_2.cnf\n");
    return true;
}

/**
 * Nothing moved: one line saying why. No report at all (pool mode disabled,
 * or an older controller): nothing printed.
 */
bool
UtS9sRpcReply::testConfigStorageMigrationSkipped()
{
    S9sVariantMap migration;
    migration["engine"]      = "vault";
    migration["skipped"]     = true;
    migration["skip_reason"] = "already_on_vault";
    migration["files"]       = S9sVariantList();

    S9sRpcReply reply;
    reply["request_status"]           = "Ok";
    reply["config_storage_migration"] = migration;

    S9S_COMPARE(captureStdout([&reply]() { reply.printConfigStorageMigration(); }),
            "Secret storage engine: vault (nothing moved: cmon already keeps its "
            "configuration in OpenBao)\n");

    migration["engine"]      = "file";
    migration["skip_reason"] = "config_storage_not_ready";
    reply["config_storage_migration"] = migration;
    S9S_COMPARE(captureStdout([&reply]() { reply.printConfigStorageMigration(); }),
            "Secret storage engine: file (nothing moved: no ready configuration storage)\n");

    S9sRpcReply noReport;
    noReport["request_status"] = "Ok";
    S9S_COMPARE(captureStdout([&noReport]() { noReport.printConfigStorageMigration(); }), "");
    return true;
}

/**
 * --pool-readiness names the secret storage engine and, in pool mode, the
 * configuration storage the pool runs on.
 */
bool
UtS9sRpcReply::testPoolModeReadinessStorage()
{
    S9sOptions::uninit();

    S9sRpcReply reply;
    reply["request_status"]        = "Ok";
    reply["pool_mode"]             = true;
    reply["secret_storage_engine"] = "vault";
    reply["pool_config_storage"]   = "vault";

    S9sString output = captureStdout([&reply]() { reply.printPoolModeReadiness(); });
    S9S_VERIFY(output.contains("Secret storage engine: vault\n"));
    S9S_VERIFY(output.contains("Pool configuration storage: vault\n"));

    reply["pool_mode"]             = false;
    reply["applicable"]            = false;
    reply["secret_storage_engine"] = "file";
    reply["pool_config_storage"]   = "";
    output = captureStdout([&reply]() { reply.printPoolModeReadiness(); });
    S9S_VERIFY(output.contains("Secret storage engine: file\n"));
    S9S_VERIFY(!output.contains("Pool configuration storage"));

    S9sOptions::uninit();
    return true;
}

S9S_UNIT_TEST_MAIN(UtS9sRpcReply)
