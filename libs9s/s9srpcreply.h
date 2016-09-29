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
#pragma once

#include "S9sVariantMap"

class S9sRpcReply : public S9sVariantMap
{
    public:
        bool isOk() const;
        S9sString errorString() const;

        int jobId() const;
        S9sString jobTitle() const;
        bool isJobFailed() const;
        
        bool progressLine(S9sString &retval, bool syntaxHighlight);

        void printJobStarted();
        void printJobLog();
        void printClusterList();
        void printNodeList();
        void printJobList();
        void printProcessList();
        
    private:
        void printJobLogBrief();
        void printJobLogLong();

        void printClusterListBrief();
        void printClusterListLong();
        
        void printNodeListBrief();
        void printNodeListLong();
        
        void printJobListBrief();
        void printJobListLong();

        static S9sString progressBar(double percent, bool syntaxHighlight);
        void html2ansi(S9sString &s);

        S9sString 
            nodeTypeFlag(
                const S9sString &className,
                const S9sString &nodeType);
        
        S9sString 
            nodeStateFlag(
                const S9sString &state);
};

