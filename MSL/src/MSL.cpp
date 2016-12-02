#include "msl/msl.h"

#if 0
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
#endif

#include <pegtl.hh>
#include <pegtl/contrib/abnf.hh>
#include <pegtl/contrib/unescape.hh>
#include <pegtl/contrib/changes.hh>
#include <pegtl/contrib/raw_string.hh>

namespace msl
{
	template<typename T, Value::Type my_type>
	class TemplatedValue : public Value
	{
	public:
		using base_type = TemplatedValue<T, my_type>;

		TemplatedValue() : { _type = my_type; }
		TemplatedValue(const T& t) : _value(t) { _type = my_type; }
		TemplatedValue(T&& t) : _value(std::move(t)) { _type = my_type; }

	protected:
		T _value;
	};

	template<typename T, Value::Type my_type>
	class NamedTemplatedValue : public Value
	{
	public:
		using base_type = NamedTemplatedValue<T, my_type>;

		NamedTemplatedValue() : { _type = my_type; }
		NamedTemplatedValue(const std::string& name, const MapType& attr, const T& t) : _attributes(attr), _value(t), _name(name) { _type = my_type; }
		NamedTemplatedValue(const std::string& name, MapType&& attr, T&& t) : _attributes(std::move(attr)), _value(std::move(t)), _name(name) { _type = my_type; }

		const std::string& name() override { return _name; };
		const MapType& attributes() override { return _attributes; }
	protected:
		MapType _attributes;
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

	class PercentValue : public TemplatedValue<float, Value::Type::Percent>
	{
	public:
		using base_type::TemplatedValue;
		float asFloat() override { return _value; }
	};

	class ArrayValue : public TemplatedValue<Value::ArrayType, Value::Type::Array>
	{
	public:
		using base_type::TemplatedValue;
		ArrayType& asArray() override { return _value; }
	};

	class MapValue : public TemplatedValue<Value::MapType, Value::Type::Map>
	{
	public:
		using base_type::TemplatedValue;
		MapType& asMap() override { return _value; }
	};

	class NamedArrayValue : public NamedTemplatedValue<Value::ArrayType, Value::Type::Array>
	{
	public:
		using base_type::NamedTemplatedValue;
		ArrayType& asArray() override { return _value; }
	};

	class NamedMapValue : public NamedTemplatedValue<Value::MapType, Value::Type::Map>
	{
	public:
		using base_type::NamedTemplatedValue;
		MapType& asMap() override { return _value; }
	};


	class NamedNullValue : public Value
	{
	public:
		NamedNullValue(const std::string& name, const MapType& attr) : _attributes(attr), _name(name) { }

		const std::string& name() override { return _name; };
		const MapType& attributes() override { return _attributes; }
	protected:
		MapType _attributes;
		std::string _name;
	};
}

#if 0
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

	template <typename T>
	struct ts_real_policies : boost::spirit::qi::real_policies<T>
	{

		//don't parse Inf as infinity
		template <typename Iterator, typename Attribute>
		static bool
			parse_inf(Iterator& first, Iterator const& last, Attribute& attr)
		{
			return false;
		}

