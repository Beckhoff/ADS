/* FRUCTOSE C++ unit test library. 
Copyright (c) 2012 Andrew Peter Marlow. All rights reserved.
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

#ifndef INCLUDED_FRUCTOSE_DOUBLECOMPARE
#define INCLUDED_FRUCTOSE_DOUBLECOMPARE

namespace fructose {

/**
 * This namespace contains functions that implement "fuzzy" 
 * equality and relational operations as a set of function calls
 * on a pair of 'double' values 'a' and 'b'. The term "fuzzy equality"
 * expresses the concept that 'a' and 'b'  are "close enough"; any 
 * difference that exists between the internal representations of 'a' and
 * 'b' is not significant, and should be neglected for comparison purposes.
 *
 * Fuzzy comparison operations are of general use in computing but there
 * is nothing in the C++ standard (yet) to address this. So each project
 * that uses floating point numbers tends to define its own.
 * The one here is general-purpose but has been encased in the fructose
 * namespace since it is used by fructose to provide unit test assertions
 * for floating point numbers. The reader will notice that the functions
 * are general enough to have wider applicability but they are (at the
 * moment, in a namespace that is part of a unit test library).
 * This may change in some later version of fructose, depending on any
 * feedback from the user community. There are other ways to get these
 * sorts of comparison functions (pending the issue being addressed
 * by the standard and then implemented by compiler writers).
 * Some widely-used Open Source libraries offer similar functions.
 * For example, there are algorithms in boost such as close_at_tolerance
 * and check_is_close. These functions exist here in order to avoid
 * a dependency on boost.
 *
 * There are two kinds of fuzzy comparison: comparison using fixed 
 * (aka absolute) tolerance value and comparison using relative tolerance
 * value. Absolute tolerances are used when and b are expected to be quite
 * close so an absolute value can be used to compare the distance between them.
 * This technique becomes inappropriate when the values are large,
 * e.g one million and one million and two are very close because they
 * are within 0.002% of each other. To handle this case, relative tolerance
 * is used. The relative difference between a and b is calculated as
 * fabs(a-b)/average(a,b). If this is less than or equal to a relative 
 * tolerance value then a and b compare as equal. The relative tolerance
 * value is thus similar to the absolute tolerance value in that it does not
 * result in a comparion to within a percentage of a or b, but
 * it does take their relative magnitudes into account.
 * See "The Art of Computer Programming, volume 2" by Knuth,
 * for a more detailed discussion on comparing floating point numbers.
 *
 * Thus, using the algorithms in this namespace, 'a' and 'b' have 
 * fuzzy equality if either their relative difference is less than or
 * equal to some user-specified or default tolerance value, or else
 * their absolute difference is less than or equal to a separate
 * user-specified or default tolerance value. The fuzzy inequality
 * and relational operators are all defined with respect to fuzzy equality.
 * 
 * The table below shows how the fuzzy comparison operations map onto
 * the function provided by this namespace:
 * \code
 *                  N  O  T  A  T  I  O  N      O  N  L  Y
 *      -------------------------------------------------------------
 *       C++ operator | fuzzy operator | namespace function name
 *      -------------------------------------------------------------
 *            ==      |  ~eq           |   eq(a, b, abstol, reltol)
 *            !=      |  ~ne           |   ne(a, b, absrol, reltol)
 *            <       |  ~lt           |   lt(a, b, abstol, reltol)
 *            <=      |  ~le           |   le(a, b, abstol, reltol)
 *            >=      |  ~ge           |   ge(a, b, abstol, reltol)
 *            >       |  ~gt           |   gt(a, b, abstol, reltol)
 *      -------------------------------------------------------------
 * \endcode
 *
 * Using the Backus-Naur notation "A := B" to mean "A is defined as B",
 * we define the comparision algorithms as follows:
 *
 * \code
 *   a ~eq b := ( (a == b) ||
 *              ( ( fabs(a - b) <= absolute_tolerance ) ||
 *              ( fabs(a - b) / ( fabs(a + b) / 2 ) <= relative_tolerance ) ) )
 *   a ~ne b :=  !(a ~eq b)
 *   a ~lt b :=  !(a ~eq b) && (a < b)
 *   a ~le b :=   (a ~eq b) || (a < b)
 *   a ~ge b :=   (a ~eq b) || (a > b)
 *   a ~gt b :=  !(a ~eq b) && (a > b)
 * \endcode
 * 
 * where fabs(double) is the standard absolute value function and
 * other symbols have their usual C++ meaning.  Note that the equality
 * operations are symmetric.
 * 
 * The functions implemented in this class are well behaved for all
 * values of 'relative_tolerance' and 'absolute_tolerance'. If either
 * relative_tolerance <= 0 or absolute_tolerance <= 0, that respective
 * "fuzzy comparison" is suppressed.  If both relative_tolerance <= 0
 * and absolute_tolerance <= 0, the six fuzzy operations behave as
 * runtime-intensive versions of their non-fuzzy counterparts.
 * 
 * Note that the definition of fuzzy equality used in this class has
 * one intermediate singularity.  When (fabs(a - b) > absolute_tolerance
 * && (a == -b)) is true, the pseudo-expression (a ~eq b) above has a
 * zero denominator. In this case, the test for relative fuzzy equality
 * is suppressed.  Note also that this intermediate singularity does NOT
 * lead to a special case behavior of fuzzy comparisons.  By definition,
 * the relative difference is the quotient of the absolute difference and
 * the absolute average, so the case (a == -b) truly represents an "infinite
 * relative difference", and thus fuzzy equality via the relative difference
 * should be false (although absolute fuzzy equality may still prevail).
 * 
 * \attention
 * The functions in this namespace are implemented using the definitions
 * provided, and as such are vulnerable to the limitations of the internal
 * representations of 'double'.  In particular, if (a + b) or (a - b) cannot
 * be represented, the functions will fail outright. As 'a' or 'b' approach
 * the limits of precision of representation -- that "approach" being defined
 * by the fuzzy tolerances -- the algorithms used in this namespace become
 * increasingly unreliable.  The user is responsible for determining the
 * limits of applicability of this namespace to a given calculation, and for
 * coding accordingly.
 * 
 * The following example tabulates the numerical results for a set
 * of function calls to the six fuzzy comparison functions of the form
 * \code
 *   double_compare::eq(x, y, relative_tolerance, absolute_tolerance)
 * \endcode
 * where 'relative_tolerance' and 'absolute_tolerance' are the relative
 * and absolute tolerances,respectively.  For convenience, the true relative
 * difference is tabulated (to five decimal places) in the column with the
 * heading 'diff'.
 * \code
 *         x   |   y   |  rel  |  abs  |  diff   | eq  ne  lt  le  ge  gt
 *      ------------------------------------------------------------------
 *        99.0 | 100.0 | 0.010 | 0.001 | 0.01005 |  0   1   1   1   0   0
 *       100.0 |  99.0 | 0.010 | 0.001 | 0.01005 |  0   1   0   0   1   1
 *        99.0 | 100.0 | 0.011 | 0.001 | 0.01005 |  1   0   0   1   1   0
 *        99.0 | 100.0 | 0.010 | 0.990 | 0.01005 |  0   1   1   1   0   0
 *        99.0 | 100.0 | 0.010 | 1.000 | 0.01005 |  1   0   0   1   1   0
 *      ------------------------------------------------------------------
 *       100.0 | 101.0 | 0.009 | 0.001 | 0.00995 |  0   1   1   1   0   0
 *       101.0 | 100.0 | 0.009 | 0.001 | 0.00995 |  0   1   0   0   1   1
 *       100.0 | 101.0 | 0.010 | 0.001 | 0.00995 |  1   0   0   1   1   0
 *       100.0 | 101.0 | 0.009 | 0.990 | 0.00995 |  0   1   1   1   0   0
 *       100.0 | 101.0 | 0.009 | 1.000 | 0.00995 |  1   0   0   1   1   0
 * \endcode
 * 
 * Here are some notes on usage: 
 * Given a 'double' value 'x' and two functions 'f' and 'g' each taking one
 *  'double' and returning 'double':
 *
 * \code
 *       double g(double x);
 * \endcode
 *
 *  we can do something if the two functions, each evaluated at 'x', are not
 *  close enough by our own explicit criteria.
 *
 * \code
 *   if (double_compare::ne(f(x), g(x), 1e-8))
 *   {
 *       doSomething();
 *   }
 * \endcode
 *
 **/
