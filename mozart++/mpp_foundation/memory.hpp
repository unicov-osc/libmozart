/**
 * Mozart++ Template Library: Memory
 * Licensed under Apache 2.0
 * Copyright (C) 2020-2021 Chengdu Covariant Technologies Co., LTD.
 * Website: https://covariant.cn/
 * Github:  https://github.com/chengdu-zhirui/
 */

#pragma once

#include <mozart++/core>
#include <memory>

namespace mpp {
    using std::allocator;

    /**
     * Mozart Allocator Generic Type
     * @tparam T: Target Allocation Type
     * @tparam blck_size: Allocation Chunk Size
     * @tparam allocator_t: Standard Allocator Implementation
     */
    template <typename T, size_t blck_size, template <typename> class allocator_t = allocator>
    class plain_allocator_type final {
        allocator_t<T> mAlloc;

    public:
        /**
         * Must provide default constructor
         */
        plain_allocator_type() = default;

        /**
         * Singleton object, no-copyable and no-movable
         */
        plain_allocator_type(const plain_allocator_type &) = delete;

        plain_allocator_type(plain_allocator_type &&) noexcept = delete;

        /**
         * Must provide default destructor
         */
        ~plain_allocator_type() = default;

        /**
         * [inlined] Allocation function
         * @tparam ArgsT: Type of arguments
         * @param args: Forwarding to constructor of T
         * @return Pointer to allocated memory space
         */
        template <typename... ArgsT>
        inline T *alloc(ArgsT &&... args) {
            T *ptr = mAlloc.allocate(1);
            mAlloc.construct(ptr, forward<ArgsT>(args)...);
            return ptr;
        }

        /**
         * [inlined] Recycle function
         * @param ptr: Pointer to allocated memory space
         */
        inline void free(T *ptr) {
            mAlloc.destroy(ptr);
            mAlloc.deallocate(ptr, 1);
        }
    };

    /**
     * Mozart Balancing Cached Allocator
     * Following Mozart Allocator Generic Type
     * @tparam T: Target Allocation Type
     * @tparam blck_size: Balancing Cache Size
     * @tparam allocator_t: Standard Allocator Implementation
     */
    template <typename T, size_t blck_size, template <typename> class allocator_t = allocator>
    class allocator_type final {
        T *mPool[blck_size];
        allocator_t<T> mAlloc;
        size_t mOffset = 0;

    public:
        allocator_type() {
            while (mOffset < 0.5 * blck_size)
                mPool[mOffset++] = mAlloc.allocate(1);
        }

        allocator_type(const allocator_type &) = delete;

        allocator_type(allocator_type &&) noexcept = delete;

        ~allocator_type() {
            while (mOffset > 0)
                mAlloc.deallocate(mPool[--mOffset], 1);
        }

        template <typename... ArgsT>
        inline T *alloc(ArgsT &&... args) {
            T *ptr = nullptr;
            if (mOffset > 0)
                ptr = mPool[--mOffset];
            else
                ptr = mAlloc.allocate(1);
            mAlloc.construct(ptr, std::forward<ArgsT>(args)...);
            return ptr;
        }

        inline void free(T *ptr) {
            mAlloc.destroy(ptr);
            if (mOffset < blck_size)
                mPool[mOffset++] = ptr;
            else
                mAlloc.deallocate(ptr, 1);
        }
    };
}
