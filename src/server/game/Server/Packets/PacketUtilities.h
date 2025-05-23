/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PacketUtilities_h__
#define PacketUtilities_h__

#include "ByteBuffer.h"
#include <short_alloc/short_alloc.h>

namespace WorldPackets
{
    class PacketArrayMaxCapacityException : public ByteBufferException
    {
    public:
        PacketArrayMaxCapacityException(std::size_t requestedSize, std::size_t sizeLimit);
    };

    /**
     * Utility class for automated prevention of loop counter spoofing in client packets
     */
    template<typename T, std::size_t N>
    class Array
    {
    public:
        using allocator_type = short_alloc::short_alloc<T, (N * sizeof(T) + (alignof(std::max_align_t) - 1)) & ~(alignof(std::max_align_t) - 1)>;
        using arena_type = typename allocator_type::arena_type;

        using storage_type = std::vector<T, allocator_type>;

        using max_capacity = std::integral_constant<std::size_t, N>;

        using value_type = typename storage_type::value_type;
        using size_type = typename storage_type::size_type;
        using pointer = typename storage_type::pointer;
        using const_pointer = typename storage_type::const_pointer;
        using reference = typename storage_type::reference;
        using const_reference = typename storage_type::const_reference;
        using iterator = typename storage_type::iterator;
        using const_iterator = typename storage_type::const_iterator;

        Array() : _storage(_data) {}

        Array(Array const& other) : Array()
        {
            for (T const& element : other)
                _storage.push_back(element);
        }

        Array(Array&& other) noexcept = delete;

        Array& operator=(Array const& other)
        {
            if (this == &other)
                return *this;

            _storage.clear();
            for (T const& element : other)
                _storage.push_back(element);

            return *this;
        }

        Array& operator=(Array&& other) noexcept = delete;

        iterator begin() { return _storage.begin(); }
        const_iterator begin() const { return _storage.begin(); }

        iterator end() { return _storage.end(); }
        const_iterator end() const { return _storage.end(); }

        pointer data() { return _storage.data(); }
        const_pointer data() const { return _storage.data(); }

        size_type size() const { return _storage.size(); }
        bool empty() const { return _storage.empty(); }

        reference operator[](size_type i) { return _storage[i]; }
        const_reference operator[](size_type i) const { return _storage[i]; }

        void resize(size_type newSize)
        {
            if (newSize > max_capacity::value)
                throw PacketArrayMaxCapacityException(newSize, max_capacity::value);

            _storage.resize(newSize);
        }

        void push_back(value_type const& value)
        {
            if (_storage.size() >= max_capacity::value)
                throw PacketArrayMaxCapacityException(_storage.size() + 1, max_capacity::value);

            _storage.push_back(value);
        }

        void push_back(value_type&& value)
        {
            if (_storage.size() >= max_capacity::value)
                throw PacketArrayMaxCapacityException(_storage.size() + 1, max_capacity::value);

            _storage.push_back(std::forward<value_type>(value));
        }

        template<typename... Args>
        T& emplace_back(Args&&... args)
        {
            _storage.emplace_back(std::forward<Args>(args)...);
            return _storage.back();
        }

        iterator erase(const_iterator first, const_iterator last)
        {
            return _storage.erase(first, last);
        }

        void clear()
        {
            _storage.clear();
        }

    private:
        arena_type _data;
        storage_type _storage;
    };

    void CheckCompactArrayMaskOverflow(std::size_t index, std::size_t limit);

    template <typename T>
    class CompactArray
    {
    public:
        CompactArray() : _mask(0) { }

        CompactArray(CompactArray const& right)
            : _mask(right._mask), _contents(right._contents) { }

        CompactArray(CompactArray&& right)
            : _mask(right._mask), _contents(std::move(right._contents))
        {
            right._mask = 0;
        }

        CompactArray& operator=(CompactArray const& right)
        {
            _mask = right._mask;
            _contents = right._contents;
            return *this;
        }

        CompactArray& operator=(CompactArray&& right)
        {
            _mask = right._mask;
            right._mask = 0;
            _contents = std::move(right._contents);
            return *this;
        }

        uint32 GetMask() const { return _mask; }
        T const& operator[](std::size_t index) const { return _contents.at(index); }
        std::size_t GetSize() const { return _contents.size(); }

        void Insert(std::size_t index, T const& value)
        {
            CheckCompactArrayMaskOverflow(index, sizeof(_mask) * 8);

            _mask |= 1 << index;
            if (_contents.size() <= index)
                _contents.resize(index + 1);
            _contents[index] = value;
        }

        void Clear()
        {
            _mask = 0;
            _contents.clear();
        }

        bool operator==(CompactArray const& r) const
        {
            if (_mask != r._mask)
                return false;

            return _contents == r._contents;
        }

        bool operator!=(CompactArray const& r) const { return !(*this == r); }

    private:
        uint32 _mask;
        std::vector<T> _contents;
    };

    template <typename T>
    ByteBuffer& operator<<(ByteBuffer& data, CompactArray<T> const& v)
    {
        uint32 mask = v.GetMask();
        data << uint32(mask);
        for (std::size_t i = 0; i < v.GetSize(); ++i)
            if (mask & (1 << i))
                data << v[i];

        return data;
    }

    template <typename T>
    ByteBuffer& operator>>(ByteBuffer& data, CompactArray<T>& v)
    {
        uint32 mask;
        data >> mask;

        for (std::size_t index = 0; mask != 0; mask >>= 1, ++index)
            if ((mask & 1) != 0)
                v.Insert(index, data.read<T>());

        return data;
    }
}

#endif // PacketUtilities_h__
