/* 
 * Copyright (C) 2016 severalnines.com
 */
#include "s9sformatter.h"

#include "S9sOptions"
#include "S9sObject"
#include "S9sUser"

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

bool
S9sFormatter::useSyntaxHighLight() const
{
    S9sOptions *options = S9sOptions::instance();
   
    return options->useSyntaxHighlight();
}

const char *
S9sFormatter::headerColorBegin() const
{
    if (useSyntaxHighLight())
        return TERM_BOLD;

    return "";
}

const char *
S9sFormatter::headerColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::userColorBegin() const
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_ORANGE;

    return "";
}

const char *
S9sFormatter::userColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::groupColorBegin(
        const S9sString &groupName) const
{
    if (useSyntaxHighLight())
    {
        if (groupName == "0")
            return XTERM_COLOR_RED;
        else
            return XTERM_COLOR_CYAN;
    }

    return "";
}

const char *
S9sFormatter::groupColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::directoryColorBegin() const
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_DIR;

    return "";
}

const char *
S9sFormatter::directoryColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::folderColorBegin() const
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_FOLDER;

    return "";
}

const char *
S9sFormatter::folderColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::clusterStateColorBegin(
        const S9sString &state)
{
    if (useSyntaxHighLight())
    {
        if (state == "STARTED")
            return XTERM_COLOR_GREEN;
       else if (state == "FAILED" || state == "FAILURE")
            return XTERM_COLOR_RED;
        else
            return XTERM_COLOR_YELLOW;
    }
    
    return "";
}

const char *
S9sFormatter::clusterStateColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::jobStateColorBegin(
        const S9sString &state)
{
    if (useSyntaxHighLight())
    {
        if (state.startsWith("RUNNING"))
        {
            return XTERM_COLOR_GREEN;
        } else if (state == "FINISHED")
        {
            return XTERM_COLOR_GREEN;
        } else if (state == "FAILED")
        {
            return XTERM_COLOR_RED;
        }
    }

    return "";
}

const char *
S9sFormatter::jobStateColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::clusterColorBegin() const
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_BLUE;

    return "";
}

const char *
S9sFormatter::clusterColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

S9sString 
S9sFormatter::bytesToHuman(
        ulonglong bytes) const
{
    S9sOptions *options = S9sOptions::instance();
    S9sString   retval;
    S9sVariant  variant = bytes;

    if (!options->humanReadable())
    {
        retval.sprintf("%'llu", variant.toULongLong());
    } else if (variant.toTBytes() > 1.0)
    {
        retval.sprintf("%.1fTB", variant.toTBytes());
    } else if (variant.toGBytes() >= 1.0) 
    {
        retval.sprintf("%.1fGB", variant.toGBytes());
    } else {
        retval.sprintf("%.1fMB", variant.toMBytes());
    }

    return retval;
}

const char *
S9sFormatter::ipColorBegin(
        const S9sString &ip)
{
    if (useSyntaxHighLight() && ip.looksLikeIpAddress())
        return XTERM_COLOR_IP;
    else if (useSyntaxHighLight() && ip.empty())
        return XTERM_COLOR_IP;

    return "";
}

const char *
S9sFormatter::ipColorEnd(
        const S9sString &ip) 
{
    if (useSyntaxHighLight() && ip.looksLikeIpAddress())
        return TERM_NORMAL;
    else if (useSyntaxHighLight() && ip.empty())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::serverColorBegin() 
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_SERVER;

    return "";
}

const char *
S9sFormatter::serverColorEnd() 
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::containerColorBegin(
        int stateAsChar)
{
    if (useSyntaxHighLight())
    {
        if (stateAsChar == 't')
            return XTERM_COLOR_RED;
        else if (stateAsChar == 's')
            return XTERM_COLOR_RED;
        else if (stateAsChar == '?')
            return XTERM_COLOR_RED;
        else if (stateAsChar == 'q')
            return XTERM_COLOR_YELLOW;

        return XTERM_COLOR_NODE;
    }

    return "";
}

const char *
S9sFormatter::containerColorEnd() 
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::typeColorBegin() const
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_GREEN;

    return "";
}

const char *
S9sFormatter::typeColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}

const char *
S9sFormatter::greyColorBegin() const
{
    if (useSyntaxHighLight())
        return XTERM_COLOR_DARK_GRAY;

    return "";
}

const char *
S9sFormatter::greyColorEnd() const
{
    if (useSyntaxHighLight())
        return TERM_NORMAL;

    return "";
}


S9sString 
S9sFormatter::mBytesToHuman(
        ulonglong mBytes) const
{
    return bytesToHuman(mBytes * (1024ull * 1024ull));
}

S9sString 
S9sFormatter::kiloBytesToHuman(
        ulonglong kBytes) const
{
    return bytesToHuman(kBytes * 1024ull);
}

