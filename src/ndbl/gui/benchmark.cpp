#include <benchmark/benchmark.h>
#include <random>
#include "ndbl/core/language/Nodlang.h"
#include "ndbl/core/Graph.h"
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
    Nodlang*     language;
    bool         autocompletion;
    Graph*       graph;
    std::random_device random_device;  // Will be used to obtain a seed for the random number engine
    std::mt19937       generator; // Standard mersenne_twister_engine
    std::uniform_real_distribution<double> distribution;

    NodlangFixture()
        : generator(random_device())
        , distribution(-100000.0, 100000.0)
    {}

    void SetUp(const ::benchmark::State& state)
    {
        language       = &ndbl::Nodlang::get_instance();
        autocompletion = false;
        graph          = new Graph(factory);
        log_set_verbosity(Verbosity_Error);
    }

    void TearDown(const ::benchmark::State& state)
    {
        delete graph;
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

template<class StringT>
static void BM_empty_constructor(benchmark::State& state)
{
    for (auto _ : state)
    {
        StringT str;
        benchmark::DoNotOptimize(str);
    }
}

const char* SIXTY_THREE_CHARS = "|<------------------------ 63 chars ------------------------->|";

template<class StringT>
static void BM_constructor(benchmark::State& state)
{
    char* buf = new char[state.range(0)];
    memset(buf, 'x', state.range(0));
    buf[state.range(0)-1] = 0;
    for (auto _ : state)
    {
        StringT str{buf};
    }
    delete[] buf;
}

template<class StringT>
static void BM_constructor_then_push_back_a_char(benchmark::State& state)
{
    char* buf = new char[state.range(0)];
    memset(buf, 'x', state.range(0));
    buf[state.range(0)-1] = 0;
    for (auto _ : state)
    {
        StringT str{buf};
        str.push_back('+');
    }
    delete[] buf;
}

template<size_t STRING_SIZE>
static void BM_strncpy_stack_to_stack(benchmark::State& state)
{
    char source[STRING_SIZE];
    while (state.KeepRunning())
    {
        char destination[STRING_SIZE];
        strncpy(destination, source, STRING_SIZE);
        benchmark::DoNotOptimize(destination);
    }
}

template<size_t STRING_SIZE>
static void BM_strncpy_heap_to_stack(benchmark::State& state)
{
    char* source = new char[STRING_SIZE];
    while (state.KeepRunning())
    {
        char destination[STRING_SIZE];
        strncpy(destination, source, STRING_SIZE);
        benchmark::DoNotOptimize(destination);
    }
    delete[] source;
}

template<size_t STRING_SIZE>
static void BM_strncpy_heap_to_heap(benchmark::State& state)
{
    char* source = new char[STRING_SIZE];
    char* destination = new char[STRING_SIZE];
    while (state.KeepRunning())
    {
        strncpy(destination, source, STRING_SIZE);
        benchmark::DoNotOptimize(destination);
    }
    delete[] source;
    delete[] destination;
}

BENCHMARK_DEFINE_F(NodlangFixture, parse_token__a_single_operator)(benchmark::State& state) {
    std::array operations{ "+", "-", "!", "!=", "==", ">", "<", ">=", "<=", "*=", "/=", "+=", "-=", "=>", "<=>" };

    size_t id = 0;
    for (auto _ : state)
    {
        language->parse_token( operations.at(id++ % operations.size()) );
    }
}

BENCHMARK_DEFINE_F(NodlangFixture, parse_token__a_single_char)(benchmark::State& state) {
    std::array chars{ ",", " ", "\n", "\t", ";", "{", "}", "(", ")" };

    size_t id = 0;
    for (auto _ : state)
    {
        language->parse_token( chars.at(id++ % chars.size() ));
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
        VERIFY(language->Language_tokenize(code), "parse failed");
    }
}

BENCHMARK_DEFINE_F(NodlangFixture, parse_token__a_single_keyword)(benchmark::State& state) {

    std::array chars{ "if", "else", "for", "operator", "int", "bool", "double", "string" };

    size_t id = 0;
    for (auto _ : state)
    {
        language->parse_token( chars.at(id++ % chars.size() ));
    }
}

BENCHMARK_DEFINE_F(NodlangFixture, parse_token__a_single_identifier_starting_with_a_keyword)(benchmark::State& state) {

    std::array chars{ "iff", "elsee", "forr", "operatorr", "intt", "booll", "doublee", "stringg"};

    size_t id = 0;
    for (auto _ : state)
    {
        language->parse_token( chars.at(id++ % chars.size() ));
    }
}

BENCHMARK_REGISTER_F(NodlangFixture, tokenize__some_code_to_graph);
BENCHMARK_REGISTER_F(NodlangFixture, parse_token__a_single_operator);
BENCHMARK_REGISTER_F(NodlangFixture, parse_token__a_single_boolean);
BENCHMARK_REGISTER_F(NodlangFixture, parse_token__a_single_double);
BENCHMARK_REGISTER_F(NodlangFixture, parse_token__a_single_char);
BENCHMARK_REGISTER_F(NodlangFixture, parse_token__a_single_keyword);
BENCHMARK_REGISTER_F(NodlangFixture, parse_token__a_single_identifier_starting_with_a_keyword);
//
//BENCHMARK(BM_empty_constructor<std::string>);
//BENCHMARK(BM_empty_constructor<string>);
//BENCHMARK(BM_empty_constructor<string64>);
//BENCHMARK(BM_empty_constructor<string128>);
//
//BENCHMARK(BM_constructor<std::string>)->Range(1, 16);
//BENCHMARK(BM_constructor<string>)->Range(1, 16);
//BENCHMARK(BM_constructor<string8>)->Range(1, 16);
//BENCHMARK(BM_constructor<string16>)->Range(1, 16);
//BENCHMARK(BM_constructor<string32>)->Range(1, 16);
//
//BENCHMARK(BM_constructor_then_append_a_char<std::string>);
//BENCHMARK(BM_constructor_then_append_a_char<string>);
//BENCHMARK(BM_constructor_then_append_a_char<string64>);
//BENCHMARK(BM_constructor_then_append_a_char<string128>);
//
//
//#define BENCHMARK_STRNCPY(BM_strncpy_, size) \
//BENCHMARK(BM_strncpy_<size/8>); \
//BENCHMARK(BM_strncpy_<size/4>); \
//BENCHMARK(BM_strncpy_<size/2>); \
//BENCHMARK(BM_strncpy_<size>); \
//BENCHMARK(BM_strncpy_<size*2>); \
//BENCHMARK(BM_strncpy_<size*4>); \
//BENCHMARK(BM_strncpy_<size*8>); \
//BENCHMARK(BM_strncpy_<size*16>); \
//BENCHMARK(BM_strncpy_<size*32>);
//
//constexpr size_t _32KiB = 32 * 1024;
//BENCHMARK_STRNCPY(BM_strncpy_stack_to_stack, _32KiB)
//BENCHMARK_STRNCPY(BM_strncpy_heap_to_stack, _32KiB)
//BENCHMARK_STRNCPY(BM_strncpy_heap_to_heap, _32KiB)

BENCHMARK_MAIN();