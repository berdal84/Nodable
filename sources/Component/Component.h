#pragma once

#include "Nodable.h"
#include "Object.h"

#include <mirror.h>

namespace Nodable{

    class Node;
    
	class Component: public Object
	{
	public:

		Component() {}

		~Component(){};
		virtual bool update() { return true; };
		virtual void setOwner(Node* _entity){ owner = _entity; }
		Node* getOwner()const{return owner;}

	private:
		Node* owner = nullptr;
		MIRROR_CLASS(Component)(
			MIRROR_PARENT(Object);
		);
	};
}