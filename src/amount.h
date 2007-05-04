/**
 * @file   amount.h
 * @author John Wiegley
 * @date   Wed Apr 18 22:05:53 2007
 * 
 * @brief  Basic type for handling commoditized math: amount_t.
 * 
 * This file contains the most basic numerical type in Ledger:
 * amount_t, which relies upon commodity.h (commodity_t) for handling
 * commoditized amounts.  This class allows Ledger to handle
 * mathematical expressions involving differing commodities, as well
 * as math using no commodities at all (such as increasing a dollar
 * amount by a multiplier).
 */

/*
 * Copyright (c) 2003-2007, John Wiegley.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of New Artisans LLC nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _AMOUNT_H
#define _AMOUNT_H

#include "utils.h"

namespace ledger {

class commodity_t;
class annotation_t;
class commodity_pool_t;

DECLARE_EXCEPTION(amount_error);

/**
 * @class amount_t
 *
 * @brief Encapsulates infinite-precision commoditized amounts.
 *
 * The amount_t class can be used for commoditized infinite-precision
 * math, and also for uncommoditized math.  In the commoditized case,
 * commodities keep track of how they are used, and will always
 * display back to the user after the same fashion.  For
 * uncommoditized numbers, no display truncation is ever done.  In
 * both cases, internal precision is always kept to an excessive
 * degree.
 */