		//don't parse Nan as nan
		template <typename Iterator, typename Attribute>
		static bool
			parse_nan(Iterator& first, Iterator const& last, Attribute& attr)
		{
			return false;
		}
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
				at_c<0>(c.attributes) = std::make_shared<Type>(at_c<0>(f), at_c<1>(f), at_c<2>(f));
			};
		}

		auto createAttrSynthesizerForNamedNull()
		{
			return [](auto&& f, auto &c)
			{
				using namespace boost::fusion;
				at_c<0>(c.attributes) = std::make_shared<msl::NamedNullValue>(at_c<0>(f), at_c<1>(f));
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
				boost::spirit::qi::real_parser<float, ts_real_policies<float> > my_float;

				auto percent = [](auto&& f, auto &c)
				{
					at_c<0>(c.attributes) = std::make_shared<PercentValue>(f / 100.0f);
				};

				rule_float = (my_float >> '%')[percent] | my_float[createAttrSynthesizer<FloatValue>()];
			}

			//string
			{
				quoted_string %= lexeme['"' >> *(char_ - '"') >> '"'];
				simple_string %= (alpha | char_('&')) >> *(char_(R"foo(a-zA-Z0-9\!\.\-\(\)\\\/\_)foo")); //a-zA-Z0-9  !.-()\/_

				rule_string = (quoted_string | simple_string)[createAttrSynthesizer<StringValue>()];
			}

			//array
			{
				rule_varray %= '[' >> *(rule_value >> -lit(',')) >> ']';
				rule_array = rule_varray[createAttrSynthesizer<ArrayValue>()];
			}

			//map
			{
				rule_vmap %= qi::lit("{") >> *(rule_value >> qi::lit(":") >> rule_value >> -lit(',')) >> qi::lit("}");
				rule_map = rule_vmap[createAttrSynthesizer<MapValue>()];
			}

			named_string %= (alpha) >> *(char_("a-zA-Z0-9\\.\\-"));
			rule_vattr %= "(" >> *(rule_value >> qi::lit(":") >> rule_value >> -lit(',')) >> ")";

			//named null
			{
				rule_named_null = (named_string >> rule_vattr)[createAttrSynthesizerForNamedNull()];
			}

			//named array
			{
				rule_named_array = (named_string >> rule_vattr >> rule_varray)[createAttrSynthesizerForNamed<NamedArrayValue>()];
			}

			//named map
			{
				rule_named_map = (named_string >> rule_vattr >> rule_vmap)[createAttrSynthesizerForNamed<NamedMapValue>()];
			}

			rule_value = rule_float | rule_array | rule_map | rule_named_array | rule_named_map | rule_named_null | rule_bool | rule_null | rule_string;
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
		standard_rule rule_named_null;

		qi::rule<Iterator, msl::Value::MapType(), skipper<Iterator>> rule_vattr;
		qi::rule<Iterator, msl::Value::MapType(), skipper<Iterator>> rule_vmap;
		qi::rule<Iterator, msl::Value::ArrayType(), skipper<Iterator>> rule_varray;
		qi::rule<Iterator, std::string()> quoted_string;

		qi::rule<Iterator, std::string()> simple_string;

		qi::rule<Iterator, std::string()> named_string;
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
		if (first != last)
			return nullptr;

		return ast;
	}



}
#endif

namespace pegtl
{
namespace msl
{

    //struct ws : one< ' ', '\t', '\n', '\r' > {};

    struct comment : disable< pegtl_string_t( "/*" ), pegtl::until< pegtl_string_t( "*/" ) > > {};

    struct sep : sor< pegtl::ascii::space, comment > {};
    struct ws : sor<sep, one< '\t', '\n', '\r' >> {};


    template< typename R, typename P = ws > struct padr : internal::seq< R, internal::star< P > > {};

    struct begin_array : padr< one< '[' > > {};
    struct end_array : one< ']' > {};
    struct begin_object : padr< one< '{' > > {};
    struct end_object : one< '}' > {};
    struct begin_attr : padr< one< '(' > > {};
    struct end_attr : one< ')' > {};
    struct name_separator : pad< one< ':' >, ws > {};
    struct value_separator : padr< opt<one< ',' >> > {}; //separator is a , or a white space

    struct false_ : pegtl_string_t( "false" ) {};
    struct null : pegtl_string_t( "null" ) {};
    struct true_ : pegtl_string_t( "true" ) {};

    //number
    struct digits : plus< abnf::DIGIT > {};
    struct exp : seq< one< 'e', 'E' >, opt< one< '-', '+'> >, must< digits > > {};
    struct frac : if_must< one< '.' >, digits > {};
    struct int_ : sor< one< '0' >, digits > {};
    struct number : seq< opt< one< '-' > >, int_, opt< frac >, opt< exp > > {};

    //string
    struct xdigit : abnf::HEXDIG {};
    struct unicode : list< seq< one< 'u' >, rep< 4, must< xdigit > > >, one< '\\' > > {};
    struct escaped_char : one< '"', '\\', '/', 'b', 'f', 'n', 'r', 't' > {};
    struct escaped : sor< escaped_char, unicode > {};
    struct unescaped : utf8::range< 0x20, 0x10FFFF > {};
    struct char_ : if_then_else< one< '\\' >, must< escaped >, unescaped > {};

    struct string_content : until< at< one< '"' > >, must< char_ > > {};
    struct string : seq< one< '"' >, must< string_content >, any >
    {
        using content = string_content;
    };

    //simplestring
    struct simplestringchar : sor<alnum, one<'!','.','-','<','>','\\','/','_'>> {};
    struct simplestringchar_prefix : sor<alnum, one<'&'>> {};

    struct simplestring : seq< simplestringchar_prefix , star< simplestringchar >>
    {
    };

    //array
    struct array_element;
    struct array_content : opt< list< array_element, value_separator > > {};
    struct array : seq< begin_array, array_content, must< end_array > >
    {
        using begin = begin_array;
        using end = end_array;
        using element = array_element;
        using content = array_content;
    };

