#pragma once

#include "Core.h"
#include <type_traits>
#include <vector>

namespace emu
{
    template <typename T>
    struct CollectionTraits : std::false_type
    {
        typedef typename void CollectionType;
    };

    template <typename T>
    struct CollectionTraits<std::vector<T>> : std::true_type
    {
        typedef typename std::vector<T> CollectionType;
        typedef typename T ElementType;
        typedef typename std::vector<T>::iterator Iterator;

        static void clear(CollectionType& instance)
        {
            instance.clear();
        }

        static void reserve(CollectionType& instance, size_t size)
        {
            instance.reserve(size);
        }

        static size_t size(const CollectionType& instance)
        {
            return instance.size();
        }

        static void push_back(CollectionType& instance, const ElementType& item, size_t index)
        {
            EMU_UNUSED(index);
            instance.push_back(item);
        }

        static Iterator begin(CollectionType& instance)
        {
            return instance.begin();
        }

        static Iterator end(CollectionType& instance)
        {
            return instance.end();
        }
    };

    template <typename T, std::size_t N>
    struct CollectionTraits<T[N]> : std::true_type
    {
        typedef typename T CollectionType[N];
        typedef typename T ElementType;
        typedef typename T* Iterator;

        static void clear(CollectionType& instance)
        {
            EMU_UNUSED(instance);
        }

        static void reserve(CollectionType& instance, size_t size)
        {
            EMU_UNUSED(instance);
            EMU_UNUSED(size);
            EMU_ASSERT(size == N);
        }

        static size_t size(const CollectionType& instance)
        {
            EMU_UNUSED(instance);
            return N;
        }

        static void push_back(CollectionType& instance, const ElementType& item, size_t index)
        {
            instance[index] = item;
        }

        static Iterator begin(CollectionType& instance)
        {
            return &instance[0];
        }

        static Iterator end(CollectionType& instance)
        {
            return &instance[N];
        }
    };
}
