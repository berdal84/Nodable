#pragma once

#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <unordered_map>

#include "Signals.h"
#include "types.h"
#include "assertions.h"
#include "reflection/reflection"

namespace tools
{
    template<typename T>
    struct ComponentBag;

    //
    // Base class to implement a new Component for class/struct EntityT
    //
    template<typename EntityT>
    requires std::is_object_v<EntityT>
    class Component
    {
        friend class ComponentBag<EntityT>;
//====== Data ==========================================================================================================
    protected:
        tools::SimpleSignal signal_init; // called after component knows its entity
        tools::SimpleSignal signal_shutdown; // called before to be deleted, when component still knows its entity
    private:
        EntityT*              _m_entity{};
        const TypeDescriptor* _m_type_desc{};
        std::string           _m_name{};
//====== Methods =======================================================================================================
    public:
        Component() = delete;
        Component(const char* name)
        : _m_name(name)
        , _m_type_desc(type::get<Component>())
        {}
        virtual ~Component() = default;
        // TODO: if ComponentBag could delete from the real type (not this base), we could remove virtual destructor no?

        EntityT* entity() const
        {
            return _m_entity;
        }

        const std::string&  name() const
        {
            return _m_name;
        }

        void set_name(const std::string& name)
        {
            // TODO: Can't we have a constexpr name?
            _m_name = name;
        }

        const TypeDescriptor* get_class() const
        {
            return _m_type_desc;
        }

//====== Internal================================== ====================================================================
    private:
        void _init(EntityT* entity, const TypeDescriptor* type_desc )
        {
            LOG_VERBOSE("Component", "_init \"%s\" (type: %s ) ...\n", _m_name.c_str(), type_desc->name() );
            _m_entity    = entity;
            _m_type_desc = type_desc;
            signal_init.emit();
        }

        void _shutdown() // do the mirror of _init()
        {
            LOG_VERBOSE("Component", "_shutdown \"%s\" ...\n", _m_name.c_str());
            signal_shutdown.emit();
            _m_entity    = nullptr;
            _m_type_desc = nullptr;
        }
    };

    template<typename T, typename EntityT>
    concept ComponentFor = requires (T t) {
        std::is_base_of_v<Component<EntityT>, T>;
    };

    //
    // Handle a set of components for an entity class EntityT
    //
    // minimalist example with components having a default constructor:
    //    struct MyEntity
    //    {
    //         template<typename T>   create_component() { return _m_components.create<T>(); }
    //         template<typename T>   get_component()    { return _m_components.get<T>(); }
    //    private:
    //         ComponentBag<MyEntity> _m_components;
    //    }
    //
    template<typename EntityT>
    struct ComponentBag
	{
        using ComponentT     = Component<EntityT>;
        using iterator       = typename std::vector<ComponentT*>::iterator;
        using const_iterator = typename std::vector<ComponentT*>::const_iterator;
        using ComponentByTypeIndex = std::unordered_multimap<std::type_index, ComponentT*>;
//====== Data ==========================================================================================================
    private:
        ComponentByTypeIndex     _m_component_indexed_by_typeid;
        std::vector<ComponentT*> _m_component;
        EntityT*                 _m_entity;
//====== Methods =======================================================================================================
    public:
        ComponentBag() = delete;
        explicit ComponentBag(EntityT* entity)
        : _m_entity(entity)
        {
            ASSERT(entity);
        };
        ComponentBag(const ComponentBag&) = delete;
        ComponentBag(ComponentBag&&) = delete;

        ~ComponentBag()
        {
            assert(_m_component.empty()); // did you called shutdown() before to delete?
            assert(_m_component_indexed_by_typeid.empty()); // should be empty if _m_component is.
        }

        void shutdown() noexcept // free memory
        {
            // TODO: we could optimize these two loops by iterating once.
            //       but for some reasons components have unordered dependencies that needs to be fixed.
            for(ComponentT* component : _m_component)
                component->_shutdown();
            for(ComponentT* component : _m_component)
                _deallocate(component);
            _m_component.clear();
            _m_component_indexed_by_typeid.clear();
        }

        size_t size() const
        {
            return _m_component.size();
        }

        template<ComponentFor<EntityT> T>
        bool has() const
        {
            return get<T>() != nullptr;
        }

        const std::vector<ComponentT*>& components()
        {
            return _m_component;
        }

        template<ComponentFor<EntityT> T>
        T* create()
        {
            auto* c = _allocate<T>();
            _append( c );
            return c;
        }

        template<ComponentFor<EntityT> T, typename ...Args>
        T* create(Args...args)
        {
            auto* c = _allocate<T>(args...);
            _append( c );
            return c;
        }

        template<ComponentFor<EntityT> T>
        void destroy(T* component)
        {
            auto it = std::find_if(_m_component_indexed_by_typeid.begin(), _m_component_indexed_by_typeid.end(), [&](const auto& pair) { return pair.second == component; });
            ASSERT(it != _m_component_indexed_by_typeid.end());
            _m_component_indexed_by_typeid.erase(it);
            _m_component.erase(std::find(_m_component.begin(), _m_component.end(), component ) );
            component->_shutdown();
            _deallocate(component);
        }

        template<ComponentFor<EntityT> T>
        T* get() const
        {
            const T* c = _get_by_type<T>();
            if ( c != nullptr )
                return const_cast<T*>( c );
            return nullptr;
        }

        template<ComponentFor<EntityT> T>
        static std::vector<T*> get_every(const std::vector<ComponentBag*>& entities)
        {
            std::vector<T*> result;
            result.reserve( entities.size() );

            for(ComponentBag* _entity : entities)
            {
                result.push_back(_entity->get<T>() );
            }

            return result;
        }

        template<ComponentFor<EntityT> T>
        T* require(const char* reason) const
        {
            T* component = get<T>();
            VERIFY(component != nullptr, reason);
            return component;
        }

        iterator       begin()        { return _m_component.begin(); }
        iterator       end()          { return _m_component.end(); }
        const_iterator cbegin() const { return _m_component.cbegin(); }
        const_iterator cend() const   { return _m_component.cend(); }
    private:

        template<ComponentFor<EntityT> T>
        const T* _get_by_type() const
        {
            auto it = _m_component_indexed_by_typeid.find(std::type_index(typeid(T)));
            if (it != _m_component_indexed_by_typeid.end() )
            {
                return reinterpret_cast<const T*>(it->second);
            }
            return nullptr;
        }

        template<ComponentFor<EntityT> T>
        const_iterator _find(T* ptr) const
        {
            return std::find(_m_component.begin(), _m_component.end(), ptr);
        }

        template<ComponentFor<EntityT> T>
        void _append(T* c)
        {
            _m_component.push_back(c );
            const auto* type = type::get<T>();
            auto it = _m_component_indexed_by_typeid.emplace( type->id() , c );
            ASSERT(it != _m_component_indexed_by_typeid.end() );
            c->_init( _m_entity, type );
        }

        // for later conversion to an allocator

        template<typename T, typename ...Args> T*   _allocate(Args...args)   { return new T(args...); }
        template<class T>                      void _deallocate(T* ptr){ delete ptr; }
    };
}
