/* FRUCTOSE C++ unit test library. 
Copyright (c) 2014 Andrew Peter Marlow. All rights reserved.
http://www.andrewpetermarlow.co.uk.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef INCLUDED_FRUCTOSE_TEST_BASE
#define INCLUDED_FRUCTOSE_TEST_BASE

#include "fructose/test_root.h"

#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <sstream>

/*! \mainpage FRUCTOSE - FRamework for Unit testing C++ for Test driven development Of SoftwarE
 *
 * \section intro_sec Introduction
 *
 * FRUCTOSE provides a framework for quickly developing test harnesses for
 * use in C++ unit tests. These harnesses are assumed to be part of using
 * TDD (Test Driven Development). A given harness will run all the tests
 * by default and silently (unless there are errors) but the harness can
 * also be used interactively, for selected tests, and with optional
 * verbose output.
 *
 * \section features Features
 *
 * - Assertions.
 * Unlike some other unit test frameworks, FRUCTOSE does not use the
 * C macro facility 'assert' for its assertions.
 * This means that the harness can run all its tests through to completion
 * instead of core dumping at the first failure.
 *
 * - Loop assertions.
 * Macros are provided which help the developer track down the
 * reason for assertion failures for data held in static tables.
 * What is needed in these cases in addition to the file and line
 * number of the assertion is the line number of the data that
 * was tested in the assert.
 *
 * - Exception assertions.
 * The test harness may assert that a condition should result in
 * the throwing of an exception of a specified type. If it does not
 * then the assertion fails. Similarly, a harness may assert that
 * no exception is to be thrown upon the evaluation of a condition;
 * if one is then the assertion fails.
 *
 * - floating point compare assertions.
 * Floating point comparisons can be asserted for using relative
 * or absolute tolerances.
 *
 * - Fine control over test selection.
 * The tests are named in the harness and may be selected by name
 * from the command line. By default all tests are run.
 *
 * - Each test can receive its own command line parameters.
 * Each test can obtain any test-specific parameters that were
 * passed using the command line.
 *
 * - Simple test harnesses.
 * The harness just defines one class, where each public function
 * is designed to be one test case.
 * The harness class inherits from fructose::test_base, which
 * provides it with three functions:
 * -# add_test to add a named test
 * -# run with no arguments runs all tests
 * -# run with argc and argv runs the tests specified on the command line.
 *
 * Note: for compatibility with older versions of fructose,
 * two additional routines are provided:
 * -# get_suite returns a list of tests to run based on parsing the command line
 * -# run runs the tests returned by get_suite.
 *
 * - verbose command line argument option.
 * The functions in the test harness class have access to the
 * function verbose(), which returns true if the -v flag was given
 * on the command line. This is a debugging aid during TDD whereby the
 * harness can print useful intermediate values that might shed light
 * on why a test is failing.
 *
 * \section why Why another unit test framework?
 *
 * \subsection cppunit CppUnit woes
 *
 * This framework was arrived after several weeks of struggling
 * with CppUnit. At the time CppUnit was not even buildable on
 * the main platform being used (Solaris). It has subsequently been
 * ported but this was just one obstacle among many.
 * CppUnit was judged to be too heavy duty for the needs of most
 * simple unit test harnesses so a more light duty library was developed.
 * This one has just two classes, test_base and test_root, and a
 * handful of macros for convenient assertion testing.
 *
 * \subsection depends Unit test framework dependencies
 *
 * Some other unit test frameworks rely on other components that are
 * not very lightweight and not always very portable.
 * FRUCTOSE also has tried to avoid dependencies on other external
 * libraries. It is completely standalone.
 * It is all done with inlined templates and so does not require 
 * a library to be linked in.
 *
 * \subsection simplicity Test harnesses must be simple
 *
 * Other frameworks tend to require alot of the test harnesses.
 * There are sometimes many classes to write and several files.
 * The objective with FRUCTOSE was to have a class that is comprised
 * of just 3 files; the header, the implementation and the test harness.
 * A FRUCTOSE test harness requires just one class to be defined.
 * Each public function of that class is designed to be a test case.
 *
 * \section not What FRUCTOSE does not do
 *
 * FRUCTOSE does not attempt to provide output tailored for any
 * particular reporting mechanism, it just writes any errors
 * (along with any verbose output) to std::cout. It is this not
 * designed to directly support web-based unit test report 
 * summaries, unlike CppUnit.
 *
 * \section misc Miscellaneous notes
 *
 * FRUCTOSE always writes its output on std::cout. It does not ever write
 * to std::cerr unless it is invoked wrongly, e.g with a named test where the
 * test name is unknown. The rational behind this is that the harness
 * works no matter how many tests pass or fail. This is particularly
 * useful when the verbose flag is enabled since verbose output is expected
 * to also go to std::cout. This avoids problems with std::cout and std::cerr
 * being out of sync (std::cout is buffered by std::cerr is not).
 *
 * If one or more of the tests fail then the exit status is set to
 * EXIT_FAILURE. This is so that any reporting tools built around the
 * invocation of these test harnesses can easily determine whether any
 * harnesses produced test failures.
 */