namespace double_compare
{
    /// default relative tolerance
    const double relative_tolerance_default = 1e-12;

    /// default absolute tolerance
    const double absolute_tolerance_default = 1e-24;

    /**
     * @brief Return 0 if the specifed 'a' and 'b' have 
     * fuzzy equality (a ~eq b).
     *
     * Otherwise, return a positive value if a > b, or a negative value
     * if a < b.  This is analogous to the return value of strcmp, which
     * allow easy implementation of all the numerical comparison operations
     * in terms of fuzzy_compare.
     * Fuzzy equality between 'a' and 'b' is defined in terms of the
     * specified 'relative_tolerance' and 'absolute_tolerance' such that
     * the expression
     *\code
     *      (a == b) || fabs(a - b) <= absolute_tolerance ||
     *      (fabs(a - b) / fabs((a + b) / 2.0)) <= relative_tolerance)
     *\endcode
     * evaluates to 'true'.  As a consequence of this definition of
     * relative difference, the special case of (a == -b && a != 0)
     * is treated as the maximum relative difference: no value of
     * 'relative_tolerance' can force fuzzy equality (although an
     * appropriate value of 'absolute_tolerance' can).
     * If relative_tolerance <= 0 or absolute_tolerance <= 0, the
     * respective aspect of fuzzy comparison is suppressed, but the
     * function is still valid.  Note that although this function
     * may be called directly, its primary purpose is to implement
     * the fuzzy quasi-boolean equality and relational functions named
     * 'eq', 'ne', 'lt', 'le', 'ge', and 'gt', corresponding to the
     * operators '==', '!=', '<', '<=', '>=', and '>', respectively.
     **/
    int fuzzy_compare(double a, double b, double relative_tolerance, 
                      double absolute_tolerance);

