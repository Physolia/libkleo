/* -*- mode: c++; c-basic-offset:4 -*-
    models/predicates.h

    This file is part of Kleopatra, the KDE keymanager
    SPDX-FileCopyrightText: 2007 Klarälvdalens Datakonsult AB

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <gpgme++/key.h>

#include <algorithm>
#include <cstring>
#include <functional>
#include <iterator>
#include <string>

namespace Kleo
{
namespace _detail
{

inline int mystrcmp(const char *s1, const char *s2)
{
    using namespace std;
    return s1 ? s2 ? strcmp(s1, s2) : 1 : s2 ? -1 : 0;
}

#define make_comparator_str_impl(Name, expr, cmp)                                                                                                              \
    template<template<typename U> class Op>                                                                                                                    \
    struct Name {                                                                                                                                              \
        typedef bool result_type;                                                                                                                              \
                                                                                                                                                               \
        bool operator()(const char *lhs, const char *rhs) const                                                                                                \
        {                                                                                                                                                      \
            return Op<int>()(cmp, 0);                                                                                                                          \
        }                                                                                                                                                      \
                                                                                                                                                               \
        bool operator()(const std::string &lhs, const std::string &rhs) const                                                                                  \
        {                                                                                                                                                      \
            return operator()(lhs.c_str(), rhs.c_str());                                                                                                       \
        }                                                                                                                                                      \
        bool operator()(const char *lhs, const std::string &rhs) const                                                                                         \
        {                                                                                                                                                      \
            return operator()(lhs, rhs.c_str());                                                                                                               \
        }                                                                                                                                                      \
        bool operator()(const std::string &lhs, const char *rhs) const                                                                                         \
        {                                                                                                                                                      \
            return operator()(lhs.c_str(), rhs);                                                                                                               \
        }                                                                                                                                                      \
                                                                                                                                                               \
        template<typename T>                                                                                                                                   \
        bool operator()(const T &lhs, const T &rhs) const                                                                                                      \
        {                                                                                                                                                      \
            return operator()((lhs expr), (rhs expr));                                                                                                         \
        }                                                                                                                                                      \
        template<typename T>                                                                                                                                   \
        bool operator()(const T &lhs, const char *rhs) const                                                                                                   \
        {                                                                                                                                                      \
            return operator()((lhs expr), rhs);                                                                                                                \
        }                                                                                                                                                      \
        template<typename T>                                                                                                                                   \
        bool operator()(const char *lhs, const T &rhs) const                                                                                                   \
        {                                                                                                                                                      \
            return operator()(lhs, (rhs expr));                                                                                                                \
        }                                                                                                                                                      \
        template<typename T>                                                                                                                                   \
        bool operator()(const T &lhs, const std::string &rhs) const                                                                                            \
        {                                                                                                                                                      \
            return operator()((lhs expr), rhs);                                                                                                                \
        }                                                                                                                                                      \
        template<typename T>                                                                                                                                   \
        bool operator()(const std::string &lhs, const T &rhs) const                                                                                            \
        {                                                                                                                                                      \
            return operator()(lhs, (rhs expr));                                                                                                                \
        }                                                                                                                                                      \
    }

#define make_comparator_str_fast(Name, expr) make_comparator_str_impl(Name, expr, _detail::mystrcmp(lhs, rhs))
#define make_comparator_str(Name, expr) make_comparator_str_impl(Name, expr, qstricmp(lhs, rhs))

make_comparator_str_fast(ByFingerprint, .primaryFingerprint());
make_comparator_str_fast(ByKeyID, .keyID());
make_comparator_str_fast(ByShortKeyID, .shortKeyID());
make_comparator_str_fast(ByChainID, .chainID());
make_comparator_str_fast(ByKeyGrip, .keyGrip());

template<typename T>
void sort_by_fpr(T &t)
{
    std::sort(t.begin(), t.end(), ByFingerprint<std::less>());
}

template<typename T>
void remove_duplicates_by_fpr(T &t)
{
    t.erase(std::unique(t.begin(), t.end(), ByFingerprint<std::equal_to>()), t.end());
}

template<typename T>
T union_by_fpr(const T &t1, const T &t2)
{
    T result;
    result.reserve(t1.size() + t2.size());
    std::set_union(t1.begin(), //
                   t1.end(),
                   t2.begin(),
                   t2.end(),
                   std::back_inserter(result),
                   ByFingerprint<std::less>());
    return result;
}
}
}
