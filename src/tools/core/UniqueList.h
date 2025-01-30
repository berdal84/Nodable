#pragma once
#include <typeindex>
#include <unordered_set>
#include <vector>
#include <list>
#include "types.h"

namespace tools
{
    //
    // Ordered list of unique elements
    // Uses a hashset (rely on std::hash<ElementT> ) and a list to guarantee uniqueness and order.
    //
    template<typename ElementT>
    struct UniqueList
    {
        static_assert( std::is_default_constructible_v<std::hash<ElementT>>, "std::hash<ElementT> must be implemented");

        using Element       = ElementT;
        using Set           = std::unordered_set<u64_t>; // std::unordered, because we need uniqueness, furthermore unordered_set::contains is faster than std::find on a list
        using List          = std::list<Element>; // std::list, because it is ordered, and it supports constant time insertion and removal of elements from anywhere in the container." (see https://devdocs.io/cpp/container/list)
        using Iterator      = typename List::iterator;
        using ConstIterator = typename List::const_iterator;

        Iterator      begin()        { return _ordered_elem.begin(); }
        Iterator      end()          { return _ordered_elem.end(); }
        ConstIterator cbegin() const { return _ordered_elem.cbegin(); }
        ConstIterator cend() const   { return _ordered_elem.cend(); }

        Element& front()
        { return _ordered_elem.front(); }

        Element& back()
        { return _ordered_elem.back(); }

        bool empty() const
        {
            return _unique_elem.empty();
        }

        bool contains(const Element& elem) const // Constant on average, worst case linear in the size of the container.
        {
            return _unique_elem.contains( _hash(elem) );
        }

        const std::list<Element>& data() const
        {
            return _ordered_elem;
        }

        void clear() // O(n)
        {
            if ( _unique_elem.empty() )
            {
                return;
            }
            _unique_elem.clear();
            _ordered_elem.clear();
        }

        template<class Iterator> size_t append( Iterator begin, Iterator end )
        {
            size_t count = 0;

            for(auto it = begin; it != end; ++it)
                if ( append( *it ) )
                    ++count;

            return count;
        }

        size_t size() const
        {
            return _unique_elem.size();
        }

        bool append(const Element& elem)  // Constant on average, worst case linear in the size of the container.
        {
            const auto& [_, inserted] = _unique_elem.insert( _hash(elem) );
            if ( inserted )
            {
                _ordered_elem.push_back( elem );
                return true;
            }
            return false;
        }

        bool remove(const Element& elem)// Constant on average, worst case linear in the size of the container.
        {
            if ( _unique_elem.erase( _hash(elem) ) )
            {
                auto it = std::find( _ordered_elem.begin(), _ordered_elem.end(), elem );
                _ordered_elem.erase( it );
                return true;
            }
            return false;
        }

    private:
        u64_t _hash(const Element& elem) const
        { return std::hash<Element>{}(elem); }

        Set  _unique_elem{};
        List _ordered_elem{};
    };
}
