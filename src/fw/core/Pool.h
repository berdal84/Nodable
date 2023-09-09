#pragma once
#include "TIdentifier.h"
#include "assertions.h"
#include <algorithm>
#include <cstdlib>
#include <set>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#if NDBL_DEBUG
#include "reflection/type.h"
#endif

#define POOL_REGISTRABLE_WITH_CUSTOM_IMPLEM( Class ) \
protected: \
    template<typename T> using ID = ::fw::ID<T>; \
    template<typename T> using PoolID = ::fw::pool::PoolID<T>; \
    PoolID<Class> m_id;\
public:                                              \
    using is_pool_registrable = std::true_type; \
    PoolID<Class> poolid() const { return m_id; }; \
    void poolid(ID<Class> _id)

#define POOL_REGISTRABLE( Class ) \
POOL_REGISTRABLE_WITH_CUSTOM_IMPLEM( Class ) { m_id = std::move(_id); }

template<typename T>
void static_assert__is_pool_registrable()
{
    static_assert( std::is_same_v<typename T::is_pool_registrable, std::true_type>, "This type is not pool registrable, use POOL_REGISTRABLE macros." );
}

namespace fw
{
namespace pool
{
    /**
     * PoolID is the equivalent of a pointer, but it points to a Pool item.
     * It can be used like regular pointer, except that de-referencing cost a bit more.
     *
     * Example:
     *
     * PoolID<Type> id = ...;  // we assume we have a valid id
     * id->do_something(); // de-reference from a pool id (cost a bit more) + call a method on the pointer.
     *
     * Type* ptr = id.get(); // de-referencing once before doing lots of calls is faster.
     * ptr->do_this();
     * ...
     * ptr->do_that();
     */
    template<typename Type>
    class PoolID: public ID<Type>
    {
    public:
        PoolID() = default;

        template<typename OtherT>
        PoolID(ID<OtherT> id)
        : ID<Type>(id)
        {}

        static PoolID<Type> null;

        Type*       get() const; // Return a pointer to the data from the Pool having an id == this->id

        operator u32_t () const { return this->id(); }
        Type* operator -> ()       { return get(); }
        Type* operator -> () const { return get(); }
        Type& operator *  ()       { return *get(); }
        Type& operator *  () const { return *get(); }
    };

    template<typename T>
    PoolID<T> PoolID<T>::null{};

    /**
     * Vector stores a pointer to a given vector type and the size of its elements
     */
    class AgnosticVector
    {
    public:
#define CHECK_TYPE(Type) FW_EXPECT( std::type_index(typeid(Type)) == m_type, "The type you asked is not the one this vector is made for." )

        template<class T>
        static AgnosticVector* create(size_t reserved_size)
        {
            auto* buffer = new std::vector<T>();
            buffer->reserve( reserved_size );

            auto _swap = [buffer](size_t a, size_t b) -> void
            {
                std::swap( buffer->at(a), buffer->at(b) );
            };

            auto _pop_back = [buffer]() -> void
            {
                buffer->pop_back();
            };

            auto _at = [buffer](size_t index) -> void*
            {
                return &buffer->at( index );
            };

            auto _size = [buffer]() -> size_t
            {
                return buffer->size();
            };

            auto _delete_buffer = [buffer]() -> void
            {
                delete buffer;
            };

            auto _poolid_at = [buffer](size_t index) -> id_t
            {
                return (id_t)buffer->at( index ).poolid();
            };

            return new AgnosticVector(buffer, std::type_index(typeid(T)),
                    _at, _size, _pop_back, _swap, _delete_buffer, _poolid_at);
        }

        AgnosticVector(
                void* buffer,
                const std::type_index type,
                const std::function<void*(size_t index)>     _at,
                const std::function<size_t()>                _size,
                const std::function<void()>                  _pop_back,
                const std::function<void(size_t, size_t)>    _swap,
                const std::function<void()>                  _delete_buffer,
                const std::function<id_t(size_t)>            _poolid_at)
            : m_type( type )
            , m_buffer( buffer )
            , at( _at )
            , size( _size )
            , pop_back( _pop_back )
            , swap( _swap )
            , delete_buffer( _delete_buffer )
            , poolid_at( _poolid_at )
            , m_typename( type.name() )
        {}

        ~AgnosticVector();

        std::function<void*(size_t index)>  at;
        std::function<size_t()>             size;
        std::function<void()>               pop_back;
        std::function<void(size_t, size_t)> swap;
        std::function<id_t(size_t)>         poolid_at; // get pool id at index

        template<class T, typename ...Args>
        T& emplace_back(Args ...args)
        { return get<T>()->template emplace_back<>( args... ); }

