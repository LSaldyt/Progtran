/// Copyright 2017 Lucas Saldyt
#include "symbol.hpp"
namespace syntax
{
    using String = StringLiteral;
    /*
    struct String : public StringLiteral 
    {
        String(string set_value) : StringLiteral(set_value){}
        virtual string representation(Generator& generator, unordered_set<string>& generated, string filetype, int nesting=0)
        {
            return "\"" + value + "\"";
        }
        virtual string abstract(int indent=0)
        {
            return repeatString("    ", indent) + "String(\"" + value + "\")";
        }
    };
    */
}
