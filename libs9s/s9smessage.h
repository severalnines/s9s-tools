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
 * S9sTools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with S9sTools. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <S9sVariantMap>

class S9sMessage
{
    public:
        S9sMessage();
        S9sMessage(const S9sVariantMap &properties);

        virtual ~S9sMessage();
        
        S9sMessage &operator=(const S9sVariantMap &rhs);

        const S9sVariantMap &toVariantMap() const;

        bool hasFileName() const;
        S9sString fileName() const;

        bool hasLineNumber() const;
        int lineNumber() const;

        S9sString message() const;

    private:
        S9sVariantMap    m_properties;
};
