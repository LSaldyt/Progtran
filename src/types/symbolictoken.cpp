/// Copyright 2017 Lucas Saldyt
#include "symbolictoken.hpp"

SymbolicToken::SymbolicToken(std::shared_ptr<syntax::Symbol> set_value, std::string set_sub_type, std::string set_type, std::string set_text, int set_line)
{
    value    = set_value;
    sub_type = set_sub_type;
    type     = set_type;
    text     = set_text;
    line     = set_line;
}
