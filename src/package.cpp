#include "package.h"

#include <algorithm>
#include <cassert>

#include "goods.h"


std::string package_slot::debug_string() {
    return std::move(_goods ?
        std::to_string(_goods->id()).append(":").append(std::to_string(_count)) : "null");
}

bool package_slot::valid() {
    return _goods != nullptr;
}

void package_slot::to_empty() {
    _goods = nullptr;
    _count = 0;
}

bool package_slot::empty() const {
    return _count == 0;
}

bool package_slot::full() const {
    return _goods && _goods->overlap_max() == _count;
}

bool package_slot::same(goods_ptr pGoods) const {
    return pGoods && same(pGoods->id());
}

bool package_slot::same(uint32_t cfgId) const {
    return !empty() && _goods->id() == cfgId;
}

bool package_slot::can_filled(goods_ptr pGoods, bool overlap) const {
    if (empty()) return true;

    if (!overlap) return false;

    if (!same(pGoods)) return false;

    if (full()) return false;

    return true;
}

uint32_t package_slot::add(uint32_t count) {
    if (!valid() || full()) {
        return 0;
    }

    const auto last_cnt = _goods->overlap_max() - _count;
    if (last_cnt >= count) {
        _count += count;
        return count;
    }
    _count += last_cnt;
    return last_cnt;
}

uint32_t package_slot::sub(uint32_t count) {
    if (_count > count) {
        _count -= count;
        return count;
    }
    const auto result = _count;
    to_empty();
    return result;
}

uint32_t package_slot::set_to(goods_ptr pGoods, uint32_t count) {
    _goods = pGoods;
    return add(count);
}

package_operator::package_operator(package_ptr package) : _package(package) {
    assert(!_package->_operator_mark);
    _package->_operator_mark = true;

    // TODO: _transaction_id
    _backup_goods_slot = _package->_goods_slot;
    _backup_capacity_cur = _package->_capacity_cur;
    _backup_empty_slot_count = _package->_empty_slot_count;
    _backup_empty_slot_next = _package->_empty_slot_next;
}

package_operator::package_operator(package_ptr package, std::string&& transaction_mask) : _package(package) {
    assert(!_package->_operator_mark);
    _package->_operator_mark = true;

    // TODO: _transaction_id
    _backup_goods_slot = _package->_goods_slot;
    _backup_capacity_cur = _package->_capacity_cur;
    _backup_empty_slot_count = _package->_empty_slot_count;
    _backup_empty_slot_next = _package->_empty_slot_next;
}

package_operator::package_operator(package_ptr package, const std::string& transaction_mask) : _package(package) {
    assert(!_package->_operator_mark);
    _package->_operator_mark = true;

    // TODO: _transaction_id
    _backup_goods_slot = _package->_goods_slot;
    _backup_capacity_cur = _package->_capacity_cur;
    _backup_empty_slot_count = _package->_empty_slot_count;
    _backup_empty_slot_next = _package->_empty_slot_next;
}

package_operator::~package_operator() {
    release();
}

void package_operator::release() {
    if (_package) {
        rollback();

        _package->_operator_mark = false;
        _package = nullptr;

        _transaction_id.clear();
        _list.clear();
        _backup.clear();
        _backup_goods_slot.clear();
    }
}

uint32_t package_operator::put(goods_ptr pGoods, uint32_t goods_count, slot_id slot /*= INVALID_SLOT*/, bool overlap /*= true*/) {
    assert(_package);

    uint32_t result = 0;

    if (!pGoods) return result;
    if (goods_count == 0) return result;

    if (slot == INVALID_SLOT && !overlap) {
        slot = _package->get_empty_slot_id();
        if (slot == INVALID_SLOT)
            return result;
    }
    if (slot == INVALID_SLOT) {
        slot = 0;
    }

    while (goods_count > 0) {
        slot = _package->find_slot(pGoods, slot, overlap);

        auto pSlot = _package->get_slot(slot);
        if (pSlot == nullptr) {
            return result;
        }

        // backup
        backup_slot(slot);

        uint32_t filled = 0;
        if (pSlot->empty()) {
            _package->sub_empty_slot();
            _package->reset_empty_slot_next(slot);  // 先重置，下次再更新
            _package->add_goods_slot(pGoods->id(), slot);
            filled = pSlot->set_to(goods::create(pGoods), goods_count);
        }
        else {
            filled = pSlot->add(goods_count);
        }

        if (filled > 0) {
            _list.emplace_back(operator_info{ slot, package_operator::type::add, filled, pSlot->_goods, pSlot->_count });
        }

        goods_count -= filled;
        result += filled;
    }

    return result;
}

