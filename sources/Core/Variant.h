#pragma once

#include "Nodable.h"    // for constants and forward declarations
#include "Type.h"
#include <string>
#include <variant>
#include <array>

namespace Nodable{

	/**
		This class is a variant implementation.

	    It wraps std::variant with a Nodable typing
	*/

	class Variant {
	public:
		Variant();
		~Variant();

		bool        isSet()const;
		bool        isType(Type _type)const;
		void        set(const Variant*);
		void        set(const std::string&);
		void        set(const char*);
		void        set(double);
		void        set(bool);
		void        setType(Type _type);
		Type        getType()const;
		std::string getTypeAsString()const;

        explicit operator int()const;
		explicit operator double()const;
		explicit operator bool()const;
		explicit operator std::string()const;

	private:

	    /** Nodable::Type to native type */
	    constexpr static const std::array<Type, 4> s_nodableTypeByIndex = {{
	        Type::Any,
	        Type::Boolean,
	        Type::Double,
	        Type::String
	    }};

        typedef void* Any;
		std::variant<Any, bool, double, std::string> data;
	};
}