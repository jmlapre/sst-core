// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_ELI_SIMPLE_INFO_H
#define SST_CORE_ELI_SIMPLE_INFO_H

#include "sst/core/eli/elibase.h"

#include <type_traits>

namespace SST::ELI {

// ProvidesSimpleInfo is a class to quickly add ELI info to an ELI
// Base API.  This class should only be used for APIs that aren't
// reported in sst-info, since you can't override the plain text and
// XML printing functions.


// Class used to differentiate the different versions of ELI_getSimpleInfo
template <int num, typename InfoType>
struct SimpleInfoPlaceHolder
{};

//  Class to check for an ELI_getSimpleInfo Function. The std::void_t<
//  decltype()> way to detect a member function's type doesn't seem to
//  work for functions with specific type signatures and we need to
//  differentiate between the various versions using the first
//  parameter to the function (index + type).
template <class T, int index, class InfoType>
class checkForELI_getSimpleInfoFunction
{
    template <typename F, F>
    struct check;

    using Match    = char;
    using NotMatch = long;

    using functionsig = const InfoType& (*)(SimpleInfoPlaceHolder<index, InfoType>);

    template <typename F>
    static Match HasFunction(check<functionsig, &F::ELI_getSimpleInfo>*);

    template <typename F>
    static NotMatch HasFunction(...);

public:
    static bool const value = (sizeof(HasFunction<T>(0)) == sizeof(Match));
};

template <class T, int index, class InfoType>
inline constexpr bool checkForELI_getSimpleInfoFunction_v =
    checkForELI_getSimpleInfoFunction<T, index, InfoType>::value;

// Actual functions that use checkForELI_getSimpleInfoFunction class
// to create functions to get the information from the class
template <class T, int index, class InfoType>
std::enable_if_t<checkForELI_getSimpleInfoFunction_v<T, index, InfoType>, const InfoType&>
ELI_templatedGetSimpleInfo()
{
    return T::ELI_getSimpleInfo(SimpleInfoPlaceHolder<index, InfoType>());
}

template <class T, int index, class InfoType>
std::enable_if_t<!checkForELI_getSimpleInfoFunction_v<T, index, InfoType>, const InfoType&>
ELI_templatedGetSimpleInfo()
{
    static InfoType var;
    return var;
}

// Class that lets you add ELI information to an ELI API.  This class
// can be used with any type/class that can be initialized using list
// initialization (x = {}).  You have to provide an index as well as
// the type to be store so that the templating system can
// differentiate between different items of the same type.  It would
// be cool if we could template on a string instead so they could be
// named, but that doesn't seem to work in c++ 11.
template <int num, typename InfoType>
class ProvidesSimpleInfo
{
public:
    const InfoType& getSimpleInfo() const { return info_; }

protected:
    template <class T>
    explicit ProvidesSimpleInfo(T* UNUSED(t)) :
        info_(ELI_templatedGetSimpleInfo<T, num, InfoType>())
    {}

private:
    InfoType info_;
};

} // namespace SST::ELI

// Macro used by the API to create macros to populate the added ELI
// info
#define SST_ELI_DOCUMENT_SIMPLE_INFO(type, index, ...)                                           \
    static const type& ELI_getSimpleInfo(SST::ELI::SimpleInfoPlaceHolder<index, type> UNUSED(a)) \
    {                                                                                            \
        static type my_info = { __VA_ARGS__ };                                                   \
        return my_info;                                                                          \
    }

#endif // SST_CORE_ELI_SIMPLE_INFO_H
