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
//

#include "sst_config.h"

#include "sst/core/serialization/serializable_base.h"

#include "sst/core/output.h"
#include "sst/core/serialization/statics.h"

#include <cstring>
#include <iostream>

namespace SST::Core::Serialization {

static need_delete_statics<serializable_factory> del_statics;
serializable_factory::builder_map*               serializable_factory::builders_ = nullptr;

void
serializable_base::serializable_abort(uint32_t line, const char* file, const char* func, const char* obj)
{
    SST::Output ser_abort("", 5, SST::Output::PrintAll, SST::Output::STDERR);
    ser_abort.fatal(line, file, func, 1, "ERROR: type %s should not be serialized\n", obj);
}

uint32_t
// serializable_factory::add_builder(serializable_builder* builder, uint32_t cls_id)
serializable_factory::add_builder(serializable_builder* builder, const char* name)
{
    if ( builders_ == nullptr ) {
        builders_ = new builder_map;
    }

    const char* key  = name;
    int         len  = ::strlen(key);
    uint32_t    hash = 0;
    for ( int i = 0; i < len; ++i ) {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    builder_map&           bmap    = *builders_;
    serializable_builder*& current = bmap[hash];
    if ( current != nullptr ) {
        // std::cerr << sprockit::printf(
        //   "amazingly %s and %s both hash to same serializable id %u",
        //   current->name(), builder->name(), hash) << std::endl;
        std::cerr << "amazingly " << current->name() << " and " << builder->name()
                  << " both hash to same serializable id " << hash << std::endl;
        abort();
    }
    current = builder;
    return hash;
}

void
serializable_factory::delete_statics()
{
    //  delete_vals(*builders_);
    delete builders_;
}

serializable_base*
serializable_factory::get_serializable(uint32_t cls_id)
{
    builder_map::const_iterator it = builders_->find(cls_id);
    if ( it == builders_->end() ) {
        std::cerr << "class id " << cls_id << " is not a valid serializable id" << std::endl;
        // spkt_throw_printf(value_error,
        //                  "class id %ld is not a valid serializable id",
        //                  cls_id);
    }
    serializable_builder* builder = it->second;
    return builder->build();
}

} // namespace SST::Core::Serialization