uint32_t package_operator::rem(uint32_t goods_id, uint32_t goods_count, slot_id slot) {
    assert(_package);

    uint32_t result = 0;

    if (goods_count == 0) return result;

    if (slot != INVALID_SLOT) {
        auto pSlot = _package->get_slot(slot);
        return inner_rem(goods_id, goods_count, slot, pSlot);
    }

    // 一次拷贝，循环内会操作这个容器
    auto slot_ids = _package->get_goods_slot(goods_id);
    for (auto& slot_id_ : slot_ids) {
        auto pSlot = _package->get_slot(slot_id_);
        if (pSlot == nullptr) {
            return result;     // !! 中间空格子 !!
        }
        
        auto sub_once = inner_rem(goods_id, goods_count, slot, pSlot);

        goods_count -= sub_once;
        result += sub_once;

        if (goods_count == 0) {
            break;
        }
    }

    return result;
}

bool package_operator::inner_swp(slot_id slot1, slot_id slot2, bool middle_modify) {
    assert(_package);

    if (slot1 == slot2 ||
        (slot1 >= _package->capacity_cur() || slot2 >= _package->capacity_cur())) {
        return false;
    }

    const auto pSlot1 = _package->get_slot(slot1);
    const auto pSlot2 = _package->get_slot(slot2);

    if (pSlot1 == nullptr || pSlot2 == nullptr) {
        return false;
    }

    if (pSlot1->empty() && pSlot2->empty()) {
        return true;
    }

    if (middle_modify) {
        backup_slot(slot1);
        backup_slot(slot2);
    }

    // merge
    if (!pSlot1->empty() && !pSlot2->empty() &&
        pSlot1->can_filled(pSlot2->_goods, true)) {

        pSlot2->sub(pSlot1->add(pSlot2->_count));

        if (middle_modify) {
            if (pSlot2->empty()) {
                _package->add_empty_slot();
                _package->set_empty_slot_next(slot2);
                _package->rem_goods_slot(pSlot1->_goods->id(), slot2);
            }
        }

        return true;
    }

    // swap
    if (middle_modify) {
        // 处理道具映射
        if (!pSlot1->empty())
            _package->rem_goods_slot(pSlot1->_goods->id(), slot1);
        if (!pSlot2->empty())
            _package->rem_goods_slot(pSlot2->_goods->id(), slot2);
    }

    if (_package->swap_slot(slot1, slot2)) {
        if (middle_modify) {
            // 处理道具映射 & 更新空格子
            if (!pSlot1->empty()) {
                _package->add_goods_slot(pSlot1->_goods->id(), slot1);
            }
            else {
                _package->set_empty_slot_next(slot1);
            }
            if (!pSlot2->empty()) {
                _package->add_goods_slot(pSlot2->_goods->id(), slot2);
            }
            else {
                _package->set_empty_slot_next(slot2);
            }
        }
        return true;
    }

    if (middle_modify) {
        // 恢复道具映射
        if (!pSlot1->empty())
            _package->add_goods_slot(pSlot1->_goods->id(), slot1);
        if (!pSlot2->empty())
            _package->add_goods_slot(pSlot2->_goods->id(), slot2);
    }

    return false;
}

bool package_operator::swp(slot_id slot1, slot_id slot2) {
    assert(_package);
    return inner_swp(slot1, slot2, true);
}