    //object
    struct value;
    struct member : if_must< value, name_separator, value > {};
    struct object_content : opt< list< member, value_separator > > {};
    struct object : seq< begin_object, object_content, must< end_object > >
    {
        using begin = begin_object;
        using end = end_object;
        using element = member;
        using content = object_content;
    };


    //attr array
	struct attributes_content : opt< list< member, value_separator > > {};
    struct attributes : seq< begin_attr, attributes_content, must< end_attr > >
    {
        using begin = begin_attr;
        using end = end_attr;
        using element = member;
        using content = attributes_content;
    };

    //named array/object/null
    struct named_value_name : seq< simplestring, attributes> {};
    struct named_value_type : sor< object, array> {};
    struct named_value : seq< named_value_name, opt<named_value_type> > {};

    struct value : padr< sor< named_value, string, number, simplestring, array, object, false_, true_, null > > {};
    struct array_element : seq< value > {};
    //struct value : padr< sor< string, number, object, array, false_, true_, null > > {};

    struct text : seq< star< ws >, value > {};
}
}


struct unescape_state_base
{
    unescape_state_base() = default;

    unescape_state_base( const unescape_state_base & ) = delete;
    void operator= ( const unescape_state_base & ) = delete;

    std::string unescaped;
};

// Action class for parsing literal strings, uses the PEGTL unescape utilities, cf. unescape.cc.

template< typename Rule, template< typename ... > class Base = pegtl::nothing >
struct unescape_action : Base< Rule > {};

template<> struct unescape_action< pegtl::msl::unicode > : pegtl::unescape::unescape_j {};
template<> struct unescape_action< pegtl::msl::escaped_char > : pegtl::unescape::unescape_c< pegtl::msl::escaped_char, '"', '\\', '/', '\b', '\f', '\n', '\r', '\t' > {};
template<> struct unescape_action< pegtl::msl::unescaped > : pegtl::unescape::append_all {};


struct result_state
{
    result_state() = default;

    result_state( const result_state & ) = delete;
    void operator= ( const result_state & ) = delete;

    msl::Value::pointer result;
};


template< typename Rule > struct value_action : unescape_action< Rule > {};



   struct string_state
         : public unescape_state_base
   {
      void success( result_state & result )
      {
         result.result = std::make_shared< msl::StringValue >( std::move( unescaped ) );
      }
   };

   template<>
   struct value_action< pegtl::msl::simplestring >
   {
      template< typename Input >
      static void apply( const Input & in, result_state & result )
      {
         result.result = std::make_shared< msl::StringValue >( in.string() ); 
      }
   };

   template<>
   struct value_action< pegtl::msl::null >
   {
      template< typename Input >
      static void apply( const Input &, result_state & result )
      {
         result.result = std::make_shared< msl::Value >();
      }
   };

   template<>
   struct value_action< pegtl::msl::true_ >
   {
      template< typename Input >
      static void apply( const Input &, result_state & result )
      {
         result.result = std::make_shared< msl::BoolValue >( true );
      }
   };

   template<>
   struct value_action< pegtl::msl::false_ >
   {
      template< typename Input >
      static void apply( const Input &, result_state & result )
      {
         result.result = std::make_shared< msl::BoolValue >( false );
      }
   };

   template<>
   struct value_action< pegtl::msl::number >
   {
      template< typename Input >
      static void apply( const Input & in, result_state & result )
      {
         result.result = std::make_shared< msl::FloatValue >( std::stof( in.string() ) );  // NOTE: stold() is not quite correct for JSON but we'll use it for this simple example.
      }
   };

   // State and action classes to accumulate the data for a JSON array.

   struct array_state
         : public result_state
   {
      std::vector< msl::Value::pointer > array;

      void push_back()
      {
         array.push_back( std::move( result ) );
         result.reset();
      }

      void success( result_state & in_result )
      {
         if ( this->result ) {
            push_back();
         }
         in_result.result = std::make_shared<msl::ArrayValue>(std::move(array));
      }
   };

   template< typename Rule > struct array_action : pegtl::nothing< Rule > {};

   template<>
   struct array_action< pegtl::msl::value_separator >
   {
      template< typename Input >
      static void apply( const Input &, array_state & result )
      {
         result.push_back();
      }
    };


   // State and action classes to accumulate the data for a JSON object.
   struct object_state
         : public result_state
   {
      msl::Value::MapType object;
      msl::Value::pointer key;


      void insert_key()
      {
          key = result;
      }

      void insert()
      {
        object[key] = result;
        key.reset();
        result.reset();
      }

      void success( result_state & in_result )
      {
         if ( this->result ) {
            insert();
         }
         in_result.result = std::make_shared<msl::MapValue>(std::move(object));
      }
   };

   template< typename Rule > struct object_action : unescape_action< Rule > {};

   template<>
   struct object_action< pegtl::msl::name_separator >
   {
      template< typename Input >
      static void apply( const Input &, object_state & result )
      {
         result.insert_key();
      }
    };

   template<>
   struct object_action< pegtl::msl::value_separator >
   {
      template< typename Input >
      static void apply( const Input &, object_state & result )
      {
         result.insert();
      }
    };


   //named value
   struct named_value_state
         : public result_state
   {
       msl::Value::MapType attr;
       std::string name;
       msl::Value::pointer key;
	   msl::Value::pointer type;

	void success( result_state & in_result )
	{
		if (!type)
			in_result.result = std::make_shared<msl::NamedNullValue>(name, attr);
		else if (type->type() == msl::Value::Type::Map)
		{
			in_result.result = std::make_shared<msl::NamedMapValue>(name, attr, std::move(type->asMap()));
		}
		else
		{
			in_result.result = std::make_shared<msl::NamedArrayValue>(name, attr, std::move(type->asArray()));
		}

		type.reset();
	}

	void insert_key()
	{
		key = result;
		result.reset();
		key.reset();
	}

	void insert_value()
	{
		attr[key] = result;
		key.reset();
		result.reset();
	}

	void insert_type()
	{
		type = result;
	}
   };

   template< typename Rule > struct named_value_action : pegtl::nothing< Rule > {};

   template<>
   struct named_value_action< pegtl::msl::simplestring >
   {
      template< typename Input >
      static void apply( const Input & in, named_value_state & result )
      {
         result.name = in.string(); 
      }
   };

   template<>
   struct named_value_action< pegtl::msl::named_value_type >
   {
	   template< typename Input >
	   static void apply( const Input &, named_value_state & result )
	   {
		   result.insert_type();
	   }
   };
