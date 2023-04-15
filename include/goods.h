#pragma once
#include <cstdint>
#include <memory>

#include "goods_type_enum.h"

class goods;

class goods : std::enable_shared_from_this<goods> {
private:
    uint64_t _uuid = 0;           // uuid
    uint32_t _id = 0;             // config id
    goods_type_enum _type;        // type
    uint32_t _overlap_max = 1;    // 最大叠加数量
public:
    goods() = default;
    virtual ~goods() = default;

    goods(uint64_t uuid_, uint32_t id_, goods_type_enum type_, uint32_t overlap_max_)
        : _uuid(uuid_)
        , _id(id_)
        , _type(type_)
        , _overlap_max(overlap_max_) {
    }

    uint64_t uuid() const {
        return _uuid;
    }
    void uuid(uint64_t uuid_) {
        _uuid = uuid_;
    }

    uint32_t id() const {
        return _id;
    }
    void id(uint32_t id_) {
        _id = id_;
    }

    uint32_t overlap_max() const {
        return _overlap_max;
    }

public:
    static std::shared_ptr<goods> create(uint64_t uuid_, uint32_t id_, goods_type_enum type_, uint32_t overlap_max_) {
        return std::make_shared<goods>(uuid_, id_, type_, overlap_max_);
    }
    static std::shared_ptr<goods> create(std::shared_ptr<goods> source) {
        return std::make_shared<goods>(source->_uuid, source->_id, source->_type, source->_overlap_max);
    }
};