    /**
     * @brief Return a non-zero value if the specified 'a' and 'b' satisfy
     * the fuzzy equality relation (a ~eq b), and 0 otherwise.
     *
     * Optionally specify the relative tolerance 'relative_tolerance', or
     * 'relative_tolerance' and the absolute tolerance 'absolute_tolerance',
     * used to determine fuzzy equality. If optional parameters are not
     * specified, reasonable implementation-dependent default tolerances
     * are used.  If relative_tolerance <= 0 or absolute_tolerance <= 0, 
     * the respective aspect of fuzzy comparison is suppressed, but the
     * function is still valid.
     **/
    bool eq(double a, double b);
    bool eq(double a, double b, double relative_tolerance);
    bool eq(double a, double b, double relative_tolerance, double absolute_tolerance);

    /**
     * @brief Return a non-zero value if the specified 'a' and 'b' satisfy
     * the fuzzy inequality relation (a ~ne b), and 0 otherwise.
     *
     * Optionally specify the relative tolerance 'relative_tolerance', or
     * 'relative_tolerance' and the absolute tolerance 'absolute_tolerance',
     * used to determine fuzzy equality. If optional parameters are not
     * specified, reasonable implementation-dependent default tolerances
     * are used.  If relative_tolerance <= 0 or absolute_tolerance <= 0,
     * the respective aspect of fuzzy comparison is suppressed, but the
     * function is still valid.
     **/
    bool ne(double a, double b);
    bool ne(double a, double b, double relative_tolerance);
    bool ne(double a, double b, double relative_tolerance, double absolute_tolerance);

