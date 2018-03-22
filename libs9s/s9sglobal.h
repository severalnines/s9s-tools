/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#pragma once

typedef unsigned long int ulong;
typedef unsigned long long int ulonglong;
typedef unsigned int uint;
typedef long long int longlong;

/**
 * Use this to avoid unused variable warnings.
 */
template <typename T>
inline void s9sUnused(T &x) { (void)x; }

#define S9S_UNUSED(x) s9sUnused(x);

namespace S9s
{
    enum Syntax
    {
        GenericConfigSyntax,
        MySqlConfigSyntax,
        HaProxyConfigSyntax,
        YamlSyntax,
        UnknownSyntax,
        MongoConfigSyntax = YamlSyntax
    };

    enum AddressType 
    {
        PublicIpv4Address,
        PrivateIpv4Address,
        PublicDnsName,
        PrivateDnsName,
    };
};

/**
 * This is the constant version of the foreach().
 */
template <typename T>
class _ForeachContainer {
public:
    inline _ForeachContainer(const T& t) : 
        c(t), brk(0), i(c.begin()), e(c.end()) 
    { 
    }
    const T c;
    int brk;
    typename T::const_iterator i, e;
};

#define foreach(variable, container)                                \
for (_ForeachContainer<__typeof__(container)> _container_(container); \
     !_container_.brk && _container_.i != _container_.e;              \
     __extension__  ({ ++_container_.brk; ++_container_.i; }))                       \
    for (variable = _container_.i->second;; __extension__ ({--_container_.brk; break;}))
