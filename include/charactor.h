#pragma once
#include <string>
#include <utility>

#include "object.h"

class charactor : public object {
private:
    std::string _name;

    uint64_t _hp;
    uint64_t _max_hp;

public:
    charactor(uint64_t uuid_, std::string name_)
        : object(uuid_)
        , _name(std::move(name_)) {

    }
    virtual ~charactor() {
        _uuid = 0;
        _name.clear();
    }
};
