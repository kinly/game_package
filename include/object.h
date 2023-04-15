#pragma once
#include <cstdint>

#include "package.h"

static constexpr uint32_t normal_package_capacity = 100;
static constexpr uint32_t store_package_capacity = 100;

class object {
protected:
    uint64_t _uuid = 0;       // uuid
    package _normal_package;
    package _store_package;

public:
    object(uint64_t uuid_)
        : _uuid(uuid_)
        , _normal_package(this, package_type_enum::normal, normal_package_capacity) 
        , _store_package(this, package_type_enum::store, store_package_capacity) {

        _normal_package.capacity_cur(10);
        _store_package.capacity_cur(100);
    }
    virtual ~object() = default;

    package* normal_package() {
        return &_normal_package;
    }
    package* store_package() {
        return &_store_package;
    }
};