class amount_t
  : public ordered_field_operators<amount_t,
	   ordered_field_operators<amount_t, long,
	   ordered_field_operators<amount_t, unsigned long,
	   ordered_field_operators<amount_t, double> > > >
{
public:
  class bigint_t;

  typedef uint_least16_t precision_t;

  /** 
   * The initialize and shutdown methods ready the amount subsystem
   * for use.  Normally they are called by `ledger::initialize' and
   * `ledger::shutdown'.
   */
  static void initialize();
  static void shutdown();

  /**
   * The default_pool is a static variable indicating which commodity
   * pool should be used when none is specified.
   */
  static commodity_pool_t * default_pool;

  /**
   * The `keep_base' member determines whether scalable commodities
   * are automatically converted to their most reduced form when
   * printing.  The default is true.
   *
   * For example, Ledger supports time values specified in seconds
   * (10s), hours (5.2h) or minutes.  Internally, such amounts are
   * always kept as quantities of seconds.  However, when streaming
   * the amount Ledger will convert it to its "least representation",
   * which is "5.2h" in the second case.  If `keep_base' is true, this
   * amount is displayed as "18720s".
   */
  static bool keep_base;

  /**
   * The following three members determine whether lot details are
   * maintained when working with commoditized values.  The default is
   * false for all three.
   *
   * Let's say a user adds two values of the following form:
   *   10 AAPL + 10 AAPL {$20}
   *
   * This expression adds ten shares of Apple stock with another ten
   * shares that were purchased for $20 a share.  If `keep_price' is
   * false, the result of this expression will be an amount equal to
   * 20 AAPL.  If `keep_price' is true, the expression yields an
   * exception for adding amounts with different commodities.  In that
   * case, a balance_t object must be used to store the combined sum.
   */
  static bool keep_price;
  static bool keep_date;
  static bool keep_tag;

  /**
   * The `full-strings' static member is currently only used by the
   * unit testing code.  It causes amounts written to streams to use
   * the `to_fullstring' method rather than the `to_string' method, so
   * that complete precision is always displayed, no matter what the
   * precision of an individual commodity might be.
   * @see to_string
   * @see to_fullstring
   */
  static bool full_strings;

protected:
  void _init();
  void _copy(const amount_t& amt);
  void _dup();
  void _resize(precision_t prec);
  void _clear();
  void _release();

  bigint_t *	quantity;
  commodity_t *	commodity_;

public:
  /**
   * Constructors.  amount_t supports several forms of construction:
   *
   * amount_t() creates a value for which `is_null' is true, and which
   * has no value or commodity.  If used in value situations it will
   * be zero, and its commodity equals `commodity_t::null_commodity'.
   *
   * amount_t(double), amount_t(unsigned long), amount_t(long) all
   * convert from the respective numerics type to an amount.  No
   * precision or sign is lost in any of these conversions.  The
   * resulting commodity is always `commodity_t::null_commodity'.
   *
   * amount_t(string), amount_t(char*) both convert from a string
   * representation of an amount, which may or may not include a
   * commodity.  This is the proper way to initialize an amount like
   * '$100.00'.
   */
  amount_t() : quantity(NULL), commodity_(NULL) {
    TRACE_CTOR(amount_t, "");
  }
  amount_t(const double val);
  amount_t(const unsigned long val);
  amount_t(const long val);

  amount_t(const string& val) : quantity(NULL) {
    TRACE_CTOR(amount_t, "const string&");
    parse(val);
  }
  amount_t(const char * val) : quantity(NULL) {
    TRACE_CTOR(amount_t, "const char *");
    parse(val);
  }

  /**
   * Static creator function.  Calling amount_t::exact(string) will
   * create an amount whose display precision is never truncated, even
   * if the amount uses a commodity (which normally causes "round on
   * streaming" to occur).  This function is mostly used by the
   * debugging code.  It is the proper way to initialize '$100.005',
   * where display of the extra precision is required.  If a regular
   * constructor is used, this amount will stream as '$100.01', even
   * though its internal value always equals $100.005.
   */
  static amount_t exact(const string& value);

  /**
   * Destructor.  Releases the reference count held for the underlying
   * bigint_t object.
   */
  ~amount_t() {
    TRACE_DTOR(amount_t);
    if (quantity)
      _release();
  }

  /**
   * Assignment and copy operators.  An amount may be assigned or
   * copied.  If a double, long or unsigned long is assigned to an
   * amount, a temporary is constructed, and then the temporary is
   * assigned to `this'.  Both the value and the commodity are copied,
   * causing the result to compare equal to the reference amount.
   *
   * Note: `quantity' must be initialized to NULL first, otherwise the
   * `_copy' function will attempt to release the unitialized pointer.
   */
  amount_t(const amount_t& amt) : quantity(NULL) {
    TRACE_CTOR(amount_t, "copy");
    if (amt.quantity)
      _copy(amt);
    else
      commodity_ = NULL;
  }
  amount_t& operator=(const amount_t& amt);

  /**
   * Comparison operators.  The fundamental comparison operation for
   * amounts is `compare', which returns a value less than, greater
   * than or equal to zero.  All the other comparison operators are
   * defined in terms of this method.  The only special detail is that
   * `operator==' will fail immediately if amounts with different
   * commodities are being compared.  Otherwise, if the commodities
   * are equivalent (@see keep_price, et al), then the amount
   * quantities are compared numerically.
   *
   * Comparison between an amount and a double, long or unsigned long
   * is allowed.  In such cases the non-amount value is constructed as
   * an amount temporary, which is then compared to `this'.
   */
  int compare(const amount_t& amt) const;

  bool operator==(const amount_t& amt) const;

  template <typename T>
  bool operator==(const T& val) const {
    return compare(val) == 0;
  }
  template <typename T>
  bool operator<(const T& amt) const {
    return compare(amt) < 0;
  }
  template <typename T>
  bool operator>(const T& amt) const {
    return compare(amt) > 0;
  }

  /**
   * Binary arithmetic operators.  Amounts support addition,
   * subtraction, multiplication and division -- but not modulus,
   * bitwise operations, or shifting.  Arithmetic is also supported
   * between amounts, double, long and unsigned long, in which case
   * temporary amount are constructed for the life of the expression.
   *
   * Although only in-place operators are defined here, the remainder
   * are provided by `boost::ordered_field_operators<>'.
   */
  amount_t& operator+=(const amount_t& amt);
  amount_t& operator-=(const amount_t& amt);
  amount_t& operator*=(const amount_t& amt);
  amount_t& operator/=(const amount_t& amt);

  /**
   * Unary arithmetic operators.  There are several unary methods
   * support on amounts:
   *
   * negate(), also unary minus (- x), returns the negated value of an
   * amount.
   *
   * abs() returns the absolute value of an amount.  It is equivalent
   * to: `(x < 0) ? - x : x'.
   *
   * round(precision_t) rounds an amount's internal value to the given
   * precision.
   *
   * round() rounds an amount to its commodity's current display
   * precision.  This also changes the internal value of the amount.
   *
   * unround() yields an amount whose display precision is never
   * truncated, even though its commodity normally displays only
   * rounded values.
   *
   * reduce() reduces a value to its most basic commodity form, for
   * amounts that utilize "scaling commodities".  For example, an
   * amount of 1h after reduction will be 3600s.
   *
   * unreduce(), if used with a "scaling commodity", yields the most
   * compact form greater than 1.0.  That is, 3599s will unreduce to
   * 59.98m, while 3601 unreduces to 1h.
   *
   * value(optional<moment_t>) returns the historical value for an
   * amount -- the default moment returns the most recently known
   * price -- based on the price history of its commodity.  For
   * example, if the amount were 10 AAPL, and on Apr 10, 2000 each
   * share of AAPL was worth $10, then call value() for that moment in
   * time would yield the amount $100.00.
   *
   * Further, for the sake of efficiency and avoiding temporary
   * objects, the following methods support "in-place" variants that
   * act on the value itself and return a reference to the result
   * (`*this'):
   *
   * in_place_negate()
   * in_place_reduce()
   * in_place_unreduce()
   */
  amount_t negate() const {
    amount_t temp = *this;
    temp.in_place_negate();
    return temp;
  }
  amount_t& in_place_negate();

  amount_t operator-() const {
    return negate();
  }

  amount_t abs() const {
    if (sign() < 0)
      return negate();
    return *this;
  }

  amount_t round(precision_t prec) const;
  amount_t round() const;
  amount_t unround() const;

  amount_t reduce() const {
    amount_t temp(*this);
    temp.in_place_reduce();
    return temp;
  }
  amount_t& in_place_reduce();

  amount_t unreduce() const {
    amount_t temp(*this);
    temp.in_place_unreduce();
    return temp;
  }
  amount_t& in_place_unreduce();

  optional<amount_t> value(const optional<moment_t>& moment =
			   optional<moment_t>()) const;

  /**
   * Truth tests.  An amount may be truth test in several ways:
   *
   * sign() returns an integer less than, greater than, or equal to
   * zero depending on whether an amount is negative, zero, or greater
   * than zero.  Note that this function tests the actual value of the
   * amount -- using its internal precision -- and not the display
   * value.  To test its display value, use: `round().sign()'.
   *
   * nonzero(), or operator bool, returns true if an amount's display
   * value is not zero.
   *
   * zero() returns true if an amount's display value is zero.  Thus,
   * $0.0001 is considered zero().
   *
   * realzero() returns true if an amount's actual value is zero.
   * $0.0001 is not considered realzero().
   *
   * is_null() returns true if an amount has no value and no
   * commodity.  This occurs only if an unitialized amount has never
   * been assigned a value.
   */
  int sign() const;

  operator bool() const {
    return nonzero();
  }
  bool nonzero() const {
    return ! zero();
  }

  bool zero() const;
  bool realzero() const {
    return sign() == 0;
  }

  bool is_null() const {
    return ! quantity && ! has_commodity();
  }

  /**
   * Conversion methods.  An amount may be converted to the same types
   * it can be constructed from -- with the exception of unsigned
   * long.  Implicit conversions are not allowed in C++ (though they
   * are in Python), rather the following conversion methods must be
   * called explicitly:
   *
   * to_double() returns an amount as a double.  Note: precision is
   * very likely to be lost in this conversion!
   *
   * to_long() returns an amount as a long integer.  This is only
   * useful if the amount is know to be of a small, integral value.
   *
   * to_string() returns an amount'ss "display value" as a string --
   * after rounding the value according to the commodity's default
   * precision.  It is equivalent to: `round().to_fullstring()'.
   *
   * to_fullstring() returns an amount's "internal value" as a string,
   * without any rounding.
   *
   * quantity_string() returns an amount's "display value", but
   * without any commodity.  Note that this is different from
   * `number().to_string()', because in that case the commodity has
   * been stripped and the full, internal precision of the amount
   * would be displayed.
   */
  double to_double() const;
  long   to_long() const;
  string to_string() const;
  string to_fullstring() const;
  string quantity_string() const;

  /**
   * Commodity-related methods.  The following methods relate to an
   * amount's commodity:
   *
   * has_commodity() returns true if the amount has a commodity.
   *
   * commodity() returns an amount's commodity.  If the amount has no
   * commodity, then the value returned will be equal to
   * `commodity_t::null_commodity'.
   *
   * set_commodity(commodity_t) sets an amount's commodity to the
   * given value.  Note that this merely sets the current amount to
   * that commodity, it does not "observe" the amount for possible
   * changes in the maximum display precision of the commodity, the
   * way that `parse' does.
   *
   * clear_commodity() sets an amount's commodity to null, such that
   * has_commodity() afterwards returns false.
   *
   * number() returns a commodity-less version of an amount.  This is
   * useful for accessing just the numeric portion of an amount.
   */
  bool has_commodity() const;
  commodity_t& commodity() const;

  void set_commodity(commodity_t& comm) {
    commodity_ = &comm;
  }
  void clear_commodity() {
    commodity_ = NULL;
  }

  amount_t number() const {
    if (! has_commodity())
      return *this;
    amount_t temp(*this);
    temp.clear_commodity();
    return temp;
  }

  /**
   * Annotated commodity methods.  An amount's commodity may be
   * annotated with special details, such as the price it was
   * purchased for, when it was acquired, or an arbitrary note,
   * identifying perhaps the lot number of an item.
   *
   * annotate_commodity(amount_t price, [moment_t date, string tag])
   * sets the annotations for the current amount's commodity.  Only
   * the price argument is required, although it can be passed as
   * `optional<amount_t>()' if no price is desired.
   *
   * commodity_annotated() returns true if an amount's commodity has
   * any annotation details associated with it.
   *
   * annotation_details() returns all of the details of an annotated
   * commodity's annotations.  The structure returns will evaluate as
   * boolean false if there are no details.
   *
   * strip_annotations([keep_price, keep_date, keep_tag]) returns an
   * amount whose commodity's annotations have been stripped.  The
   * three `keep_' arguments determine which annotation detailed are
   * kept, meaning that the default is to follow whatever
   * amount_t::keep_price, amount_t::keep_date and amount_t::keep_tag
   * have been set to (which all default to false).
   */
  void         annotate_commodity(const annotation_t& details);
  bool         commodity_annotated() const;
  annotation_t annotation_details() const;
  amount_t     strip_annotations(const bool _keep_price = keep_price,
				 const bool _keep_date  = keep_date,
				 const bool _keep_tag   = keep_tag) const;

  /**
   * Parsing methods.  The method `parse' is used to parse an amount
   * from an input stream or a string.  A global operator>> is also
   * defined which simply calls parse on the input stream.  The
   * `parse' method has two forms:
   *
   * parse(commodity_pool_t, istream, flags_t) parses an
   * amount from the given input stream, registering commodity details
   * according to the commodity pool which is passed in as the first
   * parameter.
   *
   * parse(istream, flags_t) is the same as the preceding function,
   * only it uses `amount_t::default_pool' as the commodity pool.
   *
   * parse(commodity_pool_t, string, flags_t) parses an amount from
   * the given string.
   *
   * parse(string, flags_t) also parses an amount from a string.
   *
   * The `flags' argument of both parsing may be one or more of the
   * following:
   *
   * AMOUNT_PARSE_NO_MIGRATE means to not pay attention to the way an
   * amount is used.  Ordinarily, if an amount were $100.001, for
   * example, it would cause the default display precision for $ to be
   * "widened" to three decimal places.  If AMOUNT_PARSE_NO_MIGRATE is
   * used, the commodity's default display precision is not changed.
   *
   * AMOUNT_PARSE_NO_REDUCE means not to call in_place_reduce() on the
   * resulting amount after it is parsed.
   *
   * These parsing methods observe the amounts they parse (unless
   * AMOUNT_PARSE_NO_MIGRATE is true), and set the display details of
   * the corresponding commodity accordingly.  This way, amounts do
   * not require commodities to be pre-defined in any way, but merely
   * displays them back to the user in the same fashion as it saw them
   * used.
   *
   * There is also a static convenience method called
   * `parse_conversion' which can be used to define a relationship
   * between scaling commodity values.  For example, Ledger uses it to
   * define the relationships among various time values:
   *
   *   amount_t::parse_conversion("1.0m", "60s"); // a minute is 60 seconds
   *   amount_t::parse_conversion("1.0h", "60m"); // an hour is 60 minutes
   */
#define AMOUNT_PARSE_NO_MIGRATE 0x01
#define AMOUNT_PARSE_NO_REDUCE  0x02

  typedef uint_least8_t flags_t;

  void parse(commodity_pool_t& parent, std::istream& in, flags_t flags = 0);
  void parse(commodity_pool_t& parent, const string& str, flags_t flags = 0) {
    std::istringstream stream(str);
    parse(parent, stream, flags);
  }

  void parse(std::istream& in, flags_t flags = 0) {
    assert(default_pool);
    parse(*default_pool, in, flags);
  }
  void parse(const string& str, flags_t flags = 0) {
    assert(default_pool);
    parse(*default_pool, str, flags);
  }

  static void parse_conversion(commodity_pool_t& parent,
			       const string& larger_str,
			       const string& smaller_str);

  static void parse_conversion(const string& larger_str,
			       const string& smaller_str) {
    assert(default_pool);
    parse_conversion(*default_pool, larger_str, smaller_str);
  }

  /**
   * Printing methods.  An amount may be output to a stream using the
   * `print' method.  There is also a global operator<< defined which
   * simply calls print for an amount on the given stream.  There is
   * one form of the print method, which takes one required argument
   * and two arguments with default values:
   *
   * print(ostream, bool omit_commodity = false, bool full_precision =
   * false) prits an amounts to the given output stream, using its
   * commodity's default display characteristics.  If `omit_commodity'
   * is true, the commodity will not be displayed, only the amount
   * (although the commodity's display precision is still used).  If
   * `full_precision' is true, the full internal precision of the
   * amount is displayed, regardless of its commodity's display
   * precision.
   */
  void print(std::ostream& out, bool omit_commodity = false,
	     bool full_precision = false) const;

  /**
   * Serialization methods.  An amount may be deserialized from an
   * input stream or a character pointer, and it may be serialized to
   * an output stream.  The methods used are:
   *
   * read(commodity_pool_t, istream) reads an amount from the given
   * input stream.  It must have been put there using
   * `write(ostream)'.  Also, the given pool must be exactly what it
   * was at the time the amount was `written'.  Thus, the required
   * flow of logic is:
   *   amount.write(out)
   *   pool.write(out)
   *   pool.read(in)
   *   amount.read(pool, in)
   *
   * read(istream) does the same as read, only it relies on
   * `amount_t::default_pool' to specify the pool.
   *
   * read(commodity_pool_t, char *&) reads an amount from data which
   * has been read from an input stream into a buffer.  it advances
   * the pointer passed in to the end of the deserialized amount.
   *
   * read(char *&) does the same as read, only it relies on
   * `amount_t::default_pool' to specify the pool.
   *
   * write(ostream) writes an amount to an output stream in a compact
   * binary format.
   */
  void read(commodity_pool_t& parent, std::istream& in);
  void read(commodity_pool_t& parent, char *& data);

  void read(std::istream& in) {
    assert(default_pool);
    read(*default_pool, in);
  }
  void read(char *& data) {
    assert(default_pool);
    read(*default_pool, data);
  }

  void write(std::ostream& out) const;

private:
  void read_quantity(std::istream& in);
  void read_quantity(char *& data);
  void write_quantity(std::ostream& out) const;

public:
  /**
   * Debugging methods.  There are two methods defined to help with
   * debugging:
   *
   * dump(ostream) dumps an amount to an output stream.  There is
   * little different from print(), it simply surrounds the display
   * value with a marker, for example "AMOUNT($1.00)".  This code is
   * used by other dumping code elsewhere in Ledger.
   *
   * valid() returns true if an amount is valid.  This ensures that if
   * an amount has a commodity, it has a valid value pointer, for
   * example, even if that pointer simply points to a zero value.
   */
  void dump(std::ostream& out) const {
    out << "AMOUNT(";
    print(out);
    out << ")";
  }

  bool valid() const;

private:
  friend bool parse_annotations(commodity_pool_t& parent,
				std::istream&	  in,
				annotation_t&	  details);
};

