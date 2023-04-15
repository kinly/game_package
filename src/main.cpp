#include <cassert>
#include <iostream>

#include "goods.h"
#include "goods_type_enum.h"
#include "object.h"
#include "package.h"
#include "util.h"

int main(int argc, char* argv[]) {

    auto uuid = [](uint32_t src) -> uint64_t {
        return 100000ull + src;
    };

    object* pUser_1001 = new object(1001);
    assert(pUser_1001);

    std::unordered_map<uint32_t, goods_ptr> __goods = {
        {1, goods::create(uuid(1), 1, goods_type_enum::item, 99)},
        {2, goods::create(uuid(2), 2, goods_type_enum::item, 1)},
        {3, goods::create(uuid(3), 3, goods_type_enum::item, 99)},
        {4, goods::create(uuid(4), 4, goods_type_enum::item, 99)},
        {5, goods::create(uuid(5), 5, goods_type_enum::item, 99)},
        {6, goods::create(uuid(6), 6, goods_type_enum::item, 99)},
        {7, goods::create(uuid(7), 7, goods_type_enum::item, 99)},
        {8, goods::create(uuid(8), 8, goods_type_enum::item, 99)},
        {9, goods::create(uuid(9), 9, goods_type_enum::item, 99)},
    };

    auto slot_cout = [](slot_id slot, package_slot* pSlot) -> bool {
        if (slot % 5 == 0)
            std::cout << "\n";
        std::cout << util::inner_string("slot: ", slot, " -> ", pSlot->debug_string(), "\t");
        return true;
    };

    pUser_1001->normal_package()->for_each_slot(slot_cout);
    std::cout << pUser_1001->store_package()->empty_slot_next() << ":" << pUser_1001->store_package()->empty_slot_count() << std::endl;

    package_operator operation(pUser_1001->normal_package());
    assert(operation.put(__goods[1], 100) == 100);

    assert(operation.put(__goods[1], 50, 2) == 50);

    std::cout << std::endl << "after put 1 [100]----------------------------------------------" << std::endl;
    pUser_1001->normal_package()->for_each_slot(slot_cout);
    std::cout << pUser_1001->normal_package()->empty_slot_next() << ":" << pUser_1001->normal_package()->empty_slot_count() << std::endl;

    assert(operation.put(__goods[2], 90, 5) == 5);

    assert(operation.put(__goods[1], 1, 3) == 1);

    std::cout << std::endl << "after put 2 [90]----------------------------------------------" << std::endl;
    pUser_1001->normal_package()->for_each_slot(slot_cout);
    std::cout << pUser_1001->normal_package()->empty_slot_next() << ":" << pUser_1001->normal_package()->empty_slot_count() << std::endl;

    // pUser_1001->normal_package()->auto_pack();
    //
    // std::cout << std::endl << "after auto_pack----------------------------------------------" << std::endl;
    // pUser_1001->normal_package()->for_each_slot(slot_cout);
    // std::cout << pUser_1001->normal_package()->empty_slot_next() << ":" << pUser_1001->normal_package()->empty_slot_count() << std::endl;

    assert(operation.rem(1, 99));

    std::cout << std::endl << "after rem 1 [99]----------------------------------------------" << std::endl;
    pUser_1001->normal_package()->for_each_slot(slot_cout);
    std::cout << pUser_1001->normal_package()->empty_slot_next() << ":" << pUser_1001->normal_package()->empty_slot_count() << std::endl;

    // assert(operation.swp(0, 1));

    std::cout << std::endl << "----------------------------------------------" << std::endl;
    pUser_1001->normal_package()->for_each_slot(slot_cout);
    std::cout << pUser_1001->normal_package()->empty_slot_next() << ":" << pUser_1001->normal_package()->empty_slot_count() << std::endl;

    assert(operation.put(__goods[1], 100) == 100);

    std::cout << std::endl << "----------------------------------------------" << std::endl;
    pUser_1001->normal_package()->for_each_slot(slot_cout);
    std::cout << pUser_1001->normal_package()->empty_slot_next() << ":" << pUser_1001->normal_package()->empty_slot_count() << std::endl;

    assert(operation.swp(0, 1));

    operation.commit().release();

    std::cout << std::endl << "----------------------------------------------" << std::endl;
    pUser_1001->normal_package()->for_each_slot(slot_cout);
    std::cout << pUser_1001->normal_package()->empty_slot_next() << ":" << pUser_1001->normal_package()->empty_slot_count() << std::endl;

    pUser_1001->normal_package()->auto_pack();

    std::cout << std::endl << "after auto_pack----------------------------------------------" << std::endl;
    pUser_1001->normal_package()->for_each_slot(slot_cout);
    std::cout << pUser_1001->normal_package()->empty_slot_next() << ":" << pUser_1001->normal_package()->empty_slot_count() << std::endl;

    {
        // from normal package to store
        package_operator op_nor(pUser_1001->normal_package());
        package_operator op_sto(pUser_1001->store_package());

        auto nor_slot_ptr = pUser_1001->normal_package()->get_slot(0);
        assert(nor_slot_ptr != nullptr && !nor_slot_ptr->empty());
        auto backup_slot = *nor_slot_ptr;
        op_nor.rem(nor_slot_ptr->_goods->id(), nor_slot_ptr->_count, 0);
        op_sto.put(backup_slot._goods, backup_slot._count, 1);

        std::cout << std::endl << "----------------------------------------------" << std::endl;
        pUser_1001->normal_package()->for_each_slot(slot_cout);
        std::cout << pUser_1001->normal_package()->empty_slot_next() << ":" << pUser_1001->normal_package()->empty_slot_count() << std::endl;

        std::cout << std::endl << "store----------------------------------------------" << std::endl;
        pUser_1001->store_package()->for_each_slot(slot_cout);
        std::cout << pUser_1001->store_package()->empty_slot_next() << ":" << pUser_1001->store_package()->empty_slot_count() << std::endl;

        op_nor.commit();
        op_sto.commit();
    }

    std::cout << std::endl << "----------------------------------------------" << std::endl;
    pUser_1001->normal_package()->for_each_slot(slot_cout);
    std::cout << pUser_1001->normal_package()->empty_slot_next() << ":" << pUser_1001->normal_package()->empty_slot_count() << std::endl;

    std::cout << std::endl << "store----------------------------------------------" << std::endl;
    pUser_1001->store_package()->for_each_slot(slot_cout);
    std::cout << pUser_1001->store_package()->empty_slot_next() << ":" << pUser_1001->store_package()->empty_slot_count() << std::endl;

    return 0;
}
