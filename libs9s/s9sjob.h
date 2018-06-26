/*
 * Severalnines Tools
 * Copyright (C) 2018 Severalnines AB
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

#include "S9sObject"
#include "S9sVariantMap"


/**
 * A class that represents a job instance (CmonJobInstance).
 */
class S9sJob : public S9sObject
{
    public:
        S9sJob();
        S9sJob(const S9sVariantMap &properties);

        virtual ~S9sJob();

        S9sJob &operator=(const S9sVariantMap &rhs);
       
        virtual int id() const;
        int clusterId() const;
        S9sString title() const;
        S9sString userName() const;
        int userId() const;
        S9sString groupName() const;
        S9sString status() const;
        S9sString createdString() const;
        S9sString endedString() const;
        S9sString startedString() const;
        S9sString scheduledString() const;
        bool hasProgressPercent() const;
        double progressPercent() const;

        S9sVariantList tags() const;

        S9sString 
            tags(
                bool            useSyntaxHightlight, 
                const S9sString defaultValue) const;

        bool hasTags(const S9sVariantList &requiredTags);
};


