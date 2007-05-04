/**
 * @file   commodity.h
 * @author John Wiegley
 * @date   Wed Apr 18 22:05:53 2007
 * 
 * @brief  Types for handling commodities.
 * 
 * This file contains one of the most basic types in Ledger:
 * commodity_t, and its annotated cousin, annotated_commodity_t.
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

#ifndef _COMMODITY_H
#define _COMMODITY_H

namespace ledger {

#define COMMODITY_STYLE_DEFAULTS   0x0000
#define COMMODITY_STYLE_SUFFIXED   0x0001
#define COMMODITY_STYLE_SEPARATED  0x0002
#define COMMODITY_STYLE_EUROPEAN   0x0004
#define COMMODITY_STYLE_THOUSANDS  0x0008
#define COMMODITY_STYLE_NOMARKET   0x0010
#define COMMODITY_STYLE_BUILTIN    0x0020

class commodity_base_t : public noncopyable
{
private:
  friend class commodity_pool_t;
  friend class commodity_t;
  friend class annotated_commodity_t;

  typedef std::map<const moment_t, amount_t>  history_map;
  typedef std::pair<const moment_t, amount_t> history_pair;

  struct history_t {
    history_map	prices;
    ptime	last_lookup;
  };

  typedef uint_least8_t flags_t;

  flags_t		flags;
  string		symbol;
  amount_t::precision_t	precision;
  optional<string>	name;
  optional<string>	note;
  optional<history_t>	history;
  optional<amount_t>	smaller;
  optional<amount_t>	larger;

public:
  explicit commodity_base_t()
    : flags(COMMODITY_STYLE_DEFAULTS), precision(0) {
    TRACE_CTOR(commodity_base_t, "");
  }
  explicit commodity_base_t
    (const string&	   _symbol,
     amount_t::precision_t _precision = 0,
     unsigned int          _flags     = COMMODITY_STYLE_DEFAULTS)
    : flags(_flags), symbol(_symbol), precision(_precision) {
    TRACE_CTOR(commodity_base_t,
	       "const string&, amount_t::precision_t, unsigned int");
  }
  ~commodity_base_t() {
    TRACE_DTOR(commodity_base_t);
  }
};

class annotated_commodity_t;

class commodity_t
  : public equality_comparable1<commodity_t, noncopyable>
{
public:
  static bool symbol_needs_quotes(const string& symbol);

  typedef commodity_base_t::flags_t	 flags_t;
  typedef commodity_base_t::history_t	 history_t;
  typedef commodity_base_t::history_map  history_map;
  typedef commodity_base_t::history_pair history_pair;
  typedef uint_least32_t		 ident_t;

  shared_ptr<commodity_base_t> base;

  commodity_pool_t * parent_;
  ident_t	     ident;
  optional<string>   qualified_symbol;
  optional<string>   mapping_key_;
  bool		     annotated;

public:
  explicit commodity_t(commodity_pool_t * _parent,
		       const shared_ptr<commodity_base_t>& _base)
    : base(_base), parent_(_parent), annotated(false) {
    TRACE_CTOR(commodity_t, "");
  }
  virtual ~commodity_t() {
    TRACE_DTOR(commodity_t);
  }

  operator bool() const;

  virtual bool operator==(const commodity_t& comm) const {
    if (comm.annotated)
      return comm == *this;
    return base.get() == comm.base.get();
  }

  commodity_pool_t& parent() const {
    return *parent_;
  }

  annotated_commodity_t& as_annotated();
  const annotated_commodity_t& as_annotated() const;

  string base_symbol() const {
    return base->symbol;
  }
  string symbol() const {
    return qualified_symbol ? *qualified_symbol : base_symbol();
  }

  string mapping_key() const {
    if (mapping_key_)
      return *mapping_key_;
    else
      return base_symbol();
  }

  optional<string> name() const {
    return base->name;
  }
  void set_name(const optional<string>& arg = optional<string>()) {
    base->name = arg;
  }

  optional<string> note() const {
    return base->note;
  }
  void set_note(const optional<string>& arg = optional<string>()) {
    base->note = arg;
  }

  amount_t::precision_t precision() const {
    return base->precision;
  }
  void set_precision(amount_t::precision_t arg) {
    base->precision = arg;
  }

  flags_t flags() const {
    return base->flags;
  }
  void set_flags(flags_t arg) {
    base->flags = arg;
  }
  void add_flags(flags_t arg) {
    base->flags |= arg;
  }
  void drop_flags(flags_t arg) {
    base->flags &= ~arg;
  }

  optional<amount_t> smaller() const {
    return base->smaller;
  }
  void set_smaller(const optional<amount_t>& arg = optional<amount_t>()) {
    base->smaller = arg;
  }

  optional<amount_t> larger() const {
    return base->larger;
  }
  void set_larger(const optional<amount_t>& arg = optional<amount_t>()) {
    base->larger = arg;
  }

  optional<history_t> history() const {
    return base->history;
  }

  void	   add_price(const moment_t& date, const amount_t& price);
  bool	   remove_price(const moment_t& date);

  optional<amount_t> value(const optional<moment_t>& moment =
			   optional<moment_t>());

  void write(std::ostream& out) const {
    out << symbol();
  }

  bool valid() const;
};

inline std::ostream& operator<<(std::ostream& out, const commodity_t& comm) {
  comm.write(out);
  return out;
}

struct annotation_t : public equality_comparable<annotation_t>
{
  optional<amount_t>  price;
  optional<moment_t>  date;
  optional<string>    tag;

  explicit annotation_t(const optional<amount_t>& _price = optional<amount_t>(),
			const optional<moment_t>& _date  = optional<moment_t>(),
			const optional<string>&   _tag   = optional<string>())
    : price(_price), date(_date), tag(_tag) {}

  operator bool() const {
    return price || date || tag;
  }

  bool operator==(const annotation_t& rhs) const {
    return (price == rhs.price &&
	    date  == rhs.date &&
	    tag   == rhs.tag);
  }

  void write(std::ostream& out) const {
    out << "price " << (price ? price->to_string() : "NONE") << " "
	<< "date "  << (date  ? *date : moment_t()) << " "
	<< "tag "   << (tag   ? *tag  : "NONE");
  }    

  bool valid() const {
    assert(*this);
  }
};

inline std::ostream& operator<<(std::ostream& out, const annotation_t& details) {
  details.write(out);
  return out;
}

class annotated_commodity_t
  : public commodity_t,
           equality_comparable1<annotated_commodity_t, noncopyable>
{
public:
  commodity_t * ptr;
  annotation_t  details;

  explicit annotated_commodity_t(commodity_t * _ptr,
				 const annotation_t& _details)
    : commodity_t(_ptr->parent_, _ptr->base), ptr(_ptr), details(_details) {
    TRACE_CTOR(annotated_commodity_t, "");
    annotated = true;
  }
  virtual ~annotated_commodity_t() {
    TRACE_DTOR(annotated_commodity_t);
  }

  virtual bool operator==(const commodity_t& comm) const;

  commodity_t& referent() {
    return *ptr;
  }
  const commodity_t& referent() const {
    return *ptr;
  }

  void write_annotations(std::ostream& out) const {
    annotated_commodity_t::write_annotations(out, details);
  }

  static void write_annotations(std::ostream&	    out,
				const annotation_t& info);
};

struct compare_amount_commodities {
  bool operator()(const amount_t * left, const amount_t * right) const;
};

class commodity_pool_t : public noncopyable
{
public:
  /**
   * The commodities collection in commodity_pool_t maintains pointers
   * to all the commodities which have ever been created by the user,
   * whether explicitly by calling the create methods of
   * commodity_pool_t, or implicitly by parsing a commoditized amount.
   *
   * The `commodities' member variable represents a collection which
   * is indexed by two vertices: first, and ordered sequence of unique
   * integer which identify commodities by a numerical identifier; and
   * second, by a hashed set of symbolic names which reflect how the
   * commodity was referred to by the user.
   */
  typedef multi_index_container<
    commodity_t *,
    multi_index::indexed_by<
      multi_index::ordered_unique<
	multi_index::member<commodity_t,
			    commodity_t::ident_t, &commodity_t::ident> >,
      multi_index::hashed_unique<
	multi_index::const_mem_fun<commodity_t,
				   string, &commodity_t::mapping_key> >
    >
  > commodities_t;

  commodities_t commodities;
  commodity_t *	null_commodity;
  commodity_t *	default_commodity;