namespace fructose {

/**
 * Base class for test container classes
 * 
 * The test container class, i.e., the one which contains the test cases
 * as members, must derive from test_base.
 * 
 * Synopsis:
 * 
 * @code
 *     #include "fructose/fructose.h"
 * 
 *     [...]
 * 
 *     class my_test : public test_base<my_test>
 *     {
 *     public:
 *         my_test(int val1, int val2);
 *         void test_equality(const std::string& test_name);
 *            void test_foobar(const std::string& test_name);
 *         [...]
 *     };
 * 
 *     [...]
 * 
 *     void my_test::test_equality(const std::string& the_name)
 *     {
 *         fructose_assert(my_test(1,2) == my_test(1,2));
 *     }
 * 
 *     [...]
 * 
 *     void my_test::test_foobar(const std::string& test_name)
 *     {
 *         fructose_assert_exception(my_test(1,4).foobar(), MathException);
 *     }
 * 
 *     int main(int argc, char* argv[])
 *     {
 *         my_test tests;
 * 
 *         tests.add_test("testEquality", &my_test::test_equality);
 *         tests.add_test("testAddition", &my_test::test_foobar);
 *     
 *         [...]
 * 
 *         return tests.run(argc, argv); 
 *     }
 * 
 * @endcode
 * 
 * There should be a test driver for each library module (*.cpp file with 
 * associated header), structured like the example above. Test cases are
 * represented as member functions of an ad-hoc class ("test container class")
 * which must derive from the template test_base with itself as the template 
 * argument. (This is the Curiously Recurring Template Pattern CRTP.) 
 * 
 * In writing the test code, the test assert macros defined below come
 * in handy. 
 *   - fructose_assert(condition)
 *   - fructose_loop1_assert(line, 1stLoopCounter, condition)
 *   - fructose_loop2_assert(line, 1stLoopCounter, 2ndLoopCounter, condition)
 *  
 * Test drivers accept a set of standard command line arguments which
 * are displayed if the -h option is set: 
 * 
 * @code
 *
 * USAGE: 
 *
 *    ./example [-h] [-r] [-a] [-v] [--] <testNameString> ...
 *
 * Where: 
 *
 * -h,  --help           produces this help
 *
 * -r,  --reverse        reverses the sense of test assertions.

 *  -a,  --assert_fatal   is used to make the first test failure fatal.
 *
 *  -v,  --verbose        turns on extra trace for those tests that have made use of it.
 *
 *  --,  --ignore_rest    Ignores the rest of the labeled arguments following this flag.
 *
 *
 *   <testNameString>  (accepted multiple times)
 *    test names
 *
 *   Any number of test names may be supplied on the command line. If the
 *   name 'all' is included then all tests will be run.
 *
 *  
 *  Supported test names are:
 *      test_equality
 *      test_foobar
 *
 * @endcode
 * 
 * Notice the list of configured test cases displayed after the command line
 * options. Test cases can be named explicitly when running the driver, which
 * restricts execution to the named test cases.
 * 
 * @code
 *     $ testdriver -v test_foobar
 *     running test_foobar
 * @endcode
 * 
 * This is useful when trying to focus on one test case. 
 */
template<typename test_container>
class test_base : public test_root
{
public:    

    /** 
     * Type of test case functions
     * 
     * Test cases are implemented as member functions of the test container 
     * class, which must be derived from this template (CRTP).
     * These methods return void and accept one parameter: the 
     * name of the test case.
     */
    typedef void (test_container::*test_case)(const std::string&);
    
    // compiler-generated default constructor would be OK
    // but gives warnings with GCC's -Weffc++.

    test_base<test_container>()
      : m_tests()
      , m_available_tests(suite())
      , m_exceptionPending("")
      {
      }

    /**
     * Register a test case
     * 
     * Register a member function implementing a test case against the 
     * name of the test case
     * 
     * @param name - The name of the test case
     * @param the_test - The member function implementing the test case
     */
    void add_test(const std::string& name, test_case the_test);
    
    /**
     * Run statically configured tests
     * 
     * This function runs all the registered tests in the sequence in 
     * which they were registered.
     * 
     * @return - An integer suitable as main exit status: EXIT_SUCCESS
     * if the the tests were successful, EXIT_FAILURE otherwise.
     */
    int run();

    /**
     * Run the test specified on the command line
     * (runs all tests if none specified).
     * Flags are set for various command line options
     * such as the verbose flag and first assertion is fatal.
     */
    int run(int argc, char* argv[]);

