#pragma once
#include "Base/Locale.hpp"
#include "Base/Templates.hpp"
#include "../Types/Result.hpp"

namespace Match 
{
    template <typename T>
    std::function<Result<T>(std::vector<T>)>
    annotate
    (std::function<Result<T>(std::vector<T>)> parser, std::string annotation)
    {
        return [parser, annotation](std::vector<T> tokens)
        {
            auto result = parser(tokens);
            result.annotation = annotation;
            return result;
        };
    };


    template <typename T>
    std::function<Result<T>(std::vector<T>)> 
    inOrder 
    (std::vector<std::function<Result<T>(std::vector<T>)>> matchers)
    {
        return [matchers](const std::vector<T>& original_terms)
        {
            std::vector<T> consumed;
            std::vector<T> terms(original_terms);

            for (auto matcher : matchers)
            {
                auto result = matcher(terms);
                if (result.result)
                {
                    consumed.insert(consumed.end(), result.consumed.begin(), result.consumed.end());
                    terms = result.remaining;
                }
                else
                {
                    return Result<T>(false, std::vector<T>(), original_terms);
                }
            }
            return Result<T>(true, consumed, terms);
        };
    }

    
    template <typename T>
    std::function<Result<T>(std::vector<T>)>
    just
    (T value)
    {
        auto comparator = [value](T term){ return value == term; };
        return singleTemplate<T>(comparator);
    };

    const auto startswith = [](std::string value)
    {
        auto comparator = [value](Term term){ 
            if (term.size() >= value.size())
            {
                return std::string(term.begin(), term.begin() + value.size()) == value; 
            }
            else
            {
                return false;
            }
        };
        return singleTemplate<Term>(comparator);
    };

    template <typename T>
    std::function<Result<T>(std::vector<T>)>
    optional
    (std::function<Result<T>(std::vector<T>)> matcher)
    {
        return [matcher](std::vector<T> terms)
        {
            auto result = matcher(terms);
            if (!result.result)
            {
                result.result = true;
            }
            return result;
        };
    }

    template <typename T>
    std::function<Result<T>(std::vector<T>)>
    inverse
    (std::function<Result<T>(std::vector<T>)> matcher)
    {
        return [matcher](std::vector<T> terms)
        {
            auto result = matcher(terms);
            result.result = !result.result;
            return result;
        };
    }


    // Attempt to parse any matcher from a list of matchers, failing only if all of the matchers fail, and passing if any of them pass
    template <typename T>
    std::function<Result<T>(std::vector<T>)>
    anyOf 
    (std::vector<std::function<Result<T>(std::vector<T>)>> matchers)
    {
        return [matchers](std::vector<T> terms)
        {
            auto result = Result<T>(false, std::vector<T>(), terms);
            for (auto matcher : matchers)
            {
                auto match_result = matcher(terms);
                if(match_result.result)
                {
                    result = match_result;
                    break;
                }
            }
            return result;
        };
    };


    //Parse all matchers from a list of matchers, passing only if all of them pass
    template <typename T>
    std::function<Result<T>(std::vector<T>)>
    allOf
    (std::vector<std::function<Result<T>(std::vector<T>)>> matchers)
    {
        return [matchers](std::vector<T> terms)
        {
            auto result = Result<T>(false, std::vector<T>(), terms);
            for (auto matcher : matchers)
            {
                auto match_result = matcher(terms);
                if(!match_result.result)
                {
                    result = Result<T>(false, std::vector<T>(), terms);
                    break;
                }
                else
                {
                    result = match_result;
                }
            }
            return result;
        };
    };

    // Parse any term
    template <typename T>
    std::function<Result<T>(std::vector<T>)>
    wildcard
    ()
    {
        return singleTemplate<T>([](T t){return true;});
    }

    // Takes a matcher and parses it repeatedly, never fails
    template <typename T>
    std::function<Result<T>(std::vector<T>)>
    many
    (std::function<Result<T>(std::vector<T>)> matcher)
    {
        return [matcher](std::vector<T> terms)
        {
            auto consumed = std::vector<T>(); 
            std::string annotation = "none";

            while(terms.size() > 0)
            {
                auto result = matcher(terms);
                if (result.result)
                {
                    annotation = result.annotation;
                    consumed.insert(consumed.end(), result.consumed.begin(), result.consumed.end());
                    terms = std::vector<T>(terms.begin() + result.consumed.size(), terms.end());
                }
                else
                {
                    break;
                }
            }

            auto result = Result<T>(true, consumed, terms);
            result.annotation = annotation;
            return result;
        };
    };

    template <typename T>
    std::function<Result<T>(std::vector<T>)>
    sepBy
    (std::function<Result<T>(std::vector<T>)> sep, std::function<Result<T>(std::vector<T>)> val=wildcard<T>(), std::string annotation="none")
    {
        return annotate(inOrder<T>({
        val,
        many<T>(inOrder<T>({sep, val}))
        }), annotation);
    };

    //All of these are pretty self explanatory, they check a Term to see if it is a particular group of characters
    const auto digits = singleTemplate<std::string>(is_digits);
    const auto alphas = singleTemplate<std::string>(is_alphas);
    const auto puncts = singleTemplate<std::string>(is_puncts);
    const auto uppers = singleTemplate<std::string>(is_uppers);
    const auto lowers = singleTemplate<std::string>(is_lowers);
}