bool package_operator::aug(uint32_t inc) const {
    assert(_package);

    if (_package->capacity_cur() == _package->capacity_max()
        || _package->capacity_cur() + inc >= _package->capacity_max()) {
        return false;
    }

    _package->capacity_cur(_package->capacity_cur() + inc);
    _package->add_empty_slot(inc);
    _package->set_empty_slot_next(_package->capacity_cur() - 1);
    return true;
}

bool package_operator::auto_pack() {
    assert(_package);

    // 合并
    _package->for_each_slot(0, [this](slot_id slot, package_slot* pSlot) -> bool {
        if (pSlot == nullptr) {
            // TODO: log error
            return false;   // break
        }
        if (pSlot->empty() || pSlot->full()) {
            return true;
        }
        _package->for_each_slot(slot + 1, [this, slot, pSlot](slot_id slot_next, package_slot* pSlot_next) -> bool {
            if (pSlot_next == nullptr) {
                // TODO: log error
                return false;  // break
            }
            if (pSlot_next->empty() || pSlot_next->full()) {
                return true;
            }
            if (!pSlot->same(pSlot_next->_goods)) {
                return true;
            }
            this->inner_swp(slot, slot_next, false);
            if (pSlot->full()) {
                return false;    // break;
            }
            return true;
            }
        );
        return true;
        }
    );

    // 排序
    auto& slot_array = _package->__get_slot_array();
    auto capacity = _package->capacity_cur();
    if (capacity <= 1) 
        return true;
    std::sort(std::begin(slot_array), std::begin(slot_array) + capacity,
        [this](const package_slot& lhs, const package_slot& rhs) -> bool {
            if (lhs.empty() && rhs.empty()) return false;
            if (lhs.empty()) return false;
            if (rhs.empty()) {
                return true;
            }
            return lhs._goods->id() < rhs._goods->id()
                || (lhs._goods->id() == rhs._goods->id() && lhs._count < rhs._count);
        }
    );

    _package->re_init();

    return true;
}

package_operator& package_operator::commit() {
    assert(_package);

    _backup.clear();
    _backup_goods_slot = _package->_goods_slot;
    _backup_capacity_cur = _package->_capacity_cur;
    _backup_empty_slot_count = _package->_empty_slot_count;
    _backup_empty_slot_next = _package->_empty_slot_next;

    return *this;
}

package_operator& package_operator::rollback() {
    assert(_package);

    for (const auto& iter : _backup) {
        _package->cover_slot(iter.first, &iter.second);
    }

    _package->_capacity_cur = _backup_capacity_cur;
    _package->_empty_slot_count = _backup_empty_slot_count;
    _package->_empty_slot_next = _backup_empty_slot_next;

    _package->_goods_slot = _backup_goods_slot;

    _backup.clear();
    _list.clear();

    return *this;
}

void package_operator::notify() {
    assert(_package);

    // TODO: notify
    for (const auto& iter : _list) {

    }
    _list.clear();
}

void package_operator::backup_slot(slot_id slot) {
    assert(_package);

    auto pSlot = _package->get_slot(slot);
    if (pSlot == nullptr) return;
    if (_backup.find(slot) == _backup.end()) {
        // 这里用拷贝的方式!!
        _backup.insert(std::make_pair(slot, *pSlot));
    }
}

uint32_t package_operator::inner_rem(uint32_t goods_id, uint32_t goods_count, slot_id slot, package_slot* pSlot) {
    assert(_package);

    if (pSlot == nullptr)
        return 0;
    if (!pSlot->same(goods_id))
        return 0;

    backup_slot(slot);
    
    auto goods_bak = pSlot->_goods;

    uint32_t subed = pSlot->sub(goods_count);
    if (subed > 0 && pSlot->empty()) {
        _package->add_empty_slot();
        _package->set_empty_slot_next(slot);
        _package->rem_goods_slot(goods_id, slot);
        pSlot->to_empty();
    }

    if (subed > 0) {
        _list.emplace_back(operator_info{ slot, package_operator::type::sub, subed, goods_bak, pSlot->_count });
    }

    return subed;
}

package::package(object* owner_, package_type_enum type_, uint32_t capacity_max_)
    : _owner(owner_)
    , _type(type_)
    , _capacity_max(capacity_max_) {

    _slot_array.resize(capacity_max_);
}

