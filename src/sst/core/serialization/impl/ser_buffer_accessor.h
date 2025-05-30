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

#ifndef SST_CORE_SERIALIZATION_IMPL_SER_BUFFER_ACCESSOR_H
#define SST_CORE_SERIALIZATION_IMPL_SER_BUFFER_ACCESSOR_H

#ifndef SST_INCLUDING_SERIALIZER_H
#warning \
    "The header file sst/core/serialization/impl/ser_buffer_accessor.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serializer.h"
#endif

#include "sst/core/warnmacros.h"

#include <cstring>
#include <exception>

namespace SST::Core::Serialization::pvt {

// class ser_buffer_overrun : public spkt_error {
class ser_buffer_overrun : public std::exception
{
public:
    explicit ser_buffer_overrun(int UNUSED(maxsize))
    // ser_buffer_overrun(int maxsize) :
    // spkt_error(sprockit::printf("serialization overrun buffer of size %d", maxsize))
    {}
};

class ser_buffer_accessor
{
public:
    template <class T>
    T* next()
    {
        T* ser_buffer = reinterpret_cast<T*>(bufptr_);
        bufptr_ += sizeof(T);
        size_ += sizeof(T);
        if ( size_ > max_size_ ) throw ser_buffer_overrun(max_size_);
        return ser_buffer;
    }

    char* next_str(size_t size)
    {
        char* ser_buffer = reinterpret_cast<char*>(bufptr_);
        bufptr_ += size;
        size_ += size;
        if ( size_ > max_size_ ) throw ser_buffer_overrun(max_size_);
        return ser_buffer;
    }

    size_t size() const { return size_; }

    size_t max_size() const { return max_size_; }

    void init(void* buffer, size_t size)
    {
        bufstart_ = reinterpret_cast<char*>(buffer);
        max_size_ = size;
        reset();
    }

    void clear()
    {
        bufstart_ = bufptr_ = nullptr;
        max_size_ = size_ = 0;
    }

    void reset()
    {
        bufptr_ = bufstart_;
        size_   = 0;
    }

protected:
    ser_buffer_accessor() :
        bufstart_(nullptr),
        bufptr_(nullptr),
        size_(0),
        max_size_(0)
    {}

protected:
    char*  bufstart_;
    char*  bufptr_;
    size_t size_;
    size_t max_size_;
};

} // namespace SST::Core::Serialization::pvt

#endif // SST_CORE_SERIALIZATION_IMPL_SER_BUFFER_ACCESSOR_H