    /**
     * @brief Return a non-zero value if the specified 'a' and 'b' satisfy
     * the fuzzy less-than relation (a ~lt b) and 0 otherwise.
     *
     * Optionally specify the relative tolerance 'relative_tolerance',
     * or 'relative_tolerance' and the absolute tolerance
     * 'absolute_tolerance', used to determine fuzzy equality.
     * If optional parameters are not specified, reasonable implementation-
     * dependent default tolerances are used.  If relative_tolerance <= 0 or
     * absolute_tolerance <= 0, the respective aspect of fuzzy comparison is
     * suppressed, but the function is still valid.
     **/
    bool lt(double a, double b);
    bool lt(double a, double b, double relative_tolerance);
    bool lt(double a, double b, double relative_tolerance, double absolute_tolerance);

    /**
     * @brief Return a non-zero value if the specified 'a' and 'b' satisfy
     * the fuzzy less-than-or-equal-to relation (a ~le b), and 0 otherwise.
     *
     * Optionally specify the relative tolerance 'relative_tolerance', or 
     * 'relative_tolerance' and the absolute tolerance 'absolute_tolerance',
     * used to determine fuzzy equality. If optional parameters are not
     * specified,  reasonable implementation-dependent default tolerances
     * are used. If relative_tolerance <= 0 or absolute_tolerance <= 0,
     * the respective aspect of fuzzy comparison is suppressed, but the
     * function is still valid.
     **/
    bool le(double a, double b);
    bool le(double a, double b, double relative_tolerance);
    bool le(double a, double b, double relative_tolerance, double absolute_tolerance);

    /**
     * @brief Return a non-zero value if the specified 'a' and 'b' satisfy
     * the fuzzy greater-than-or-equal-to relation (a ~ge b), and 0 otherwise.
     *
     * Optionally specify the relative tolerance 'relative_tolerance', or
     * 'relative_tolerance' and the absolute tolerance 'absolute_tolerance',
     * used to determine fuzzy equality. If optional parameters are not
     * specified, default tolerances are used. If relative_tolerance <= 0
     * or absolute_tolerance <= 0, the respective aspect of fuzzy
     * comparison is suppressed, but the function is still valid.
     **/
    bool ge(double a, double b);
    bool ge(double a, double b, double relative_tolerance);
    bool ge(double a, double b, double relative_tolerance, double absolute_tolerance);

    /**
     * @brief Return a non-zero value if the specified 'a' and 'b' satisfy
     * the fuzzy greater-than relation (a ~gt b), and 0 otherwise.
     *
     * Optionally specify the relative tolerance 'relative_tolerance', or
     * 'relative_tolerance' and the absolute tolerance 'absolute_tolerance',
     * used to determine fuzzy equality.If optional parameters are not
     * specified, reasonable implementation-dependent default tolerances
     * are used.  If relative_tolerance <= 0 or absolute_tolerance <= 0,
     * the respective aspect of fuzzy comparison is suppressed, but the
     * function is still valid.
     **/
    bool gt(double a, double bl);
    bool gt(double a, double b, double relative_tolerance);
    bool gt(double a, double b, double relative_tolerance, double absolute_tolerance);

    /**
     * Return the absolute value of the specified 'input'.
     * This is used instead of std::fabs to avoid a dependency
     * on the math library.
     */
    double fabsval(double input);
}

// ====================
// INLINE definitions
// ====================

inline 
double double_compare::fabsval(double input)
{
    return 0.0 <= input ? input : -input;
}

