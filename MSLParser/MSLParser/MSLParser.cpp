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
#include <boost/spirit/include/qi_char_class.hpp>
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

		virtual const std::string& name() { static std::string n = ""; return n; };

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

	template<typename T, Value::Type type>
	class NamedTemplatedValue : public Value
	{
	public:
		using base_type = NamedTemplatedValue<T, type>;

		NamedTemplatedValue() : { _type = type; }
		NamedTemplatedValue(const std::string& name, const T& t) : _value(t), _name(name) { _type = type; }
		NamedTemplatedValue(const std::string& name, T&& t) : _value(std::move(t)), _name(name) { _type = type; }

		const std::string& name() override { return _name; };
	protected:
		T _value;
		std::string _name;
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


	class NamedArrayValue : public NamedTemplatedValue<Value::ArrayType, Value::Type::Array>
	{
	public:
		using base_type::NamedTemplatedValue;
		const ArrayType& asArray() override { return _value; }
	};

	class NamedMapValue : public NamedTemplatedValue<Value::MapType, Value::Type::Map>
	{
	public:
		using base_type::NamedTemplatedValue;
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
				at_c<0>(c.attributes) = std::make_shared<Type>(std::move(f));
			};
		}

		template<typename Type>
		auto createAttrSynthesizerForNamed()
		{
			return [](auto&& f, auto &c)
			{
				using namespace boost::fusion;
				at_c<0>(c.attributes) = std::make_shared<Type>(at_c<0>(f), at_c<1>(f));
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
			using ascii::alpha;
			using ascii::string;
			using namespace qi::labels;
			using namespace boost::fusion;

			//float
			{
				auto percent = [](auto&& f, auto &c)
				{
					using namespace boost::fusion;
					at_c<0>(c.attributes) = std::make_shared<FloatValue>(f / 100.0f);
				};

				rule_float = (float_ >> '%')[percent] | float_[createAttrSynthesizer<FloatValue>()];
			}

			//string
			{
				quoted_string %= lexeme['"' >> *(char_ - '"') >> '"'];
				simple_string %= alpha >> *(char_("a-zA-Z0-9\\.\\-"));

				rule_string = (quoted_string | simple_string)[createAttrSynthesizer<StringValue>()];
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

			//named array
			{
				rule_named_array = (simple_string >> qi::lit("()") >> rule_varray)[createAttrSynthesizerForNamed<NamedArrayValue>()];
			}

			//named map
			{
				rule_named_map = (simple_string >> qi::lit("()") >> rule_vmap)[createAttrSynthesizerForNamed<NamedMapValue>()];
			}

			rule_value = rule_float | rule_array | rule_map | rule_named_array | rule_named_map | rule_string;
			msl = rule_value;
		}

		standard_rule msl;

		standard_rule rule_value;
		standard_rule rule_float;
		standard_rule rule_string;
		standard_rule rule_array;
		standard_rule rule_map;

		standard_rule rule_named_array;
		standard_rule rule_named_map;

		qi::rule<Iterator, msl::Value::MapType(), ascii::space_type> rule_vmap;
		qi::rule<Iterator, msl::Value::ArrayType(), ascii::space_type> rule_varray;
		qi::rule<Iterator, std::string()> quoted_string;

		qi::rule<Iterator, std::string()> simple_string;
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

#ifdef TST_1
	{
		std::string str = "34.1";
		auto p = client::parse_msl(str.begin(), str.end());

		std::string strArr = R"foo(

		[
			Test.1
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
#endif
	
	{
		std::string strArr = R"foo(

		[
			50%
			tr()[1 2 3]
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

