#include "MSL.h"

#define BOOST_RESULT_OF_USE_DECLTYPE
#define BOOST_SPIRIT_USE_PHOENIX_V3
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/qi_char_class.hpp>
#include <boost/make_shared.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/spirit/repository/include/qi_confix.hpp>


namespace msl
{
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


	class BoolValue : public TemplatedValue<bool, Value::Type::Boolean>
	{
	public:
		using base_type::TemplatedValue;
		bool asBool() override { return _value; }
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
}


namespace client
{
	namespace qi = boost::spirit::qi;
	namespace ascii = boost::spirit::ascii;

	///////////////////////////////////////////////////////////////////////////////
	//  The skipper grammar
	///////////////////////////////////////////////////////////////////////////////
	template <typename Iterator>
	struct skipper : qi::grammar<Iterator>
	{
		skipper() : skipper::base_type(start)
		{
			qi::char_type char_;
			ascii::space_type space;

			start =
				space                               // tab/space/cr/lf
				| "/*" >> *(char_ - "*/") >> "*/"   // C-style comments
				| "//" >> *(char_ - eol) >> eol     // C++-style comments
				;
		}

		qi::rule<Iterator> start;
	};




	using namespace boost::spirit;

	template <typename Iterator>
	struct msl_grammar : qi::grammar<Iterator, msl::Value::pointer(), skipper<Iterator>>
	{
		using standard_rule = qi::rule<Iterator, msl::Value::pointer(), skipper<Iterator>>;


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

			//null
			{
				auto rule_null_f = [](auto&& f, auto &c) { at_c<0>(c.attributes) = std::make_shared<Value>(); };
				rule_null = lit("null")[rule_null_f];
			}

			//bool
			{
				auto rule_true_f = [](auto&& f, auto &c) {at_c<0>(c.attributes) = std::make_shared<BoolValue>(true);	};
				auto rule_false_f = [](auto&& f, auto &c) {at_c<0>(c.attributes) = std::make_shared<BoolValue>(false);	};
				rule_bool = lit("true")[rule_true_f] | lit("false")[rule_false_f];
			}

			//float
			{
				auto percent = [](auto&& f, auto &c)
				{
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

			rule_value = rule_float | rule_array | rule_map | rule_named_array | rule_named_map | rule_bool | rule_null | rule_string;
			msl = rule_value;
		}

		standard_rule msl;

		standard_rule rule_value;
		standard_rule rule_float;
		standard_rule rule_bool;
		standard_rule rule_null;
		standard_rule rule_string;
		standard_rule rule_array;
		standard_rule rule_map;

		standard_rule rule_named_array;
		standard_rule rule_named_map;

		qi::rule<Iterator, msl::Value::MapType(), skipper<Iterator>> rule_vmap;
		qi::rule<Iterator, msl::Value::ArrayType(), skipper<Iterator>> rule_varray;
		qi::rule<Iterator, std::string()> quoted_string;

		qi::rule<Iterator, std::string()> simple_string;
	};

	template <typename Iterator>
	msl::Value::pointer parse_msl(Iterator first, Iterator last)
	{
		msl_grammar<Iterator> msl; // Our grammar
		msl::Value::pointer ast; // Our tree

		using boost::spirit::ascii::space;
		using boost::spirit::repository::confix;
		using ascii::char_;


		//qi::rule<Iterator> 
		skipper<Iterator> skipper;

		bool r = phrase_parse(first, last, msl, skipper, ast);

		return ast;
	}



}


msl::Value::pointer msl::Value::fromString(const std::string &str)
{
	return client::parse_msl(str.begin(), str.end());
}