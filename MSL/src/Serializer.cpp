#include "msl/Serializer.h"
#include <set>

using namespace msl;

namespace
{
    struct Context
    {
        Context(std::ostream& o) : out(o) {}

        std::ostream& out;
        int indentLevel = 0;

        void writeFloat(const Value::pointer& ptr)
        {
            auto f = ptr->asFloat();
            out << f;
        }

        void writeString(const Value::pointer& ptr)
        {

            auto isSimple = [](const std::string& str) -> bool
            {
                static std::set<char> additionalChars = { '!','.','-','<','>','\\','/','_' };

                for (int i = 0; i < str.size(); i++)
                {
                    auto c = str[i];
                    if (i == 0)
                    {
                        return isalpha(c) || c == '&';
                    }
                    return isalnum(c) || additionalChars.find(c) != additionalChars.end();
                }
            };

            auto str = ptr->asString();
            if (isSimple(str))
            {
                out << str;
                return;
            }
            out << '"' << str << '"';
        }

        void writeNull(const Value::pointer& ptr)
        {
            out << "null";
        }

        void writeBool(const Value::pointer& ptr)
        {
            out << (ptr->asBool() ? "true": "false");
        }

        void writeArray(const Value::pointer& ptr)
        {
            auto& arr = ptr->asArray();
            out << '[';
            bool first = true;
            for (auto& e : arr)
            {
                if (!first) out << ", ";
                write(e);

                first = false;
            }
            out << ']';
        }

        void writeMap(const Value::pointer& ptr)
        {
            auto& arr = ptr->asMap();
            out << '{';
            bool first = true;
            for (auto& [k,v] : arr)
            {
                if (!first) out << ", ";
                write(k);
                out << ": ";
                write(v);

                first = false;
            }
            out << '}';
        }

        void writeParams(const Value::pointer& ptr)
        {
            out << ptr->name();
            auto& arr = ptr->attributes();
            out << '(';
            bool first = true;
            for (auto& [k, v] : arr)
            {
                if (!first) out << ", ";
                write(k);
                out << ": ";
                write(v);

                first = false;
            }
            out << ')';
        }

        void write(const Value::pointer& ptr)
        {
            if (!ptr->name().empty())
            {
                writeParams(ptr);
            }
            auto type = ptr->type();
            //String, Float, Boolean, Null, Array, Map, Percent
            switch (type)
            {
            case msl::Value::Type::Percent:
            case msl::Value::Type::Float:
                return writeFloat(ptr);
            case msl::Value::Type::String:
                return writeString(ptr);
            case msl::Value::Type::Boolean:
                return writeBool(ptr);
            case msl::Value::Type::Null:
                return writeNull(ptr);
            case msl::Value::Type::Array:
                return writeArray(ptr);
            case msl::Value::Type::Map:
                return writeMap(ptr);

            }
        }
    };
}


void Serializer::Write(const msl::Value::pointer& ptr, std::ostream& out)
{
    Context ctx{ out };
    ctx.write(ptr);
}