S9sString
S9sFormatter::percent(
        const ulonglong total,
        const ulonglong part) const
{
    S9sString retval;
    double    percent;
    
    if (total == 0ull)
        percent = 100.0;
    else
        percent = 100.0 * ((double)part / (double)total);

    retval.sprintf("%.1f%%", percent);
    return retval;
}

void 
S9sFormatter::printObjectStat(
        S9sObject    &object) const
{
    //S9sOptions *options = S9sOptions::instance();
    //int         terminalWidth = options->terminalWidth();
    const char *greyBegin = greyColorBegin();
    const char *greyEnd   = greyColorEnd();
    
    //
    // "    Name: 192.168.0.128"
    //
    printf("%s    Name:%s ", greyBegin, greyEnd);
    // FIXME: the color should depend on the class
    printf("%s", clusterColorBegin());
    printf("%-32s ", STR(object.name()));
    printf("%s", clusterColorEnd());
    
    printf("\n");
   
    //
    // "CDT path: /ft_ndb_6776"
    //
    printf("%sCDT path:%s ", greyBegin, greyEnd);
    printf("%s", folderColorBegin());
    printf("%-32s ", STR(object.cdtPath()));
    printf("%s", folderColorEnd());
    printf("\n");
    
    //
    // "   Class: CmonNdbHost                          Owner: pipas/testgroup"
    //
    printf("%s   Class:%s ", greyBegin, greyEnd);
    printf("%s%-33s%s ", 
            typeColorBegin(), 
            STR(object.className()), 
            typeColorEnd());
    
    printf("%s   Owner:%s ", greyBegin, greyEnd);
    printf("%s%s%s/%s%s%s ", 
            userColorBegin(), STR(object.ownerName()), userColorEnd(),
            groupColorBegin(object.groupOwnerName()), 
            STR(object.groupOwnerName()), 
            groupColorEnd());
    
    printf("\n");
    
    //
    // "      ID: -                                      ACL: rwxrw----"
    //
    printf("%s      ID:%s ", greyBegin, greyEnd);
    printf("%-38s", STR(object.id("-")));

    printf("%s ACL:%s ", greyBegin, greyEnd);
    printf("%s", STR(object.aclShortString()));

    printf("\n");
}

void 
S9sFormatter::printUserStat(
        S9sUser &user) const
{
    S9sOptions *options = S9sOptions::instance();
    int         terminalWidth = options->terminalWidth();
    const char *greyBegin = greyColorBegin();
    const char *greyEnd   = greyColorEnd();
    S9sString   title;
    
    //
    // The title that is in inverse. 
    //
    if (!user.fullName().empty())
        title.sprintf("%s", STR(user.fullName()));
    else
        title.sprintf("%s", STR(user.userName()));

    printf("%s", TERM_INVERSE);
    printf("%s", STR(title));
    for (int n = title.length(); n < terminalWidth; ++n)
        printf(" ");
    printf("%s", TERM_NORMAL);
   
    printObjectStat(user);
 
    //
    // "Fullname: László Pere                  Email: laszlo@severalnines.com"
    //
    printf("%sFullname:%s ", greyBegin, greyEnd);
    printf("%-28s ", STR(user.fullName("-")));
    printf("\n");
   
    //
    //
    //
    printf("%s   Email:%s ", greyBegin, greyEnd);
    printf("%s ", STR(user.emailAddress("-")));
    printf("\n");
    
    //
    //
    //
    printf("%sDisabled:%s ", greyBegin, greyEnd);
    printf("%s", user.isDisabled() ? "yes" : "no");
    printf("\n");

    
    //
    // " Suspend: no                   Failed logins: 0"
    //
    printf("%s Suspend:%s ", greyBegin, greyEnd);
    printf("%-19s ", user.isSuspended() ? "yes" : "no");
    
    printf("%s         Failed logins:%s ", greyBegin, greyEnd);
    printf("%d", user.nFailedLogins());
    printf("\n");
    
    //
    // "  Groups: users"
    //
    printf("%s  Groups:%s ", greyBegin, greyEnd);
    printf("%-30s ", STR(user.groupNames(", ")));
    printf("\n");

    //
    // " Created: 2017-10-26T12:08:55.945Z"
    //
    printf("%s Created:%s ", greyBegin, greyEnd);
    printf("%-30s ", STR(user.createdString("-")));
    printf("\n");

    //
    // "   Login: 2017-10-26T12:10:37.762Z"
    //
    printf("%s   Login:%s ", greyBegin, greyEnd);
    printf("%-30s ", STR(user.lastLoginString("-")));
    printf("\n");
    
    //
    // 
    //
    printf("%s Failure:%s ", greyBegin, greyEnd);
    printf("%-24s ", STR(user.failedLoginString("-")));
    
    
    printf("\n\n");
}