private:
  template<typename T>
  struct first_initialized
  {
    typedef T result_type;

    template<typename InputIterator>
    T operator()(InputIterator first, InputIterator last) const
    {
      for (; first != last; first++)
	if (*first)
	  return *first;
      return T();
    }
  };

public:
  boost::signal<optional<amount_t>
		(commodity_t&		   commodity,
		 const optional<moment_t>& date,
		 const optional<moment_t>& moment,
		 const optional<moment_t>& last),
		first_initialized<optional<amount_t> > > get_quote;

  explicit commodity_pool_t();

  ~commodity_pool_t() {
    typedef commodity_pool_t::commodities_t::nth_index<0>::type
      commodities_by_ident;

    commodities_by_ident& ident_index = commodities.get<0>();
    for (commodities_by_ident::iterator i = ident_index.begin();
	 i != ident_index.end();
	 i++)
      checked_delete(*i);
  }

  commodity_t * create(const string& symbol);
  commodity_t * find(const string& name);
  commodity_t * find(const commodity_t::ident_t ident);
  commodity_t * find_or_create(const string& symbol);

  commodity_t * create(const string& symbol, const annotation_t& details);
  commodity_t * find(const string& symbol, const annotation_t& details);
  commodity_t * find_or_create(const string& symbol,
			       const annotation_t& details);

  commodity_t * create(commodity_t&	   comm,
		       const annotation_t& details,
		       const string&	   mapping_key);

  commodity_t * find_or_create(commodity_t&	   comm,
			       const annotation_t& details);

  void parse_amount(amount_t& amount, std::istream& in,
		    amount_t::flags_t flags = 0) {
    amount.parse(*this, in, flags);
  }
  void parse_amount(amount_t& amount, const string& str,
		    amount_t::flags_t flags = 0) {
    amount.parse(*this, str, flags);
  }

  amount_t parse_amount(std::istream& in, amount_t::flags_t flags = 0) {
    amount_t temp;
    parse_amount(temp, in, flags);
    return temp;
  }
  amount_t parse_amount(const string& str, amount_t::flags_t flags = 0) {
    amount_t temp;
    parse_amount(temp, str, flags);
    return temp;
  }
};

} // namespace ledger

#endif // _COMMODITY_H