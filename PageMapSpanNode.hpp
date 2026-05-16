#pragma once
#include <cstdint>   // 用于 uintptr_t
#include <cstring>   // 用于 memset
#include <cassert>   // 用于断言


#include "FixedSizeMemoryPool.hpp"

// 定义断言宏（兼容原有 ASSERT 用法）
#define ASSERT(expr) assert(expr)

// 单级数组映射（适用于 BITS 较小场景，直接分配连续数组）
template <int BITS>
class TCMalloc_PageMap1 {
private:
    static const int LENGTH = 1 << BITS;  // 数组长度：2^BITS
    void** array_;                        // 存储映射关系的数组（void* 类型值）

public:
    typedef uintptr_t Number;  // 键值类型（页号等无符号整数）

    // 构造函数：通过指定分配器分配数组内存并初始化
    // 参数：allocator - 内存分配函数（输入大小，返回分配的内存指针）
    explicit TCMalloc_PageMap1(void* (*allocator)(size_t)) {
        // 分配 2^BITS 个 void* 大小的内存（总大小：sizeof(void*) * 2^BITS）
        array_ = reinterpret_cast<void**>((*allocator)(sizeof(void*) * LENGTH));
        memset(array_, 0, sizeof(void*) * LENGTH);  // 初始化为 NULL
    }

    // 获取键 k 对应的 value
    // 返回值：存在返回对应指针，k 超出范围或未设置返回 NULL
    void* get(Number k) const {
        if ((k >> BITS) > 0) {  // k 超过 2^BITS - 1（超出映射范围）
            return NULL;
        }
        return array_[k];
    }

    // 设置键 k 对应的 value
    // 前置条件：k 必须在 [0, 2^BITS - 1] 范围内，且已通过 Ensure 确保节点存在
    void set(Number k, void* v) {
        ASSERT(k < LENGTH);  // 确保 k 不越界（补充原有前置条件的断言）
        array_[k] = v;
    }
};

// 二级基数树映射（适用于中等 BITS 场景，分根节点+叶节点两级，节省内存）
template <int BITS>
class TCMalloc_PageMap2 {
private:
    static const int ROOT_BITS = 5;          // 根节点位数（32 个根节点）
    static const int ROOT_LENGTH = 1 << ROOT_BITS;  // 根节点数组长度（32）
    static const int LEAF_BITS = BITS - ROOT_BITS;  // 叶节点位数
    static const int LEAF_LENGTH = 1 << LEAF_BITS;  // 每个叶节点存储的 value 数量

    // 叶节点结构：存储实际的 value 数组
    struct Leaf {
        void* values[LEAF_LENGTH];
    };

    Leaf* root_[ROOT_LENGTH];  // 根节点：指向 32 个叶节点的指针数组    

    FixedSizeMemoryPool<Leaf> _leafObjPool;

public:
    typedef uintptr_t Number;  // 键值类型

    // 构造函数：初始化分配器和根节点（初始化为 NULL）
    explicit TCMalloc_PageMap2() {
        
        memset(root_, 0, sizeof(root_));  // 根节点指针初始化为 NULL
    }

    // 获取键 k 对应的 value
    // 返回值：存在返回对应指针，k 超出范围或节点未分配返回 NULL
    void* get(Number k) const {
        const Number i1 = k >> LEAF_BITS;  // 根节点索引（高 LEAF_BITS 位）
        const Number i2 = k & (LEAF_LENGTH - 1);  // 叶节点索引（低 LEAF_BITS 位）

        // k 超出范围 或 根节点对应的叶节点未分配
        if ((k >> BITS) > 0 || root_[i1] == NULL) {
            return NULL;
        }
        return root_[i1]->values[i2];
    }

    // 设置键 k 对应的 value
    // 前置条件：k 必须在 [0, 2^BITS - 1] 范围内，且已通过 Ensure 确保叶节点存在
    void set(Number k, void* v) {
        const Number i1 = k >> LEAF_BITS;
        const Number i2 = k & (LEAF_LENGTH - 1);

        ASSERT(i1 < ROOT_LENGTH);          // 确保根节点索引不越界
        ASSERT(root_[i1] != NULL);         // 确保叶节点已分配（补充断言）
        root_[i1]->values[i2] = v;
    }

    // 确保 [start, start + n - 1] 范围内的键对应的叶节点已分配
    // 返回值：分配成功返回 true，失败（内存不足/超出范围）返回 false
    bool Ensure(Number start, size_t n) {
        for (Number key = start; key <= start + n - 1;) {
            const Number i1 = key >> LEAF_BITS;  // 当前键对应的根节点索引

            // 根节点索引超出范围（键超出总映射范围）
            if (i1 >= ROOT_LENGTH) {
                return false;
            }

            // 若叶节点未分配，则创建并初始化
            if (root_[i1] == NULL) {
                Leaf* leaf = _leafObjPool.New();
                if (leaf == NULL) {  // 内存分配失败
                    return false;
                }
                memset(leaf, 0, sizeof(*leaf));  // 叶节点 value 初始化为 NULL
                root_[i1] = leaf;
            }

            // 跳过当前叶节点覆盖的所有键（推进到下一个叶节点的起始键）
            key = ((key >> LEAF_BITS) + 1) << LEAF_BITS;
        }
        return true;
    }

    // 预分配所有可能的映射节点（覆盖 [0, 2^BITS - 1] 所有键）
    void PreallocateMoreMemory() {
        Ensure(0, 1 << BITS);
    }
};

