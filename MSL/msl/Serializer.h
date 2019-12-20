#pragma once
#include <ostream>
#include "MSL.h"

namespace msl
{
    class Serializer
    {
    public:
        static void Write(const msl::Value::pointer& ptr, std::ostream& out);
    };
}