    /**
     * Run tests configured in test suite
     * 
     * This function runs the tests named in the test suite in the sequence
     * by the suite. If the suite contains a name which is not registered, 
     * a warning message is issued. 
     * 
     * @param suite - The test suite
     * 
     * @return - An integer suitable as main exit status: EXIT_SUCCESS
     * if the the tests were successful, EXIT_FAILURE otherwise.
     */
    int run(const suite& suite);
    
    /**    
     * Return a list of test names from the command line.
     * 
     * This function returns a suite containing all the test cases named on 
     * the command line. If none are named, or if the keyword "all" is passed,
     * all the configured test cases are included.
     * 
     * Note: This routine is provided for compatibility with older
     * versions of fructose. New programs should use the function
     * run(argc, argv).
     * 
     * @param argc - The argument count passed to main
     * @param argv - The argument vector passed to main
     * 
     * @return The configured test suite 
     */
    suite get_suite(int argc, char* argv[]);
    
private:    

    /**
     * Helper routine to provide the main functionality of run but
     * with added exception handling so the test harness does not have
     * to do it.
     */
    int do_run(const suite& suite);

    /**
     * Collection of test cases, keyed by their names
     */
    std::map<std::string, std::pair<test_case, test_info> > m_tests;

    /*
     * Defines the sequence in which tests have to be run.
     */
    suite m_available_tests;

    /*
     * If the fructose machinery itself has an error, then
     * it is stored as a pending exception in this string.
     * This enables the string to be checked when run is called.
     */
    std::string m_exceptionPending;
};

// ====================
// INLINE definitions
// ====================

template<typename test_container>
inline void 
test_base<test_container>::add_test(const std::string& name, 
                                    test_case the_test)
{
    if (m_exceptionPending.length() > 0)
    {
        return;
    }

    typename std::map<std::string, std::pair<test_case, test_info> >::const_iterator it = m_tests.find(name);
    if (it == m_tests.end())
    {
        m_tests[name] = std::make_pair(the_test, test_info(name));
        m_available_tests.push_back(name);
    }
    else
    {
        std::stringstream str;
        str << "add_test called with test name '" << name
            << "' which has already been added.";
        m_exceptionPending = str.str();
    }
}
    
template<typename test_container>
inline int 
test_base<test_container>::run()
{
    return run(m_available_tests);    
}

template<typename test_container>
inline int 
test_base<test_container>::run(int argc, char* argv[])
{
    int exitStatus = EXIT_SUCCESS;

    if (m_exceptionPending.length() > 0)
    {
        std::cout << "ERROR in use of FRUCTOSE: " << m_exceptionPending << std::endl;
        exitStatus = EXIT_FAILURE;
    }
    else
    {
        test_root::suite the_suite = get_suite(argc, argv);
        exitStatus = run(the_suite);
    }

    return exitStatus; 
}

template<typename test_container>
inline int 
test_base<test_container>::run(const test_root::suite& suite) 
{
    try
    {
        return do_run(suite);
    }
    catch(...)
    {
        return fructose::test_root::exception_handler();
    }
}

template<typename test_container>
inline int 
test_base<test_container>::do_run(const suite& suite) 
{
    test_container* runner = dynamic_cast<test_container*>(this);
    
    if (runner == 0)
    {
        throw std::runtime_error(
            "problem in test set-up; probable cause: "
            "test container class not passed to test_base template");
    }

    for (typename suite::const_iterator it = suite.begin(); it != suite.end(); ++it)
    {
        std::pair<test_case, test_info> value = m_tests[it->m_test_name];
        test_case test_case = value.first;
        if (test_case)
        {
            const std::string title = "Running test case " + it->m_test_name;
            if (verbose())
            {
                std::cout << std::endl << title << std::endl
                          << underline(title) << '\n' << std::endl;
            }
            runner->setup();
            runner->set_test_info(&(*it));
            try
            {
                (runner->*test_case)(it->m_test_name);    
                runner->teardown();
            }
            catch(std::exception& ex)
            {
                runner->teardown();
                set_exception_happened();
		std::cout << ex.what() << std::endl;
            }
        }
        else
        {
            std::cerr << "No such test case: " << it->m_test_name << std::endl;
        }
    }
    
    return return_status();
}

template<typename test_container>
inline 
test_root::suite test_base<test_container>::get_suite(int argc, char* argv[])
{
    return do_get_suite(m_available_tests, argc, argv);    
}

} // namespace

// Macros for test harness code generator.

#define FRUCTOSE_STRUCT(name) struct name : public fructose::test_base<name>
#define FRUCTOSE_CLASS(name) class name : public fructose::test_base<name>
#define FRUCTOSE_TEST(name) void name(const std::string& test_name)

#endif
