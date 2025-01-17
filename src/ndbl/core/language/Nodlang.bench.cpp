#include <benchmark/benchmark.h>
#include <random>
#include "ndbl/core/language/Nodlang.h"
#include "ndbl/core/Graph.h"
#include "ndbl/core/ASTNodeUtils.h"
#include "tools/core/reflection/reflection"
#include "tools/core/string.h"

/*
 *
 * EXAMPLE:
 * --------

    #include <benchmark/benchmark.h>

    static void BM_SomeFunction(benchmark::State& state) {
      // Perform setup here
      for (auto _ : state) {
        // This code gets timed
        SomeFunction();
      }
    }
    // Register the function as a benchmark
    BENCHMARK(BM_SomeFunction);
    // Run the benchmark
    BENCHMARK_MAIN();

 */

using namespace ndbl;
using namespace tools;

class NodlangFixture : public benchmark::Fixture {
public:
    Nodlang*           language;
    Graph*             graph;
    std::random_device random_device;  // Will be used to obtain a seed for the random number engine
    std::mt19937       generator; // Standard mersenne_twister_engine
    std::uniform_real_distribution<double> distribution;

    NodlangFixture()
        : generator(random_device())
        , distribution(-100000.0, 100000.0)
    {}

    void SetUp(const ::benchmark::State& state)
    {
        language = init_language();;
        graph    = new Graph(factory);
        log_set_verbosity(Verbosity_Error);
    }

    void TearDown(const ::benchmark::State& state)
    {
        delete graph;
        shutdown_language(language);
    }

    std::string get_random_double_as_string()
    {
        return std::to_string( distribution(generator) );
    }
};

BENCHMARK_DEFINE_F(NodlangFixture, parse_token__a_single_double)(benchmark::State& state) {
    std::array<std::string, 500> double_as_str;
    for(size_t i = 0; i < double_as_str.size(); ++i)
    {
        double_as_str[i] = get_random_double_as_string();
    }
    size_t j = 0;
    for (auto _ : state)
    {
        language->parse_token(double_as_str[j%double_as_str.size()]);
        ++j;
    }
}

BENCHMARK_DEFINE_F(NodlangFixture, parse_token__a_single_boolean)(benchmark::State& state) {
    bool b = true;
    for (auto _ : state)
    {
        language->parse_token(b ? "true" : "false");
        b = !b;
    }
}

BENCHMARK_DEFINE_F(NodlangFixture, parse_token__a_single_operator)(benchmark::State& state) {
    std::array operations{ "+", "-", "!", "!=", "==", ">", "<", ">=", "<=", "*=", "/=", "+=", "-=", "=>", "<=>" };

    size_t id = 0;
    for (auto _ : state)
    {
        language->parse_token(operations.at(id++ % operations.size()));
    }
}

BENCHMARK_DEFINE_F(NodlangFixture, parse_token__a_single_char)(benchmark::State& state) {
    std::array chars{ ",", " ", "\n", "\t", ";", "{", "}", "(", ")" };

    size_t id = 0;
    for (auto _ : state)
    {
        language->parse_token(chars.at(id++ % chars.size()));
    }
}

BENCHMARK_DEFINE_F(NodlangFixture, tokenize__some_code_to_graph)(benchmark::State& state) {
    std::string code = "double a = 10.400012;"
                       "double b = 5.564478;"
                       "if(a>b){"
                       " print(\"a>b\");"
                       "}else{"
                       " print(\"a<=b\");"
                       "}"
                       ;

    for (auto _ : state)
    {
        VERIFY(language->tokenize(code), "parse failed");
    }
}

BENCHMARK_DEFINE_F(NodlangFixture, parse_token__a_single_keyword)(benchmark::State& state) {

    std::array chars{ "if", "else", "for", "operator", "int", "bool", "double", "string" };

    size_t id = 0;
    for (auto _ : state)
    {
        language->parse_token(chars.at(id++ % chars.size()));
    }
}

BENCHMARK_DEFINE_F(NodlangFixture, parse_token__a_single_identifier_starting_with_a_keyword)(benchmark::State& state) {

    std::array chars{ "iff", "elsee", "forr", "operatorr", "intt", "booll", "doublee", "stringg"};

    size_t id = 0;
    for (auto _ : state)
    {
        language->parse_token(chars.at(id++ % chars.size()));
    }
}

BENCHMARK_REGISTER_F(NodlangFixture, tokenize__some_code_to_graph);
BENCHMARK_REGISTER_F(NodlangFixture, parse_token__a_single_operator);
BENCHMARK_REGISTER_F(NodlangFixture, parse_token__a_single_boolean);
BENCHMARK_REGISTER_F(NodlangFixture, parse_token__a_single_double);
BENCHMARK_REGISTER_F(NodlangFixture, parse_token__a_single_char);
BENCHMARK_REGISTER_F(NodlangFixture, parse_token__a_single_keyword);
BENCHMARK_REGISTER_F(NodlangFixture, parse_token__a_single_identifier_starting_with_a_keyword);

BENCHMARK_MAIN();