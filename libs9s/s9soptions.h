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

#include "S9sString"
#include "S9sVariant"
#include "S9sVariantMap"

#define PRINT_VERBOSE(...) \
    S9sOptions::printVerbose(__VA_ARGS__)

/**
 * Singleton class to handle s9s command line options and settings.
 */
class S9sOptions
{
    public:
        static S9sOptions *instance();
        static void uninit();

        enum OperationMode 
        {
            NoMode  = 0,
            Cluster,
            Node,
        };

        bool readOptions(int *argc, char *argv[]);
        bool executeInfoRequest();

        void setController(const S9sString &url);
        S9sString controller() const;

        int controllerPort() const;

        S9sString rpcToken() const;

        bool isNodeOperationRequested() const;
        bool isClusterOperationRequested() const;

        bool isListRequested() const;
        bool isLongRequested() const;
        bool isJsonRequested() const;

        bool useSyntaxHighlight() const;

        S9sString binaryName() const;
        S9sString errorString() const;
        int exitStatus() const;
        bool isVerbose() const;

        static void printVerbose(const char *formatString, ...);

    private:
        bool readOptionsNoMode(int argc, char *argv[]);
        bool readOptionsNode(int argc, char *argv[]);
        bool readOptionsCluster(int argc, char *argv[]);

        bool setMode(const S9sString &modeName);

        S9sOptions();
        ~S9sOptions();
        
        static S9sOptions *sm_instance;

    private:
        S9sFileName        m_myName;
        OperationMode      m_operationMode;
        int                m_exitStatus;
        S9sString          m_errorMessage;
        S9sVariantMap      m_options;

    friend class UtS9sOptions;
};
