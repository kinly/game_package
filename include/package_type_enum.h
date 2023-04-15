#pragma once
#include <cstdint>

enum class package_type_enum : uint32_t {
    normal = 0,   // 普通
    store,        // 仓库
    dress,        // 穿戴
    pet,          // 宠物
};
