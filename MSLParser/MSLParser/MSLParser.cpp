// MSLParser.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#define BOOST_RESULT_OF_USE_DECLTYPE
#define BOOST_SPIRIT_USE_PHOENIX_V3


#include <string>
#include <vector>
#include <memory>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/make_shared.hpp>
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

		enum class Type { String, Float, Boolean, Null, Array, Map };

		virtual float asFloat() { return 0.0f; }
		virtual const std::string& asString() { static std::string n = "Null"; return n; };
		virtual const ArrayType& asArray() { throw std::exception(); }

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
};


namespace client
{
	template <typename Iterator>
	msl::Value::pointer parse_complex(Iterator first, Iterator last)
	{
		using namespace msl;
		using namespace boost::spirit;
		using qi::rule;
		using qi::float_;
		using qi::_1;
		using qi::phrase_parse;
		using ascii::space;
		using ascii::char_;
		using ascii::string;

		using boost::phoenix::ref;

		//float float_value = 0.0;
		//std::string
		msl::Value::pointer ptr;

		//value!
		rule<Iterator> rule_value;

		rule<Iterator> rule_string;
		rule<Iterator> rule_float;
		rule<Iterator> rule_array;

		//string
		{
			auto sfunc = [&](auto&& f) { ptr = boost::make_shared<StringValue>(f); };

			rule<Iterator, std::string()> rule_string_nq = lexeme[+(char_ - ' ')[_val += _1]];
			rule<Iterator, std::string()> rule_string_q = lexeme['"' >> +(char_ - '"')[_val += _1] >> '"'];
			rule_string = rule_string_q[sfunc] | rule_string_nq[sfunc];
		}

		//float
		{
			auto ffunc = [&](auto&& f) { ptr = boost::make_shared<FloatValue>(f); };
			rule_float = float_[ffunc];
		}

		//array
		{
			rule_array = '[' >> +rule_value >> ']';
		}


		rule_value = rule_array | rule_float | rule_string;



		bool r = phrase_parse(first, last,

			//  Begin grammar
			(
				rule_value
			),
			//  End grammar

			space);

#if 0
		if (!r || first != last) // fail if we did not get a full match
			return false;
#endif
		return ptr;
	}



	using namespace boost::spirit;

	template <typename Iterator>
	struct msl_grammar : qi::grammar<Iterator, msl::Value::pointer(), ascii::space_type>
	{
		using standard_rule = qi::rule<Iterator, msl::Value::pointer(), ascii::space_type>;

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
			using namespace boost::fusion;


			

			//standard_rule rule_value = rule_float;
			
			//float
			{
				auto ffunc = [](auto& f, auto &c) 
				{ 
					at_c<0>(c.attributes) = std::make_shared<FloatValue>(f);
				};

				rule_float = float_[ffunc];
			}

			//string
			{
				auto func = [](auto&& f, auto &c)
				{ 
					at_c<0>(c.attributes) = std::make_shared<StringValue>(f);
				};

				
				quoted_string %= lexeme['"' >> +(char_ - '"') >> '"'];

				rule_string = quoted_string[func];
			}

			//array
			{
				
				rule_varray %= '[' >> *rule_value >> ']';

				auto func = [](auto&& f, auto &c)
				{
					at_c<0>(c.attributes) = std::make_shared<ArrayValue>(f);
				};
				rule_array = rule_varray[func];
			}



			rule_value = rule_float | rule_array | rule_string;
			msl = rule_value;
		}

		standard_rule msl;

		standard_rule rule_value;
		standard_rule rule_float;
		standard_rule rule_string;
		standard_rule rule_array;



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


		std::string strArr = "[34.1 2 \"Test\"]";
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

