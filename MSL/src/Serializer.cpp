#include "msl/Serializer.h"
#include <set>
#include <sstream>
#include <ios>

using namespace msl;

namespace
{
    struct Context
    {
        Context(std::ostream& o) : out(o) {}

        std::ostream& out;
        bool noNewlines = false;
        int indentLevel = 0;
        int maxSingleLine = 40;

        int measureSingleLine(const Value::pointer& ptr)
        {
            std::stringstream ss;
            Context ctx{ ss };
            ctx.noNewlines = true;
            ctx.write(ptr);
            ss.seekp(0, std::ios::end);
            return ss.tellp();
        }

        bool shouldBeSingleLine(const Value::pointer& ptr)
        {
            int singleSize = measureSingleLine(ptr);
            return singleSize <= maxSingleLine;
        }

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

        void writeIndent()
        {
            for(int i = 0; i < indentLevel; i ++)
                out << "\t";
        }

        void writeNull(const Value::pointer& ptr)
        {
            out << "null";
        }

        void writeBool(const Value::pointer& ptr)
        {
            out << (ptr->asBool() ? "true": "false");
        }

        void writeArrayOneLine(const Value::pointer& ptr)
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

        void writeMapOneLine(const Value::pointer& ptr)
        {
            auto& arr = ptr->asMap();
            out << "{ ";
            bool first = true;
            for (auto& [k,v] : arr)
            {
                if (!first) out << ", ";
                write(k);
                out << ": ";
                write(v);

                first = false;
            }
            out << " }";
        }

        void writeArrayMultipleLines(const Value::pointer& ptr)
        {
            auto& arr = ptr->asArray();
            out << "[\n";
            bool first = true;
            for (auto& e : arr)
            {
                write(e);
                out << "\n";
                first = false;
            }
            out << ']';
        }

        void writeMapMultipleLines(const Value::pointer& ptr)
        {
            auto& arr = ptr->asMap();
            out << "\n";
            writeIndent();
            out << "{\n";
            bool first = true;
            indentLevel++;
            for (auto& [k, v] : arr)
            {
                writeIndent();
                write(k);
                out << ": ";
                write(v);
                out << "\n";
                first = false;
            }
            indentLevel--;
            writeIndent();
            out << '}';
        }

        void writeArray(const Value::pointer& ptr)
        {
            if (noNewlines || shouldBeSingleLine(ptr))
            {
                writeArrayOneLine(ptr);
                return;
            }

            writeArrayMultipleLines(ptr);
        }

        void writeMap(const Value::pointer& ptr)
        {
            if (noNewlines || shouldBeSingleLine(ptr))
            {
                writeMapOneLine(ptr);
                return;
            }

            writeMapMultipleLines(ptr);
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
