#pragma once
#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <list>

#include "package_type_enum.h"

class object;

class goods;
using goods_ptr = std::shared_ptr<goods>;   // 道具智能指针

class package_slot;
class package;
using package_ptr = package*;

using slot_id = uint32_t;
static constexpr slot_id INVALID_SLOT = 0xFFFF;   // 标记无效的格子

/// <summary>
/// 背包格子
/// </summary>
class package_slot final {

public:
    goods_ptr   _goods = nullptr;  // 对象
    uint32_t    _count = 0;        // 数量

public:
    std::string debug_string();
public:
    explicit package_slot()
        : _goods(nullptr)
        , _count(0) {
    }

    package_slot(const package_slot& slot)
        : _goods(slot._goods)
        , _count(slot._count) {
        
    }

    ~package_slot() {
        _goods = nullptr;
        _count = 0;
    }

    package_slot& operator = (const package_slot& src) = default;

    /// <summary>
    /// 格子是否有效（用于 empty -> fill）
    /// </summary>
    /// <returns>是否有效</returns>
    bool valid();

    /// <summary>
    /// 设置为空格子(清除goods & count)
    /// </summary>
    void to_empty();

    /// <summary>
    /// 是否为空格子
    /// </summary>
    /// <returns>是否空</returns>
    bool empty() const;

    /// <summary>
    /// 是否已放满
    /// </summary>
    /// <returns>是否满</returns>
    bool full() const;

    /// <summary>
    /// 格子内物品是否相同
    /// </summary>
    /// <param name="pGoods">物品对象</param>
    /// <returns>是否相同</returns>
    bool same(goods_ptr pGoods) const;

    /// <summary>
    /// 格子内物品是否相同
    /// </summary>
    /// <param name="cfgId">物品配置ID</param>
    /// <returns>是否相同</returns>
    bool same(uint32_t cfgId) const;

    /// <summary>
    /// 是否可填充到这个格子里
    /// </summary>
    /// <param name="pGoods">物品对象</param>
    /// <param name="overlap">是否可叠加</param>
    /// <returns>是否可填充</returns>
    bool can_filled(goods_ptr pGoods, bool overlap) const;

    /// <summary>
    /// add
    /// </summary>
    /// <param name="count">数量</param>
    /// <returns>放了几个</returns>
    uint32_t add(uint32_t count);

    /// <summary>
    /// sub
    /// </summary>
    /// <param name="count">数量</param>
    /// <returns>删了几个</returns>
    uint32_t sub(uint32_t count);

    /// <summary>
    /// set
    /// </summary>
    /// <param name="pGoods">物品对象</param>
    /// <param name="count">物品数量</param>
    /// <returns></returns>
    uint32_t set_to(goods_ptr pGoods, uint32_t count);
};

class package_operator {
public:
    enum class type : uint32_t {
        get,
        add,
        sub,
    };

    struct operator_info {
        slot_id _slot;            // 格子index
        enum type _type;          // 操作类型

        uint32_t      _count;     // 操作数量
        goods_ptr     _goods;     // 操作道具
        uint32_t _after_count;    // 操作后数量
    };
private:
    std::string _transaction_id;          // 事务ID
    package_ptr _package = nullptr;       // 背包
    std::list<operator_info> _list;       // 操作过程

    //////////////////////////////////////////////////////////////////////////
    // history backup
    std::unordered_map<slot_id, package_slot> _backup;                   // 被操作前的格子内容 slot_id, slot
    std::unordered_map<uint32_t, std::set<slot_id>> _backup_goods_slot;  // 被操作前的物品配置id->格子
    uint32_t _backup_capacity_cur = 0;                                   // 被操作前的容量
    uint32_t _backup_empty_slot_count = 0;                               // 被操作前的空格子数量
    slot_id  _backup_empty_slot_next = INVALID_SLOT;                     // 被操作前的下一个空格子
    //////////////////////////////////////////////////////////////////////////
    
public:
    explicit package_operator(package_ptr package);

    explicit package_operator(package_ptr package, std::string&& transaction_mask);

    explicit package_operator(package_ptr package, const std::string& transaction_mask);

    virtual ~package_operator();

    void release();

    // !! non copyable 
    package_operator() = delete;
    package_operator(const package_operator&) = delete;
    package_operator& operator = (const package_operator&) = delete;