// 三级基数树映射（适用于大 BITS 场景，分根节点+中间节点+叶节点三级，进一步节省内存）
template <int BITS>
class TCMalloc_PageMap3 {
private:
    static const int INTERIOR_BITS = (BITS + 2) / 3;  // 中间节点位数（向上取整，分3级）
    static const int INTERIOR_LENGTH = 1 << INTERIOR_BITS;  // 中间节点数组长度
    static const int LEAF_BITS = BITS - 2 * INTERIOR_BITS;  // 叶节点位数（剩余 bits）
    static const int LEAF_LENGTH = 1 << LEAF_BITS;  // 每个叶节点存储的 value 数量

    // 中间节点结构：指向子节点（中间节点或叶节点）的指针数组
    struct Node {
        Node* ptrs[INTERIOR_LENGTH];
    };

    // 叶节点结构：存储实际的 value 数组
    struct Leaf {
        void* values[LEAF_LENGTH];
    };

    Node* root_;  // 根节点（三级树的入口）
    //void* (*allocator_)(size_t);  // 内存分配函数指针

    FixedSizeMemoryPool<Node> _NodeObjPool;

    FixedSizeMemoryPool<Leaf> _LeafObjPool;

    // 创建并初始化一个中间节点（所有指针初始化为 NULL）
    Node* NewNode() {
        Node* result = _NodeObjPool.New();
        if (result != NULL) {
            memset(result, 0, sizeof(*result));
        }
        return result;
    }

public:
    typedef uintptr_t Number;  // 键值类型

    // 构造函数：初始化分配器和根节点
    explicit TCMalloc_PageMap3() {
        root_ = NewNode();  // 根节点初始化为新创建的中间节点
    }

    // 获取键 k 对应的 value
    // 返回值：存在返回对应指针，k 超出范围或节点未分配返回 NULL
    void* get(Number k) const {
        // 拆分键为三级索引：i1（根->中间）、i2（中间->叶）、i3（叶->value）
        const Number i1 = k >> (LEAF_BITS + INTERIOR_BITS);
        const Number i2 = (k >> LEAF_BITS) & (INTERIOR_LENGTH - 1);
        const Number i3 = k & (LEAF_LENGTH - 1);

        // 检查条件：k 超出范围 或 中间节点未分配 或 叶节点未分配
        if ((k >> BITS) > 0 ||
            root_->ptrs[i1] == NULL ||
            root_->ptrs[i1]->ptrs[i2] == NULL) {
            return NULL;
        }

        // 从叶节点中获取 value（将中间节点的指针强制转换为叶节点）
        return reinterpret_cast<Leaf*>(root_->ptrs[i1]->ptrs[i2])->values[i3];
    }

    // 设置键 k 对应的 value
    // 前置条件：k 必须在 [0, 2^BITS - 1] 范围内，且已通过 Ensure 确保节点存在
    void set(Number k, void* v) {
        ASSERT((k >> BITS) == 0);  // 确保 k 不超出总范围

        // 拆分三级索引
        const Number i1 = k >> (LEAF_BITS + INTERIOR_BITS);
        const Number i2 = (k >> LEAF_BITS) & (INTERIOR_LENGTH - 1);
        const Number i3 = k & (LEAF_LENGTH - 1);

        // 断言节点已分配（符合前置条件）
        ASSERT(root_->ptrs[i1] != NULL);
        ASSERT(root_->ptrs[i1]->ptrs[i2] != NULL);

        // 设置叶节点中的 value
        reinterpret_cast<Leaf*>(root_->ptrs[i1]->ptrs[i2])->values[i3] = v;
    }

    // 确保 [start, start + n - 1] 范围内的键对应的节点（中间+叶）已分配
    // 返回值：分配成功返回 true，失败（内存不足/超出范围）返回 false
    bool Ensure(Number start, size_t n) {
        for (Number key = start; key <= start + n - 1;) {
            // 拆分当前键的三级索引（仅用到前两级：i1->中间节点，i2->叶节点）
            const Number i1 = key >> (LEAF_BITS + INTERIOR_BITS);
            const Number i2 = (key >> LEAF_BITS) & (INTERIOR_LENGTH - 1);  // 修正：原代码漏写 key，应为 key

            // 检查索引是否超出范围（超出总映射范围）
            if (i1 >= INTERIOR_LENGTH || i2 >= INTERIOR_LENGTH) {
                return false;
            }

            // 确保中间节点已分配
            if (root_->ptrs[i1] == NULL) {
                Node* mid_node = NewNode();
                if (mid_node == NULL) {  // 内存分配失败
                    return false;
                }
                root_->ptrs[i1] = mid_node;
            }

            // 确保叶节点已分配
            if (root_->ptrs[i1]->ptrs[i2] == NULL) {
                Leaf* leaf = _LeafObjPool.New();
                if (leaf == NULL) {  // 内存分配失败
                    return false;
                }
                memset(leaf, 0, sizeof(*leaf));  // 修正：原代码用 sizeof(leaf)（指针大小），应为 sizeof(*leaf)
                // 将叶节点指针存入中间节点（强制转换为 Node* 类型存储）
                root_->ptrs[i1]->ptrs[i2] = reinterpret_cast<Node*>(leaf);
            }

            // 跳过当前叶节点覆盖的所有键（推进到下一个叶节点的起始键）
            key = ((key >> LEAF_BITS) + 1) << LEAF_BITS;
        }
        return true;
    }

};
