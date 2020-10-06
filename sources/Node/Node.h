#pragma once
#include <string>
#include <memory>               // for unique_ptr

#include "Nodable.h"            // for constants and forward declarations
#include "Object.h"
#include "Component.h"
#include "NodeTraversal.h"

namespace Nodable{
	
	/*
		The role of this class is to provide connectable Objects as Nodes.

		A node is an Object (composed by Members) but here they can be linked together in 
		order to create graphs.

		Every Node has a parentContainer (cf. Container class).
		All nodes are built from a Container, which first create an instance of this class (or derived) and then
		add some components (cf. Component and derived classes) on it.
	*/
	class Node : public Object
	{
	public:
		Node(std::string /* label */ = "UnnamedNode");
		~Node();

		/* Get parent container of this node.
		   note: Could be nullptr only if this node is a root. */
		Container* getParentContainer()const;

		/* Set the parent container of this node */
		void setParentContainer(Container*);

		inline Container* getInnerContainer()const { return this->innerContainer;  }
		inline void setInnerContainer(Container* _container) { this->innerContainer = _container; };

		/* Get the label of this Node */
		const char* getLabel()const;		
		

		/* Update the label of the node.
		   note: a label is not unique. */
		virtual void        updateLabel       (){};

		/* Set a label for this Node */
		void                setLabel          (const char*);

		/* Set a label for this Node */
		void                setLabel          (std::string);

		/* Adds a new wire related* to this node. (* connected to one of the Node Member)
		   note: a node can connect two Members (cf. Wire class) */
		void                addWire           (Wire*);

		/* Removes a wire from this Node */
		void                removeWire        (Wire*);

		/* Get wires related* to this node. (* connected to one of the Node Member) */
		Wires&              getWires          ();

		/* Get the input connectedd wire count. */
		int                 getInputWireCount ()const;

		/* Get the output connected wire count. */
		int                 getOutputWireCount()const;

		/* Force this node to be evaluated at the next update() call */
		void                setDirty          (bool _value = true);

		/* return true if this node needs to be updated and false otherwise */
		bool                isDirty           ()const;
		
		/* Update the state of this (and only this) node */
		virtual bool update();

		/* Create an oriented edge (Wire) between two Members */
		static Wire* Connect(Member* /*_from*/, Member* /*_to*/);
	
		/* Disconnect a wire. This method is the opposite of Node::Connect.*/
		static void Disconnect(Wire* _wire);

		/* Add a component to this Node
		   note: User must check be sure this Node has no other Component of the same type (cf. hasComponent(...))*/
		template<typename T>
		void addComponent(T* _component)
		{
			static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
			std::string name(T::GetClass()->getName());
			components.emplace(std::make_pair(name, _component));
			_component->setOwner(this);
		}

		/* Return true if this node has the component specified by it's type T.
		   note: T must be a derived class of Component
		 */
		template<typename T>
		bool hasComponent()const
		{
			static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
			return getComponent<T>() != nullptr;
		}

		/* Get all components of this Node */
		inline const Components& getComponents()const
		{
			return components;
		}

		/* Delete a component of this node by specifying it's type T.
		   note: T must be Component derived. */
		template<typename T>
		void deleteComponent() {
			static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
			auto name = T::GetClass()->getName();
			auto component = getComponent<T>();
			components.erase(name);
			delete component;
		}

		/* Get a component of this Node by specifying it's type T.
		   note: T must be Component derived.*/
		template<typename T>
		T* getComponent()const {
			static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
			auto c = T::GetClass();
			auto name = c->getName();

			// Search with class name
			{
				auto it = components.find(name);
				if (it != components.end()) {
					return reinterpret_cast<T*>(it->second);
				}
			}

			// Search for a derived class
			for (auto it = components.begin(); it != components.end(); it++) {
				Component* component = it->second;
				if (component->getClass()->isChildOf(c, false)) {
					return reinterpret_cast<T*>(component);
				}
			}

			return nullptr;
		};

	protected:
		Components components;

	private:
		/* Will be called automatically on member value changes */
		void onMemberValueChanged(const char* _name)override;
		
		Container* innerContainer;
		Container*                parentContainer;
		std::string               label;
		bool                      dirty;   // when is true -> needs to be evaluated.
		Wires                     wires;             // contains all wires connected to or from this node.
	
	public:
		MIRROR_CLASS(Node)(
			MIRROR_PARENT(Object)
		);	
	};
}