inline
int
double_compare::fuzzy_compare(double a, double b, 
                              double relative_tolerance, 
                              double absolute_tolerance)
{
    if (a == b)
    {
        // Special case: equality. Done.
        return 0;
    }

    if (a == -b)
    {
        // Special case: Relative difference
        if (fabsval(a - b) <= absolute_tolerance)
        {
            // is "infinite"
            // Fuzzy equality (via abs. tol. only)
            return 0;
        }
        else if (a > b)
        {
            return 1;           // Fuzzy greater than
        }
        else
        {
            return -1;          // Fuzzy less than
        }
    }

    const double diff = fabsval(a - b);
    const double average = fabsval((a + b) / 2.0);

    if (diff <= absolute_tolerance || diff / average <= relative_tolerance)
    {
        return 0;               // Fuzzy equality.
    }
    else if (a > b)
    {
        return 1;               // Fuzzy greater than
    }
    else
    {
        return -1;              // Fuzzy less than
    }
}

inline
bool
double_compare::eq(double a, double b)
{
    return fuzzy_compare(a, b, relative_tolerance_default, absolute_tolerance_default) == 0;
}

inline
bool
double_compare::eq(double a, double b, double relative_tolerance)
{
    return fuzzy_compare(a, b, relative_tolerance, absolute_tolerance_default) == 0;
}

inline
bool
double_compare::eq(double a, double b, double relative_tolerance, double absolute_tolerance)
{
    return fuzzy_compare(a, b, relative_tolerance, absolute_tolerance) == 0;
}

inline
bool
double_compare::ne(double a, double b)
{
    return fuzzy_compare(a, b, relative_tolerance_default, absolute_tolerance_default) != 0;
}

inline
bool
double_compare::ne(double a, double b, double relative_tolerance)
{
    return fuzzy_compare(a, b, relative_tolerance, absolute_tolerance_default) != 0;
}

inline
bool
double_compare::ne(double a, double b, double relative_tolerance, double absolute_tolerance)
{
    return fuzzy_compare(a, b, relative_tolerance, absolute_tolerance) != 0;
}

inline
bool
double_compare::lt(double a, double b)
{
    return fuzzy_compare(a, b, relative_tolerance_default, absolute_tolerance_default) < 0;
}

inline
bool
double_compare::lt(double a, double b, double relative_tolerance)
{
    return fuzzy_compare(a, b, relative_tolerance, absolute_tolerance_default) < 0;
}

inline
bool
double_compare::lt(double a, double b, double relative_tolerance, double absolute_tolerance)
{
    return fuzzy_compare(a, b, relative_tolerance, absolute_tolerance) < 0;
}

inline
bool
double_compare::le(double a, double b)
{
    return fuzzy_compare(a, b, relative_tolerance_default, absolute_tolerance_default) <= 0;
}

inline
bool
double_compare::le(double a, double b, double relative_tolerance)
{
    return fuzzy_compare(a, b, relative_tolerance, absolute_tolerance_default) <= 0;
}

inline
bool
double_compare::le(double a, double b, double relative_tolerance, double absolute_tolerance)
{
    return fuzzy_compare(a, b, relative_tolerance, absolute_tolerance) <= 0;
}

inline
bool
double_compare::ge(double a, double b)
{
    return fuzzy_compare(a, b, relative_tolerance_default, absolute_tolerance_default) >= 0;
}

inline
bool
double_compare::ge(double a, double b, double relative_tolerance)
{
    return fuzzy_compare(a, b, relative_tolerance, absolute_tolerance_default) >= 0;
}

inline
bool
double_compare::ge(double a, double b, double relative_tolerance, double absolute_tolerance)
{
    return fuzzy_compare(a, b, relative_tolerance, absolute_tolerance) >= 0;
}

inline
bool
double_compare::gt(double a, double b)
{
    return fuzzy_compare(a, b, relative_tolerance_default, absolute_tolerance_default) > 0;
}

inline
bool
double_compare::gt(double a, double b, double relative_tolerance)
{
    return fuzzy_compare(a, b, relative_tolerance, absolute_tolerance_default) > 0;
}

inline
bool
double_compare::gt(double a, double b, double relative_tolerance, double absolute_tolerance)
{
    return fuzzy_compare(a, b, relative_tolerance, absolute_tolerance) > 0;
}


} //  namespace

#endif
