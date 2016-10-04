#include "grammar.hpp"
#include <exception>

namespace Grammar
{

const std::unordered_map<std::string, StatementConstructor> Grammar::construction_map = {
        {"expression", 
            [](std::vector<std::shared_ptr<Symbol>> tokens)
            {
                Expression e;
                std::cout << tokens.size() << std::endl;
                e.base = tokens[0];
                if (tokens.size() > 1)
                {
                    if ((tokens.size() - 1) % 2 != 0)
                    {
                        std::cout << "Cannot build expression extension from odd number of tokens" << std::endl;
                        throw std::exception();
                    }

                    for (int i = 1; i < tokens.size(); i += 2)
                    {
                        e.extensions.push_back(std::make_tuple(tokens[i], tokens[i + 1]));
                    }
                }

                return std::make_shared<Expression>(e);
            }
        },
        {"assignment",
            [](std::vector<std::shared_ptr<Symbol>> tokens)
            {
                return std::make_shared<Assignment>(Assignment(tokens));
            }
        },
        {"functioncall",
            [](std::vector<std::shared_ptr<Symbol>> tokens)
            {
                return std::make_shared<FunctionCall>(FunctionCall(tokens));
            }
        },
        {"value",
            [](std::vector<std::shared_ptr<Symbol>> tokens)
            { 
                return std::make_shared<Symbol>(Symbol());
            }
        }

   };

Grammar::Grammar::Grammar(std::vector<std::string> filenames, std::string directory)
{
    for (auto filename : filenames)
    {
        grammar_map[filename] = read(directory + filename);
    }
}

std::vector<std::shared_ptr<Symbol>> Grammar::constructFrom(SymbolicTokens& tokens)
{
    std::vector<std::shared_ptr<Symbol>> symbols;

    while (tokens.size() > 0)
    {
        auto result = identify(tokens);
        std::cout << "Identified tokens as " << std::get<0>(result) << std::endl;
        for (auto sub_result : std::get<1>(result))
        {
            for (auto t : sub_result.consumed)
            {
                std::cout << t.value->representation() << std::endl;
            }
        }
        auto constructed = construct(std::get<0>(result), std::get<1>(result)); 
        symbols.push_back(constructed);
    }

    return symbols;
}


SymbolicTokenParsers Grammar::Grammar::readGrammarPairs(std::vector<std::string>& terms)
{
    SymbolicTokenParsers parsers;

    if (terms.size() % 2 != 0)
    {
        std::cout << "Could not read type pairs:" << std::endl;
        for (auto t : terms)
        {
            std::cout << t << " ";
        }
        std::cout << std::endl;
        throw std::exception();
    }
    for (int i = 0; i < (terms.size() / 2); i++)
    {
        int x = i * 2;
        std::vector<std::string> pair(terms.begin() + x, terms.begin() + x + 2);
        parsers.push_back(readGrammarTerms(pair));
    }

    return parsers;
}

SymbolicTokenParser Grammar::Grammar::readGrammarTerms(std::vector<std::string>& terms)
{
    SymbolicTokenParser parser;

    if (terms.size() == 2)
    {
        auto first = terms[0];
        bool keep  = true;
        // If first of pair starts with !, discard its parse result
        if (first[0] == '!')
        {
            first = std::string(first.begin() + 1, first.end());
            keep = false;
        }
        // Allow linking to other grammar files
        if (first == "link")
        {
            parser = retrieveGrammar(terms[1]);
            parser = annotate(parser, terms[1]); // Mark the link as such
        }
        // Parse by type only
        else if (terms[1] == "wildcard")
        {
            parser = typeParser(first);
        }
        // Parse by a specific subtype (ex "keyword return")
        else
        {
            if (first == "keyword")
            {
                keywords.push_back(terms[1]);
            }
            parser = dualTypeParser(first, terms[1]);
        }

        // Take care of a "!" if it was found early - make the parser discard its result
        if (not keep)
        {
            parser = discard(parser);
        }
    }
    else if (terms.size() > 2)
    {
        const auto keyword = terms[0];
        terms = std::vector<std::string>(terms.begin() + 1, terms.end());

        // Repeatedly parse a parser!
        if (keyword == "many")
        {
            parser = many<SymbolicToken>(readGrammarTerms(terms)); 
        }
        // Optionally parse a parser
        else if (keyword == "optional")
        {
            parser = optional<SymbolicToken>(readGrammarTerms(terms));
        }
        else if (keyword == "annotate")
        // Annotate a parser for 
        {
            std::vector<std::string> remaining(terms.begin() + 1, terms.end());
            parser = annotate<SymbolicToken>(readGrammarTerms(remaining), terms[0]);
        }
        // Run several parsers in order, failing if any of them fail
        else if (keyword == "inOrder")
        {
            parser = inOrder<SymbolicToken>(readGrammarPairs(terms));
        }
        // Choose from several parsers
        else if (keyword == "anyOf")
        {
            parser = anyOf<SymbolicToken>(readGrammarPairs(terms));
        }
        else
        {
            std::cout << "Expected keyword..." << std::endl;
            throw std::exception();
        }
    }
    else
    {
        std::cout << "Grammar file incorrectly formatted: " << std::endl;
        for (auto t : terms)
        {
            std::cout << t << " ";
        }
        std::cout << std::endl;
        throw std::exception();
    }

    return parser;
}

std::tuple<SymbolicTokenParsers, std::vector<int>> Grammar::Grammar::read(std::string filename)
{
    SymbolicTokenParsers parsers;
    auto content = readFile(filename);
    auto construct_line = content.back();
    content = std::vector<std::string>(content.begin(), content.end() - 1);
    
    for (auto line : content)
    {
        auto terms = Lex::seperate(line, {std::make_tuple(" ", false)});
        parsers.push_back(readGrammarTerms(terms));
    }

    std::vector<int> construct_indices;
    auto construct_terms = Lex::seperate(construct_line, {std::make_tuple(" ", false)});
    for (auto t : construct_terms)
    {
        construct_indices.push_back(std::stoi(t));
    }

    return std::make_tuple(parsers, construct_indices);
}

SymbolicTokenParser Grammar::Grammar::retrieveGrammar(std::string filename)
{
    SymbolicTokenParser grammar_parser = [filename, this](SymbolicTokens tokens)
    {
        SymbolicTokenParser parser = typeParser("literal");

        auto search = grammar_map.find(filename);
        if (search != grammar_map.end())
        {
             parser = inOrder<SymbolicToken>(std::get<0>(search->second));
        }
        else
        {
            std::cout << filename << " is not an element of the grammar map" << std::endl;
            throw std::exception();
        }

        Result<SymbolicToken> result = parser(tokens);
        result.annotation = filename;
        return result;
    };
    return grammar_parser;
}


std::tuple<std::string, std::vector<Result<SymbolicToken>>> 
Grammar::identify
(SymbolicTokens& tokens)
{
    SymbolicTokens tokens_copy(tokens);

    std::vector<std::string> keys;
    keys.reserve(grammar_map.size());
    for (auto kv : grammar_map)
    {
        keys.push_back(kv.first);
    }

    // Sort keys by the lengths of the parsers they refer to
    std::sort(keys.begin(), keys.end(),
                      [this] (auto a, auto b) 
                      {
                          auto a_len = std::get<0>(grammar_map[a]).size();
                          auto b_len = std::get<0>(grammar_map[b]).size();
                          return a_len > b_len; 
                      });

    for (auto key : keys)
    {
        std::cout << "Attempting to identify as: " << key << std::endl;

        auto value   = grammar_map[key];
        auto parsers = std::get<0>(value);
        auto result  = evaluateGrammar(parsers, tokens_copy);

        if (std::get<0>(result))
        {
            tokens = tokens_copy; // Apply our changes once we know the tokens were positively identified
            return std::make_tuple(key, std::get<1>(result));
        }
        else
        {
            tokens_copy = tokens;
        }
    }

    std::cout << "Could not identify tokens" << std::endl;
    throw std::exception();
}

std::tuple<bool, std::vector<Result<SymbolicToken>>> 
Grammar::evaluateGrammar
(SymbolicTokenParsers parsers, SymbolicTokens& tokens)
{
    std::vector<Result<SymbolicToken>> results;

    for (auto parser : parsers)
    {
        auto result = parser(tokens);
        if (result.result)
        {
            tokens = result.remaining;
            results.push_back(result);
        }
        else
        {
            std::cout << "Failed. Remaining tokens were: " << std::endl;
            for (auto t : tokens)
            {
                std::cout << t.value->representation() << std::endl;
            }
            return std::make_tuple(false, results);
        }
    }

    return std::make_tuple(true, results);
};


std::vector<std::shared_ptr<Symbol>> fromTokens(std::vector<SymbolicToken> tokens)
{
    std::vector<std::shared_ptr<Symbol>> symbols;
    symbols.reserve(tokens.size());

    for (auto t : tokens)
    {
        symbols.push_back(t.value);
    }

    return symbols;
}

std::shared_ptr<Symbol> Grammar::build(std::string name, std::vector<std::shared_ptr<Symbol>> symbols)
{
    StatementConstructor constructor;
    auto it = Grammar::construction_map.find(name);
    if (it != Grammar::construction_map.end())
        constructor = it->second;
    else
    {
        std::cout << name << " is not an element of the construction map" << std::endl;
        throw std::exception();
    }

    auto constructed = constructor(symbols);
    return constructed;
}

std::shared_ptr<Symbol> Grammar::construct(std::string name, std::vector<Result<SymbolicToken>> results)
{
    std::cout << "Constructing " << name << std::endl;
    auto construction_indices = std::get<1>(grammar_map[name]);

    std::vector<std::shared_ptr<Symbol>> result_symbols;

    for (auto i : construction_indices)
    {
        auto result = results[i];
        result.consumed = clean(result.consumed); // Discard tokens that have been marked as unneeded

        if (result.annotation == "none")
        {
            for (auto t : result.consumed)
            {
                result_symbols.push_back(t.value);
            }
        }
        else if (result.consumed.size() > 0) 
        {
            std::cout << "Building sub-symbol " << result.annotation << std::endl;
            auto constructed = build(result.annotation, fromTokens(result.consumed));
            result_symbols.push_back(constructed);
        }
    }

    auto constructed = build(name, result_symbols);
    return constructed; 
}

}
