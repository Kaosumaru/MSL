// MSLParser.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#define BOOST_RESULT_OF_USE_DECLTYPE
#define BOOST_SPIRIT_USE_PHOENIX_V3


#include <string>
#include <vector>
#include <map>
#include <memory>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/make_shared.hpp>
#include <boost/fusion/include/std_pair.hpp>

/*
MSL Parser

Wbudowane typy:
- string (cudzys?owia, bez cudzys?owiu)
- float (1. 1 100%)
- boolean (true, false)
- null

- tablica [10 20 30]
- mapa {1:1 2:2 3:3}

- named value   name() value

-komentarze


[name() aaa 
*/

namespace msl
{
	class Value
	{
	public:
		using pointer = std::shared_ptr<Value>;
		using ArrayType = std::vector<pointer>;
		using MapType = std::map<pointer, pointer>;

		enum class Type { String, Float, Boolean, Null, Array, Map };

		virtual float asFloat() { return 0.0f; }
		virtual const std::string& asString() { static std::string n = "Null"; return n; };
		virtual const ArrayType& asArray() { throw std::exception(); }
		virtual const MapType& asMap() { throw std::exception(); }

	protected:
		Type _type = Type::Null;
	};

	template<typename T, Value::Type type>
	class TemplatedValue : public Value
	{
	public:
		using base_type = TemplatedValue<T, type>;

		TemplatedValue() : { _type = type; }
		TemplatedValue(const T& t) : _value(t) { _type = type; }
		TemplatedValue(T&& t) : _value(std::move(t)) { _type = type; }

	protected:
		T _value;
	};

	class StringValue : public TemplatedValue<std::string, Value::Type::String>
	{
	public:
		using base_type::TemplatedValue;
		const std::string& asString() override { return _value; }
	};

	class FloatValue : public TemplatedValue<float, Value::Type::Float>
	{
	public:
		using base_type::TemplatedValue;
		float asFloat() override { return _value; }
	};

	class ArrayValue : public TemplatedValue<Value::ArrayType, Value::Type::Array>
	{
	public:
		using base_type::TemplatedValue;
		const ArrayType& asArray() override { return _value; }
	};

	class MapValue : public TemplatedValue<Value::MapType, Value::Type::Map>
	{
	public:
		using base_type::TemplatedValue;
		const MapType& asMap() override { return _value; }
	};
};


namespace client
{



	using namespace boost::spirit;

	template <typename Iterator>
	struct msl_grammar : qi::grammar<Iterator, msl::Value::pointer(), ascii::space_type>
	{
		using standard_rule = qi::rule<Iterator, msl::Value::pointer(), ascii::space_type>;


		template<typename Type>
		auto createAttrSynthesizer()
		{
			
			return [](auto&& f, auto &c)
			{
				using namespace boost::fusion;
				at_c<0>(c.attributes) = std::make_shared<Type>(f);
			};
		}

		msl_grammar() : msl_grammar::base_type(msl)
		{
			using namespace msl;
			using qi::lit;
			using qi::rule;
			using qi::float_;
			using qi::_1;
			using qi::phrase_parse;
			using ascii::space;
			using ascii::char_;
			using ascii::string;
			using namespace qi::labels;
			

			//float
			{
				rule_float = float_[createAttrSynthesizer<FloatValue>()];
			}

			//string
			{
				quoted_string %= lexeme['"' >> *(char_ - '"') >> '"'];
				rule_string = quoted_string[createAttrSynthesizer<StringValue>()];
			}

			//array
			{
				rule_varray %= '[' >> *rule_value >> ']';
				rule_array = rule_varray[createAttrSynthesizer<ArrayValue>()];
			}

			//map
			{
				rule_vmap %= qi::lit("{") >> *(rule_value >> qi::lit(":") >> rule_value) >> qi::lit("}");
				rule_map = rule_vmap[createAttrSynthesizer<MapValue>()];
			}


			rule_value = rule_float | rule_array | rule_map | rule_string;
			msl = rule_value;
		}

		standard_rule msl;

		standard_rule rule_value;
		standard_rule rule_float;
		standard_rule rule_string;
		standard_rule rule_array;
		standard_rule rule_map;


		qi::rule<Iterator, msl::Value::MapType(), ascii::space_type> rule_vmap;
		qi::rule<Iterator, msl::Value::ArrayType(), ascii::space_type> rule_varray;
		qi::rule<Iterator, std::string()> quoted_string;
	};

	template <typename Iterator>
	msl::Value::pointer parse_msl(Iterator first, Iterator last)
	{
		msl_grammar<Iterator> msl; // Our grammar
		msl::Value::pointer ast; // Our tree

		using boost::spirit::ascii::space;
		bool r = phrase_parse(first, last, msl, space, ast);

		return ast;
	}



}

int main()
{
	R"foo(

	{
		HP: 100
		Damage: 20
	
	}

	)foo";



#if 1
	{
		std::string str = "34.1";
		auto p = client::parse_msl(str.begin(), str.end());


		//std::string strArr = "[34.1 {1:1 \"Mateusz\":\"Borycki\"} \"Test\"]";

		std::string strArr = R"foo(

		[
			34.1
			{
				1:1 
				"Mateusz":"Borycki"
			}
			"Test"
		]

		)foo";

		auto arr = client::parse_msl(strArr.begin(), strArr.end());
	}
	
#else
	{
		std::string str = "34.1";
		auto p = client::parse_msl(str.begin(), str.end());
	}

	{
		std::string str = "aaa sadas";
		auto p = client::parse_complex(str.begin(), str.end());
	}

	{
		std::string str = "\"aaa sa\"das";
		auto p = client::parse_complex(str.begin(), str.end());
	}
#endif

    return 0;
}

