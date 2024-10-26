#pragma once

#include <iostream>

#define REPORT_LOCATION "\n\t(" <<__FILE__ << ":" << __LINE__  << ")"


#define CHECK(x) do { \
    if(!(x)) {  \
        std::cerr << "FAILED: " << #x << REPORT_LOCATION << std::endl; \
        exit(1);\
    } else {\
        std::cout << "Passed: " << #x <<  std::endl;\
    }\
}while(false)

#define CHECK_PRINT(x,v) do { \
    if(!(x)) {  \
        std::cerr << "FAILED: " << #x << ": " << (v)<< REPORT_LOCATION << std::endl; \
        exit(1);\
    } else {\
        std::cout << "Passed: " << #x << ": " << (v) <<  std::endl;\
    }\
}while(false)

#define CHECK_BINARY_OP(a,op,b) do { \
    if((a) op (b)) {  \
        std::cout << "Passed: " << #a << #op << #b << ": " << (a) << #op << (b) << std::endl;\
    } else {\
        std::cerr << "FAILED: " << #a << #op << #b << ": "<< (a) << #op << (b) << REPORT_LOCATION << std::endl; \
        exit(1);\
    } \
}while(false)


#define CHECK_EQUAL(a,b) CHECK_BINARY_OP(a,==,b)
#define CHECK_NOT_EQUAL(a,b) CHECK_BINARY_OP(a,!=,b)
#define CHECK_LESS(a,b) CHECK_BINARY_OP(a,<,b)
#define CHECK_GREATER(a,b) CHECK_BINARY_OP(a,>,b)
#define CHECK_LESS_EQUAL(a,b) CHECK_BINARY_OP(a,<=,b)
#define CHECK_GREATER_EQUAL(a,b) CHECK_BINARY_OP(a,>=,b)
#define CHECK_BETWEEN(a,b,c) do {CHECK_BINARY_OP(a,<=,b); CHECK_BINARY_OP(b,<=,c);} while(false)


#define CHECK_EXCEPTION(type, ... ) \
    try { \
        __VA_ARGS__; \
        std::cerr << "FAILED: throw " << #type << REPORT_LOCATION << std::endl; \
        exit(1);\
    } catch (const type &)  { \
        std::cout << "Passed: throw " << #type << std::endl; \
    }

#define CHECK_EXCEPTION_EXPR(type, var, test_expr, ... ) \
    try { \
        __VA_ARGS__; \
        std::cerr << "FAILED: throw " << #type << REPORT_LOCATION << std::endl; \
        exit(1);\
    } catch (const type &var)  { \
        if (test_expr) { \
            std::cout << "Passed: throw " << #type << std::endl; \
        } else { \
            std::cerr << "FAILED: throw expression failed " << #type << ": " << #test_expr << REPORT_LOCATION << std::endl; \
            exit(1);\
        }\
    }
