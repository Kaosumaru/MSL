#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace msl
{
	class Value
	{
	public:
		using pointer = std::shared_ptr<Value>;
		using ArrayType = std::vector<pointer>;
		using MapType = std::map<pointer, pointer>;

		enum class Type { String, Float, Boolean, Null, Array, Map };

		virtual const std::string& name() { static std::string n = ""; return n; };
		virtual const MapType& attributes() { throw std::exception(); }

		virtual bool asBool() { return false; }
		virtual float asFloat() { return 0.0f; }
		virtual const std::string& asString() { static std::string n = "Null"; return n; };
		virtual const ArrayType& asArray() { throw std::exception(); }
		virtual const MapType& asMap() { throw std::exception(); }

		static pointer fromString(const std::string &s);
	protected:
		Type _type = Type::Null;
	};
};