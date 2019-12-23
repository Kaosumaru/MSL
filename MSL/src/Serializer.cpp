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

        struct Info
        {
            int indentLevel = 0;
            msl::Value::Type type = msl::Value::Type::Null;
        };

        std::ostream& out;
        bool noNewlines = false;
        int maxSingleLine = 40;
        bool isObject = false;
        std::vector<Info> stack = { Info{} };

        auto& ctx() { return stack.back(); }

        void writeIndent()
        {
            for (int i = 0; i < ctx().indentLevel; i++)
                out << "\t";
        }

        void incIndent(msl::Value::Type type = msl::Value::Type::Null)
        {
            auto next = ctx();
            next.indentLevel++;
            next.type = type;
            stack.push_back(next);
        }

        void decIndent()
        {
            stack.pop_back();
        }

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
                    if (c == ' ') return false;
                    if (i == 0)
                    {
                        if (!(isalpha(c) || c == '&'))
                            return false;
                    }
                    if (!(isalnum(c) || additionalChars.find(c) != additionalChars.end()))
                        return false;
                }
                return true;
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
            if (ctx().type == msl::Value::Type::Map || isObject)
            {
                out << "\n";
                writeIndent();
            }
            
            out << "[\n";
            incIndent(msl::Value::Type::Array);
            for (auto& e : arr)
            {
                writeIndent();
                write(e);
                out << "\n";
            }
            decIndent();

            writeIndent();
            out << ']';
        }

        void writeMapMultipleLines(const Value::pointer& ptr)
        {
            auto& arr = ptr->asMap();
            if (ctx().type == msl::Value::Type::Map || isObject)
            {
                out << "\n";
                writeIndent();
            }
            
            out << "{\n";
 
            incIndent(msl::Value::Type::Map);
            for (auto& [k, v] : arr)
            {
                writeIndent();
                write(k);
                out << ": ";
                write(v);
                out << "\n";
            }
            decIndent();

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
            isObject = false;
            if (!ptr->name().empty())
            {
                isObject = true;
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