    /// <summary>
    /// 添加物品
    /// </summary>
    /// <param name="goods">物品对象</param>
    /// <param name="goods_count">添加数量</param>
    /// <param name="slot">格子(无效格子表示由系统查找)</param>
    /// <param name="overlap">是否叠加</param>
    /// <returns>添加了几个</returns>
    uint32_t put(goods_ptr pGoods, uint32_t goods_count, slot_id slot = INVALID_SLOT, bool overlap = true);

    /// <summary>
    /// 扣除物品
    /// </summary>
    /// <param name="goods_id">道具配置ID</param>
    /// <param name="goods_count">扣除数量</param>
    /// <param name="slot">具体格子(若有效则只扣除这一个格子，若无效则遍历背包)</param>
    /// <returns>扣除了几个</returns>
    uint32_t rem(uint32_t goods_id, uint32_t goods_count, slot_id slot = INVALID_SLOT);

    /// <summary>
    /// 交换物品（相同道具可merge则从2 merge to 1）
    /// </summary>
    /// <param name="slot1">格子index</param>
    /// <param name="slot2">格子index</param>
    /// <returns>是否成功</returns>
    bool swp(slot_id slot1, slot_id slot2);

    /// <summary>
    /// 扩容
    /// </summary>
    /// <param name="inc">扩容增量</param>
    /// <returns>是否成功</returns>
    bool aug(uint32_t inc = 1) const;

    /// <summary>
    /// 提交
    /// </summary>
    /// <returns>自身引用，建议链式调用release</returns>
    package_operator& commit();

    /// <summary>
    /// 回滚
    /// </summary>
    /// <returns>自身引用，建议链式调用release</returns>
    package_operator& rollback();

    /// <summary>
    /// 通知
    /// </summary>
    virtual void notify();

private:

    friend class package;

    /// <summary>
    /// 整理背包
    /// </summary>
    /// <returns>是否成功</returns>
    bool auto_pack();

    /// <summary>
    /// 备份格子
    /// </summary>
    /// <param name="slot">格子index</param>
    void backup_slot(slot_id slot);

    /// <summary>
    /// 对指定格子扣除物品
    /// </summary>
    /// <param name="goods_id">物品配置ID</param>
    /// <param name="goods_count">扣除数量</param>
    /// <param name="slot">格子index</param>
    /// <param name="pSlot">格子对象</param>
    /// <returns>实际扣除数量</returns>
    uint32_t inner_rem(uint32_t goods_id, uint32_t goods_count, slot_id slot, package_slot* pSlot);

    /// <summary>
    /// 交换物品（相同道具可merge则从2 merge to 1）
    /// </summary>
    /// <param name="slot1">格子index</param>
    /// <param name="slot2">格子index</param>
    /// <param name="middle_modify">是否记录变更（auto_pack 不记录）</param>
    /// <returns>是否成功</returns>
    bool inner_swp(slot_id slot1, slot_id slot2, bool middle_modify);
};

class package {
protected:
    object* _owner;
private:
    package_type_enum _type = package_type_enum::normal;  // 背包类型
    uint32_t _capacity_max = 0;                           // 最大背包容量
    uint32_t _capacity_cur = 0;                           // 当前背包容量
    std::vector<package_slot> _slot_array;                // 背包格子 size() == _capacity_max

    uint32_t _empty_slot_count = 0;                       // empty slot 数量 （快速检查使用）
    slot_id  _empty_slot_next = INVALID_SLOT;             // empty slot, change at consume/throw （快速检查使用）

    std::unordered_map<uint32_t, std::set<slot_id>> _goods_slot;  // 物品配置id->格子

public:
    package(object* owner_, package_type_enum type_, uint32_t capacity_max_);
    virtual ~package();

    // !! non copyable 
    package() = delete;
    package(const package&) = delete;
    package& operator = (const package&) = delete;

    /// <summary>
    /// 初始化 _empty_slot_count, _empty_slot_next, _goods_slot
    /// </summary>
    /// <returns>是否成功</returns>
    bool re_init();

    package_type_enum type_enum() const {
        return _type;
    }

    uint32_t capacity_cur() const {
        return _capacity_cur;
    }

    void capacity_cur(uint32_t cur) {
        _capacity_cur = cur;
    }

