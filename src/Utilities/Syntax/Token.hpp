#pragma once
#include "Symbols.hpp"
#include "../Parser/Types/Types.hpp"

namespace Syntax
{
    using Token  = std::tuple<Parse::Term, SymbolGenerator>;
    using Tokens = std::vector<Token>;
}