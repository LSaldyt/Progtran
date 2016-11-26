#include "compiler.hpp"


int main(int argc, char* argv[])
{
    using namespace compiler;

    vector<string> args;
    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
        {
            args.push_back(argv[i]);
        }
    }

    assert(args.size() > 2);

    string from = args[0];
    string to   = args[1];

    auto grammar   = loadGrammar(from);
    auto generator = loadGenerator(to);

    vector<string> files = slice(args, 2);
    string input_directory  = "";
    string output_directory = "";

    for (auto& file : files)
    {
        compile(file, grammar, generator, "input", "output");
    }
    print("Compilation finished");
}

namespace compiler
{
    Grammar loadGrammar(string language)
    {
        auto grammar_files = readFile("languages/" + language + "/grammar/core");
        auto grammar = Grammar(grammar_files, "languages/" + language + "/grammar/");


        auto operators        = readFile("languages/" + language + "/grammar/operators");
        auto logicaloperators = readFile("languages/" + language + "/grammar/logicaloperators"); 
        auto punctuators      = readFile("languages/" + language + "/grammar/punctuators");

        LanguageTermSets term_sets;
        term_sets.push_back(make_tuple(grammar.keywords,  "keyword"));
        term_sets.push_back(make_tuple(logicaloperators, "logicaloperator"));
        term_sets.push_back(make_tuple(operators, "operator"));
        term_sets.push_back(make_tuple(punctuators, "punctuator"));

        LanguageLexers lexer_set = {
            LanguageLexer(digits, "int", "literal", 3),
            LanguageLexer(startswith("\""), "string", "literal", 1),
            LanguageLexer(identifiers, "identifier", "identifier", 3)};

        Language test_language(term_sets, lexer_set);
        grammar.language = test_language;
        return grammar;
    }

    Generator loadGenerator(string language)
    {
        auto constructor_files = readFile("languages/" + language + "/constructors/core");
        return Generator(constructor_files, "languages/" + language + "/constructors/");
    }

    void compile(string filename, Grammar& grammar, Generator& generator, string input_directory, string output_directory)
    {
        print("Reading File");
        auto content         = readFile     (input_directory + "/" + filename);
        print("Lexing terms");
        auto tokens          = tokenPass    (content, grammar.language); 
        print("Creating symbols");
        auto symbolic_tokens = symbolicPass (tokens);
        print("Joining symbolic tokens");
        auto joined_tokens   = join         (symbolic_tokens);

        for(auto jt : joined_tokens)
        {
            print("Joined Token: " + jt.type + ", " + jt.sub_type + ", " + jt.text);
        }

        print("Constructing from grammar:");
        unordered_set<string> names;

        auto identified_groups = grammar.identifyGroups(joined_tokens);
        for (auto identified_group : identified_groups)
        {
            print(get<0>(identified_group));
            auto generated = generator(names, get<1>(identified_group), get<0>(identified_group), filename);
            for (auto fileinfo : generated)
            {
                string type         = get<0>(fileinfo);
                string path         = get<1>(fileinfo);
                vector<string> body = get<2>(fileinfo);
                print("Created " + type + " file:");
                for (auto line : body)
                {
                    print("    " + line);
                }
                writeFile(body, output_directory + "/" + path);
            }
        }
    }

    std::vector<Tokens> tokenPass(std::vector<std::string> content, const Language& language)
    {
        std::vector<Tokens> tokens;
        for (auto line : content)
        {
            print("Lexing: " + line);
            tokens.push_back(lexWith(line, language));
        }
        return tokens;
    }

    std::vector<SymbolicTokens> symbolicPass(std::vector<Tokens> tokens)
    {
        std::vector<SymbolicTokens> symbolic_tokens;
        for (auto token_group : tokens)
        {
            symbolic_tokens.push_back(toSymbolic(generatorMap, token_group));
        }
        return symbolic_tokens;
    }

    SymbolicTokens join(std::vector<SymbolicTokens> token_groups)
    {
        auto tokens = SymbolicTokens();
        for (auto token_group : token_groups)
        {
            for (auto t : token_group)
            {
                tokens.push_back(t);
            }
        }
        return tokens;
    }
}