inline amount_t amount_t::exact(const string& value) {
  amount_t temp;
  temp.parse(value, AMOUNT_PARSE_NO_MIGRATE);
  return temp;
}

inline string amount_t::to_string() const {
  std::ostringstream bufstream;
  print(bufstream);
  return bufstream.str();
}

inline string amount_t::to_fullstring() const {
  std::ostringstream bufstream;
  print(bufstream, false, true);
  return bufstream.str();
}

inline string amount_t::quantity_string() const {
  std::ostringstream bufstream;
  print(bufstream, true);
  return bufstream.str();
}

inline std::ostream& operator<<(std::ostream& out, const amount_t& amt) {
  amt.print(out, false, amount_t::full_strings);
  return out;
}
inline std::istream& operator>>(std::istream& in, amount_t& amt) {
  amt.parse(in);
  return in;
}

} // namespace ledger

#include "commodity.h"

namespace ledger {

inline bool amount_t::operator==(const amount_t& amt) const {
  if (commodity() != amt.commodity())
    return false;
  return compare(amt) == 0;
}

inline amount_t amount_t::round() const {
  if (! has_commodity())
    return *this;
  return round(commodity().precision());
}

inline bool amount_t::has_commodity() const {
  return commodity_ && commodity_ != commodity_->parent().null_commodity;
}

inline commodity_t& amount_t::commodity() const {
  // jww (2007-05-02): Should be a way to access null_commodity better
  return has_commodity() ? *commodity_ : *default_pool->null_commodity;
}

} // namespace ledger

#endif // _AMOUNT_H