    uint32_t capacity_max() const {
        return _capacity_max;
    }

    uint32_t empty_slot_count() const {
        return _empty_slot_count;
    }

    slot_id empty_slot_next() const {
        return _empty_slot_next;
    }

    package_slot* get_slot(slot_id slot) {
        if (slot >= _capacity_cur) return nullptr;
        return &_slot_array.at(slot);
    }

    slot_id get_empty_slot_id() const {
        if (_empty_slot_next != INVALID_SLOT 
            && _slot_array.at(_empty_slot_next).empty()) {
            return _empty_slot_next;
        }

        for (slot_id i = 0; i < _capacity_cur; ++i) {
            if (_slot_array.at(i).empty()) {
                return i;
            }
        }
        return INVALID_SLOT;
    }

    /// <summary>
    /// 自动整理（严格限制，不能用在未完成的operator中间使用）
    /// </summary>
    void  auto_pack();

    /// <summary>
    /// 遍历格子
    /// </summary>
    /// <param name="caller">执行函数. false 返回值停止（break）</param>
    void for_each_slot(std::function<bool(package_slot*)>&&);
    void for_each_slot(std::function<bool(slot_id, package_slot*)>&&);

    void for_each_slot(slot_id, std::function<bool(package_slot*)>&&);
    void for_each_slot(slot_id, std::function<bool(slot_id, package_slot*)>&&);

private:
    friend class package_operator;

    std::atomic<bool> _operator_mark;     // 操作中的标记

    /// <summary>
    /// 交换格子内容
    /// </summary>
    /// <param name="slot1"></param>
    /// <param name="slot2"></param>
    bool swap_slot(slot_id slot1, slot_id slot2) {
        if (slot1 >= _capacity_cur || slot2 >= _capacity_cur)
            return false;
        std::swap(_slot_array[slot1], _slot_array[slot2]);
        return true;
    }

    /// <summary>
    /// 覆盖指定格子
    /// </summary>
    /// <param name="index">格子ID</param>
    /// <param name="slot">格子内容</param>
    void cover_slot(slot_id index, const package_slot* slot) {
        if (index < _capacity_max) {
            _slot_array[index] = *slot;
        }
    }

    void add_empty_slot() {
        _empty_slot_count = std::min(_capacity_cur, ++_empty_slot_count);
    }

    void add_empty_slot(uint32_t inc) {
        _empty_slot_count = std::min(_capacity_cur, _empty_slot_count + inc);
    }
    void sub_empty_slot() {
        if (_empty_slot_count > 0) --_empty_slot_count;
    }
    /// <summary>
    /// 设置空格子标记
    /// </summary>
    void set_empty_slot_next(slot_id slot) {
        _empty_slot_next = std::min(_empty_slot_next, slot);
    }
    /// <summary>
    /// 重置空格子标记（带从头回溯概念）
    /// </summary>
    /// <param name="slot">格子index</param>
    void reset_empty_slot_next(slot_id slot);
    /// <summary>
    /// 获取已有物品所在格子信息
    /// </summary>
    /// <param name="goods_id"></param>
    /// <returns></returns>
    const std::set<slot_id>& get_goods_slot(uint32_t goods_id);

    /// <summary>
    /// 添加道具对应格子标记
    /// </summary>
    void add_goods_slot(uint32_t goods_id, slot_id slot);
    /// <summary>
    /// 移除道具对应格子标记
    /// </summary>
    void rem_goods_slot(uint32_t goods_id, slot_id slot);

    /// <summary>
    /// 从已经有该道具的格子找
    /// </summary>
    /// <param name="pGoods">物品对象</param>
    /// <param name="overlap">叠加</param>
    /// <returns>格子ID</returns>
    slot_id find_slot_existing(goods_ptr pGoods, bool overlap);
    /// <summary>
    /// 从指定开始位置遍历找格子
    /// </summary>
    /// <param name="pGoods">物品对象</param>
    /// <param name="start">开始格子</param>
    /// <param name="overlap">叠加</param>
    /// <returns>格子ID</returns>
    slot_id find_slot(goods_ptr pGoods, slot_id start, bool overlap);

    /// <summary>
    /// 全部格子（暂时只给auto_pack使用）
    /// </summary>
    std::vector<package_slot>& __get_slot_array() {
        return _slot_array;
    }
};

