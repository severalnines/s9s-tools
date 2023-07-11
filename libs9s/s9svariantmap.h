/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#pragma once

#include "s9svariant.h"
#include "s9smap.h"
#include "s9svector.h"
#include "s9sstring.h"
#include "s9sparsecontext.h"

class S9sVariantList;

class S9sVariantMap : public S9sMap<S9sString, S9sVariant>
{
    public:
        S9sVariantMap() : S9sMap<S9sString, S9sVariant>() {};
        virtual ~S9sVariantMap() {};

        S9sVector<S9sString> keys() const;

        const S9sVariant &valueByPath(const S9sString &path) const;
        const S9sVariant &valueByPath(S9sVariantList path) const;

        bool parse(const char *source);

        S9sString toString() const;

        S9sString toString(
                const bool       syntaxHighlight,
                const S9sString &formatString) const;

        bool parseAssignments(const S9sString &input);
        bool isSubSet(const S9sVariantMap &superSet) const;

        S9sString 
            toJsonString(
                    const S9sFormatFlags &formatFlags) const;

        S9sString
            toJsonString(
                    int                   depth,
                    const S9sFormatFlags &formatFlags) const;


    protected:
        static const S9sVariant sm_invalid;

    private:
        S9sString toString(
                int                  depth, 
                const S9sVariantMap &variantMap) const;

        S9sString toString (
                int                   depth,
                const S9sVariantList &theList) const;

        S9sString toString (
                int               depth,
                const S9sVariant &value) const;

        S9sString indent(int depth) const;
        S9sString quote(const S9sString &s) const;
};