package::~package() {
    _owner = nullptr;
    _type = package_type_enum::normal;
    _capacity_cur = 0;
    _capacity_max = 0;
    _slot_array.clear();
    _empty_slot_count = 0;
    _empty_slot_next = INVALID_SLOT;
    _goods_slot.clear();
}

bool package::re_init() {

    _goods_slot.clear();
    _empty_slot_count = 0;
    _empty_slot_next = INVALID_SLOT;

    for (slot_id one = 0; one < _capacity_cur; ++one) {
        auto& slot_ref = _slot_array[one];
        if (!slot_ref.empty()) {
            _goods_slot[slot_ref._goods->id()].emplace(one);
            continue;
        }
        _empty_slot_count += 1;
        set_empty_slot_next(one);
    }
    return true;
}

void package::auto_pack() {

    assert(!_operator_mark);

    package_operator oper(this);
    oper.auto_pack();
    oper.commit();
}

void package::for_each_slot(std::function<bool(package_slot*)>&& caller) {
    using __Caller = std::function<bool(package_slot*)>;
    for_each_slot(0, std::forward<__Caller>(caller));
}

void package::for_each_slot(std::function<bool(slot_id, package_slot*)>&& caller) {
    using __Caller = std::function<bool(slot_id, package_slot*)>;
    for_each_slot(0, std::forward<__Caller>(caller));
}

void package::for_each_slot(slot_id start, std::function<bool(package_slot*)>&& caller) {
    for (slot_id one = start; one < _capacity_cur; ++one) {
        if (!caller(&_slot_array[one]))
            break;
    }
}

void package::for_each_slot(slot_id start, std::function<bool(slot_id, package_slot*)>&& caller) {
    for (slot_id one = start; one < _capacity_cur; ++one) {
        if (!caller(one, &_slot_array[one]))
            break;
    }
}

void package::reset_empty_slot_next(slot_id slot) {
    if (slot == _empty_slot_next) _empty_slot_next = INVALID_SLOT;
    static constexpr slot_id max_re_get_count = 11;
    slot_id retry = 1;
    slot_id offset = 0;
    while (retry < max_re_get_count) {
        slot_id next = slot + retry;
        if (next >= _capacity_cur) {
            next = offset;
            ++offset;
        }
        if (_slot_array[next].empty()) {
            _empty_slot_next = next;
            break;
        }
        ++retry;
    }
}

const std::set<slot_id>& package::get_goods_slot(uint32_t goods_id) {
    static const std::set<slot_id> empty_result;
    auto iter = _goods_slot.find(goods_id);
    if (iter != _goods_slot.end()) 
        return iter->second;
    return empty_result;
}

void package::add_goods_slot(uint32_t goods_id, slot_id slot) {
    auto iter = _goods_slot.find(goods_id);
    if (iter == _goods_slot.end()) {
        iter = _goods_slot.emplace(goods_id, std::set<slot_id>()).first;
    }
    iter->second.emplace(slot);
}

void package::rem_goods_slot(uint32_t goods_id, slot_id slot) {

    auto iter = _goods_slot.find(goods_id);
    if (iter != _goods_slot.end())
        iter->second.erase(slot);
}

slot_id package::find_slot_existing(goods_ptr pGoods, bool overlap) {
    const auto& slots = get_goods_slot(pGoods->id());
    for (const auto& one : slots) {
        if (_slot_array[one].can_filled(pGoods, overlap))
            return one;
    }
    return INVALID_SLOT;
}

slot_id package::find_slot(goods_ptr pGoods, slot_id start, bool overlap) {

    if (start <= _empty_slot_next 
        && _empty_slot_next < _capacity_cur 
        && _slot_array[_empty_slot_next].can_filled(pGoods, overlap)) {
        return _empty_slot_next;
    }

    for (slot_id one = start; one < _capacity_cur; ++one) {
        if (_slot_array[one].can_filled(pGoods, overlap))
            return one;
    }
    return INVALID_SLOT;
}
