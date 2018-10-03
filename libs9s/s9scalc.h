/*
 * Severalnines Tools
 * Copyright (C) 2016-2018 Severalnines AB
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
#pragma once

#include "S9sDisplay"
#include "S9sRpcClient"
#include "S9sSpreadsheet"

class S9sCalc :
    public S9sDisplay
{
    public:
        S9sCalc(S9sRpcClient &client);
        virtual ~S9sCalc();

        void setSpreadsheetName(const S9sString &name);
        S9sString spreadsheetName() const;

        void main();

    protected:
        virtual void processKey(int key);
        virtual void processButton(uint button, uint x, uint y);
        virtual bool refreshScreen();
        virtual void printHeader();
        virtual void printFooter();

    private:
        S9sRpcClient                &m_client;
        S9sSpreadsheet               m_spreadsheet;
        S9sString                    m_spreadsheetName;
        S9sString                    m_errorString;
};