        template<class T>
        T& emplace_back()
        { return get<T>()->template emplace_back<>(); }

        template<class T>
        std::vector<T>* get()
        { CHECK_TYPE(T); return (std::vector<T>*)m_buffer; }

    private:
        const std::function<void()>  delete_buffer;
        void*                        m_buffer; // std::vector<T>* with T as std::type_index(typeid(T)) == m_type
        const std::type_index        m_type;
        const char*                  m_typename;
#undef CHECK_TYPE
    };

    /**
     * When a new instance is created, a new record is added.
     * It allow to find the location of the object in memory and its type.
     */
    struct Record
    {
        AgnosticVector*     vector;
        size_t pos;  // Zero-based position of the data in the vector.
        [[nodiscard]] void* data() const;
    };

    /**
     * Handle memory for a given set of types.
     *
     * Types must be initialized using init_for<MyType>() prior to use.
     * Then, a new instance can be created with create<MyType>()
     *
     * Pointers are provided for convenience but should not be stored.
     * Users should use id to store references to a given object.
     * The pool guarantee the id is still valid even if the memory layout changed.
     */
    class Pool
    {
    public:
        static Pool* init(size_t reserved_size = 0);
        static void  shutdown();
        static Pool* get_pool();

    private: /** for now, lets allow a single Pool at a time (see statics) */
        Pool(size_t reserved_size);
        ~Pool();
    public:

        template<typename T>
        void init_for();

        template<typename T>
        T* get(id_t id);

        template<typename T>
        T* get(PoolID<T> id);

        template<typename T>
        std::vector<T*> get(std::vector<PoolID<T>> ids);

        template<typename T>
        std::vector<T>& get_all();

        template<typename T, typename ...Args>
        PoolID<T> create(Args... args);

        template<typename T>
        PoolID<T> create();

        template<typename T>
        void destroy(T* ptr);

        template<typename T>
        void destroy(PoolID<T> pool_id);

        // TODO: define a unique destroy using iterators
        template<typename T>
        void destroy(std::unordered_set<PoolID<T>> ids);

        template<typename T>
        void destroy(std::set<PoolID<T>> ids);

        template<typename T>
        void destroy(std::vector<PoolID<T>> ids);
        // TODO: define a unique destroy using iterators (END)

    private:
        template<typename T>
        ID<T> generate_id()
        {
            m_next_id++;
            return ID<T>{m_next_id};
        }

        template<typename T>
        PoolID<T> make_record(T* data, AgnosticVector* vec, size_t pos );

        template<typename T>
        AgnosticVector* get_agnostic_vector();

        size_t m_reserved_size;
        id_t   m_next_id;
        std::unordered_map<id_t, Record> m_record_by_id;
        std::unordered_map<std::type_index, AgnosticVector*> m_vector_by_type;
    private:
        static Pool* s_current_pool;
    };

    template<typename Type>
    Type* PoolID<Type>::get() const
    { return *this == null ? nullptr : Pool::get_pool()->get<Type>( *this ); }

    template<typename T>
    T* Pool::get(PoolID<T> poolid)
    { return get<T>( poolid.id() ); }

    template<typename T>
    std::vector<T*> Pool::get(std::vector<PoolID<T>> ids)
    {
        static_assert__is_pool_registrable<T>();
        std::vector<T*> result;
        result.reserve(ids.size());
        for(auto each_id : ids )
        {
            T* ptr = get<T>(each_id);
            FW_ASSERT(ptr != nullptr);
            result.push_back( ptr );
        }
        return result;
    }

    template<typename T>
    std::vector<T>& Pool::get_all()
    { return *get_agnostic_vector<T>()->template get<T>(); }

    template<typename T, typename ...Args>
    PoolID<T> Pool::create(Args... args)
    {
        static_assert__is_pool_registrable<T>();
        LOG_VERBOSE("Pool", "Create '%s' (with args) ...\n", fw::type::get<T>()->get_name() );
        auto*  vec   = get_agnostic_vector<T>();
        size_t index = vec->size();
        T*     data  = &vec->template emplace_back<T>(args...);
        PoolID<T> id = make_record(data, vec, index );
        LOG_VERBOSE("Pool", "Create '%s' (with args) OK\n", fw::type::get<T>()->get_name() );
        LOG_FLUSH();
        return id;
    }

