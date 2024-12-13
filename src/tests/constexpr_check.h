#include <utility>
#define CONSTEXPR_TESTABLE constexpr


///overwrite to defines OK result of the test
template<auto x> constexpr auto  assert_failed = false;

///evaluates the test
/**
 * @tparam T type of test
 * @tparam result result of test
 * @return true
 *
 * @note side effect of this function - if test failed, it emits name of the test
 * and result of the test if it is intergral value
 */
template<typename T, auto result>
constexpr bool evaluate_test() {
    static_assert(assert_failed<result>, "test failed");
    return true;
}

///helper class
template<template<int> class Test, typename Idx>
struct RunTests;

///helper class
template<template<int> class Test, int ...indexes>
struct RunTests<Test, std::integer_sequence<int, indexes... > > {
    static constexpr auto result = (true
        && ... && evaluate_test<Test<indexes>,(Test<indexes>()()) >());
};

///runs test defined as TestCase<N> where N is index of the test (index to test data)
/**
 * @tparam Test reference to TestCase<N> template.
 * @tparam count count of tests. It runs test from TestCase<0>, TestCase<1> ... TestCase<N-1>
 *
 * The value is evaluated to true. In case test failed, this line is not compiled
 * You can use static_assert(run_tests"...") to run all tests
 */
template<template<int> class Test, int count>
constexpr bool run_tests = RunTests<Test, std::make_integer_sequence<int, count> >::result;

///Counts of items in an array
template<typename T, int N>
constexpr int countof(T (&)[N]) {return N;}

