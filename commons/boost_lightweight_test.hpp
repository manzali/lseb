#ifndef BOOST_CORE_LIGHTWEIGHT_TEST_HPP
#define BOOST_CORE_LIGHTWEIGHT_TEST_HPP

// MS compatible compilers support #pragma once

#if defined(_MSC_VER)
# pragma once
#endif

# include <assert.h> // .h to support old libraries w/o <cassert> - effect is the same

#define BOOST_ASSERT(expr) assert(expr)

#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)

#define BOOST_CURRENT_FUNCTION __PRETTY_FUNCTION__

#elif defined(__DMC__) && (__DMC__ >= 0x810)

#define BOOST_CURRENT_FUNCTION __PRETTY_FUNCTION__

#elif defined(__FUNCSIG__)

#define BOOST_CURRENT_FUNCTION __FUNCSIG__

#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))

#define BOOST_CURRENT_FUNCTION __FUNCTION__

#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)

#define BOOST_CURRENT_FUNCTION __FUNC__

#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)

#define BOOST_CURRENT_FUNCTION __func__

#else

#define BOOST_CURRENT_FUNCTION "(unknown)"

#endif

#include <iostream>

//  IDE's like Visual Studio perform better if output goes to std::cout or
//  some other stream, so allow user to configure output stream:
#ifndef BOOST_LIGHTWEIGHT_TEST_OSTREAM
#define BOOST_LIGHTWEIGHT_TEST_OSTREAM std::cerr
#endif

namespace boost
{

namespace detail
{

struct report_errors_reminder
{
    bool called_report_errors_function;

    report_errors_reminder() : called_report_errors_function(false) {}

    ~report_errors_reminder()
    {
        BOOST_ASSERT(called_report_errors_function);  // verify report_errors() was called  
    }
};

inline report_errors_reminder& report_errors_remind()
{
    static report_errors_reminder r;
    return r;
}

inline int & test_errors()
{
    static int x = 0;
    report_errors_remind();
    return x;
}

inline void test_failed_impl(char const * expr, char const * file, int line, char const * function)
{
    BOOST_LIGHTWEIGHT_TEST_OSTREAM
      << file << "(" << line << "): test '" << expr << "' failed in function '"
      << function << "'" << std::endl;
    ++test_errors();
}

inline void error_impl(char const * msg, char const * file, int line, char const * function)
{
    BOOST_LIGHTWEIGHT_TEST_OSTREAM
      << file << "(" << line << "): " << msg << " in function '"
      << function << "'" << std::endl;
    ++test_errors();
}

inline void throw_failed_impl(char const * excep, char const * file, int line, char const * function)
{
   BOOST_LIGHTWEIGHT_TEST_OSTREAM
    << file << "(" << line << "): Exception '" << excep << "' not thrown in function '"
    << function << "'" << std::endl;
   ++test_errors();
}

template<class T, class U> inline void test_eq_impl( char const * expr1, char const * expr2,
  char const * file, int line, char const * function, T const & t, U const & u )
{
    if( t == u )
    {
        report_errors_remind();
    }
    else
    {
        BOOST_LIGHTWEIGHT_TEST_OSTREAM
            << file << "(" << line << "): test '" << expr1 << " == " << expr2
            << "' failed in function '" << function << "': "
            << "'" << t << "' != '" << u << "'" << std::endl;
        ++test_errors();
    }
}

template<class T, class U> inline void test_ne_impl( char const * expr1, char const * expr2,
  char const * file, int line, char const * function, T const & t, U const & u )
{
    if( t != u )
    {
        report_errors_remind();
    }
    else
    {
        BOOST_LIGHTWEIGHT_TEST_OSTREAM
            << file << "(" << line << "): test '" << expr1 << " != " << expr2
            << "' failed in function '" << function << "': "
            << "'" << t << "' == '" << u << "'" << std::endl;
        ++test_errors();
    }
}

} // namespace detail

inline int report_errors()
{
    detail::report_errors_remind().called_report_errors_function = true;

    int errors = detail::test_errors();

    if( errors == 0 )
    {
        BOOST_LIGHTWEIGHT_TEST_OSTREAM
          << "No errors detected." << std::endl;
        return 0;
    }
    else
    {
        BOOST_LIGHTWEIGHT_TEST_OSTREAM
          << errors << " error" << (errors == 1? "": "s") << " detected." << std::endl;
        return 1;
    }
}

} // namespace boost

#define BOOST_TEST(expr) ((expr)? (void)0: ::boost::detail::test_failed_impl(#expr, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION))

#define BOOST_ERROR(msg) ( ::boost::detail::error_impl(msg, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION) )

#define BOOST_TEST_EQ(expr1,expr2) ( ::boost::detail::test_eq_impl(#expr1, #expr2, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION, expr1, expr2) )
#define BOOST_TEST_NE(expr1,expr2) ( ::boost::detail::test_ne_impl(#expr1, #expr2, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION, expr1, expr2) )

#define BOOST_TEST_THROWS( EXPR, EXCEP )                    \
   try {                                                    \
      EXPR;                                                 \
      ::boost::detail::throw_failed_impl                    \
      (#EXCEP, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION); \
   }                                                        \
   catch(EXCEP const&) {                                    \
   }                                                        \
   catch(...) {                                             \
      ::boost::detail::throw_failed_impl                    \
      (#EXCEP, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION); \
   }                                                        \


#endif // #ifndef BOOST_CORE_LIGHTWEIGHT_TEST_HPP