#if 0
   template<>
   struct named_value_action< pegtl::msl::name_separator >
   {
      template< typename Input >
      static void apply( const Input &, named_value_state & result )
      {
         
      }
    };

   template<>
   struct named_value_action< pegtl::msl::value_separator >
   {
      template< typename Input >
      static void apply( const Input &, named_value_state & result )
      {
         
      }
    };
   template<>
   struct named_value_action< pegtl::msl::begin_attr >
   {
	   template< typename Input >
	   static void apply( const Input &, named_value_state & result )
	   {

	   }
   };
#endif



   template< typename Rule > struct attribute_action : pegtl::nothing< Rule > {};

   template<>
   struct attribute_action< pegtl::msl::name_separator >
   {
	   template< typename Input >
	   static void apply( const Input &, named_value_state & result )
	   {
		   result.insert_key();
	   }
   };

   template<>
   struct attribute_action< pegtl::msl::value_separator >
   {
	   template< typename Input >
	   static void apply( const Input &, named_value_state & result )
	   {
		   result.insert_value();
	   }
   };



   template< typename Rule > struct control : public pegtl::normal< Rule > {};  // Inherit from json_errors.hh.

   template<> struct control< pegtl::msl::value > : pegtl::change_action< pegtl::msl::value, value_action > {};
   template<> struct control< pegtl::msl::string::content > : pegtl::change_state< pegtl::msl::string::content, string_state > {};
   template<> struct control< pegtl::msl::array::content > : pegtl::change_state_and_action< pegtl::msl::array::content, array_state, array_action > {};
   template<> struct control< pegtl::msl::object::content > : pegtl::change_state_and_action< pegtl::msl::object::content, object_state, object_action > {};
   template<> struct control< pegtl::msl::named_value > : pegtl::change_state_and_action< pegtl::msl::named_value, named_value_state, named_value_action > {};
   template<> struct control< pegtl::msl::attributes::content > : pegtl::change_action< pegtl::msl::attributes::content, attribute_action > {};

   struct grammar : pegtl::must< pegtl::msl::text, pegtl::eof > {};


msl::Value::pointer msl::Value::fromString(const std::string &str)
{
    //std::string tst = "[1 test \"test2\" ]";
    //std::string tst = "{ 2: test() }";
    //std::string tst = "test( a:1 )[]";
    result_state result;
    try
    {
        pegtl::parse_string<grammar, value_action, control>( str, "test", result );
    }
    catch ( ... )
    {
        return nullptr;
    }
    
    return result.result;
}