    template<typename T>
    PoolID<T> Pool::create()
    {
        static_assert__is_pool_registrable<T>();
        AgnosticVector* vector  = get_agnostic_vector<T>();
        LOG_VERBOSE("Pool", "Create '%s' ...\n", fw::type::get<T>()->get_name() );
        T* data = &vector->emplace_back<T>();
        PoolID<T> id = make_record(data, vector, vector->size()-1 );
        LOG_VERBOSE("Pool", "Create '%s' OK\n", fw::type::get<T>()->get_name() );
        LOG_FLUSH();
        return id;
    }

    template<typename T>
    T* Pool::get(id_t id)
    {
        static_assert__is_pool_registrable<T>();
        auto it = m_record_by_id.find(id);
        if ( it == m_record_by_id.end() )
        {
            return nullptr;
        }
        auto ptr = reinterpret_cast<T*>( it->second.data() );
        LOG_VERBOSE("Pool", "de-referencing id 0x%.8x => addr: &0x%.8x (typename: %.16s*)\n", (size_t)id, (size_t)ptr, fw::type::get<T>()->get_name() )
        LOG_FLUSH()
        return ptr;
    }

    template<typename T>
    void Pool::init_for()
    {
        static_assert__is_pool_registrable<T>();
        auto type_id = std::type_index(typeid(T));
        FW_ASSERT(m_vector_by_type.find(type_id) == m_vector_by_type.end() );
        AgnosticVector* agnostic_vector = AgnosticVector::create<T>(m_reserved_size);
        m_vector_by_type.insert({type_id, agnostic_vector});
        LOG_VERBOSE("Pool", "Init for '%s' OK\n", fw::type::get<T>()->get_name() );
        LOG_FLUSH()
    }

    template<typename T>
    AgnosticVector* Pool::get_agnostic_vector()
    {
        static_assert__is_pool_registrable<T>();
        auto type_id = std::type_index(typeid(T));
        auto it = m_vector_by_type.find(type_id);

        if (it == m_vector_by_type.end())
        {
            LOG_VERBOSE("Pool", "No vector found for '%s'\n", fw::type::get<T>()->get_name() );
            // Not great to do the init here, but required when a type is not handled yet
            init_for<T>();
            return get_agnostic_vector<T>();
        }
        return it->second;
    }

    template<typename T>
    PoolID<T> Pool::make_record(T* data, AgnosticVector* vec, size_t pos )
    {
        static_assert__is_pool_registrable<T>();
        data->poolid( generate_id<T>() );
        m_record_by_id.insert( {data->poolid(), { vec, pos } } );
        LOG_VERBOSE("Pool", "New record with id %zu (type: %s, index: %zu) ...\n", data->poolid(), fw::type::get<T>()->get_name(), pos );
        return data->poolid();
    }

    template<typename T>
    void Pool::destroy(T* ptr)
    { static_assert__is_pool_registrable<T>(); destroy(ptr->poolid()); }

    template<typename T>
    void Pool::destroy(PoolID<T> pool_id)
    {
        static_assert__is_pool_registrable<T>();
        auto it = m_record_by_id.find( (id_t)pool_id );
        FW_EXPECT( it != m_record_by_id.end(), "No record_to_delete found" );

        Record& record_to_delete = it->second;
        size_t  last_pos       = record_to_delete.vector->size() - 1; // size() can't be 0, because at least the item to destroy is in it

        // Preserve contiguous memory by swapping the record_to_delete to delete and the last.
        if(record_to_delete.pos != last_pos)
        {
            // swap with the back, and update back's pool id
            size_t last_poolid = record_to_delete.vector->poolid_at( last_pos );
            record_to_delete.vector->swap( record_to_delete.pos, last_pos );
            m_record_by_id.find( last_poolid )->second.pos = record_to_delete.pos;
        }
        // Delete instance and record_to_delete
        record_to_delete.vector->pop_back();
        m_record_by_id.erase( it );

        LOG_VERBOSE("Pool", "Destroyed record with id %zu (type: %s, index: %zu) ...\n", pool_id, fw::type::get<T>()->get_name(), record_to_delete.pos);
    }

    template<typename T>
    void Pool::destroy(std::unordered_set<PoolID<T>> ids)
    { static_assert__is_pool_registrable<T>(); LOG_WARNING("Pool", "destroy(std::unordered_set<PoolID<T>> id) not implemented yet\n" ) }

    template<typename T>
    void Pool::destroy(std::set<PoolID<T>> ids)
    { static_assert__is_pool_registrable<T>(); LOG_WARNING("Pool", "destroy(std::set<PoolID<T>> id) not implemented yet\n" ) }

    template<typename T>
    void Pool::destroy(std::vector<PoolID<T>> ids)
    {
        static_assert__is_pool_registrable<T>();
        for(auto each_id : ids )
        {
            destroy( each_id );
        }
    }
} // namespace pool
} // namespace fw
