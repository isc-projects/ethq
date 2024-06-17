/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <regex>

//
// abstract base class for stats string parsers, includes a static
// registry that maps from NIC driver names to parsers
//

class StringsetParser {

public:
	typedef StringsetParser* ptr_t;
	typedef std::map<std::string, ptr_t> parsermap_t;
	typedef std::vector<std::string> driverlist_t;

private:
	static parsermap_t *parsers;

protected:
	void save(const std::string& driver);
	void save(const driverlist_t& drivers);

public:
	StringsetParser(const driverlist_t& drivers);
	virtual ~StringsetParser() = default;

	virtual bool match_total(const std::string& key, size_t value, bool& rx, bool& bytes) {
		return false;
	}

	virtual bool match_queue(const std::string& key, size_t value, bool& rx, bool& bytes, size_t& queue) {
		return false;
	}

public:
	static ptr_t find(const std::string& driver);
};

//
// concrete class that knows how to extract the three fields
// from groups in a regex
//
// the `order` table specifies the order in which the three
// fields (direction, queue number, metric type) appear as
// groups within the regex
//
// NB: match on "direction" requires an exact match for "rx"
//
//     match on "type" requires an exact match for "bytes"
//     or "octets"
//
class RegexParser : public StringsetParser {

public:
	typedef std::array<int, 2> total_order_t;
	typedef std::array<int, 3> queue_order_t;

	typedef std::pair<std::regex, total_order_t>	total_t;
	typedef std::pair<std::regex, queue_order_t>	queue_t;

	typedef std::pair<std::string, total_order_t>	total_str_t;
	typedef std::pair<std::string, queue_order_t>	queue_str_t;

public:
	static total_str_t	total_generic(void);
	static total_str_t	total_nomatch(void);
	static queue_str_t	queue_nomatch(void);

protected:
	std::smatch     	ma;
	total_t			total;
	queue_t			queue;
	std::string     	ms(size_t n);

public:
	RegexParser(const driverlist_t& drivers,
		    const total_str_t& total,
		    const queue_str_t& queue);

	virtual ~RegexParser() = default;

	virtual bool match_total(const std::string& key, size_t value, bool& rx, bool& bytes);
	virtual bool match_queue(const std::string& key, size_t value, bool& rx, bool& bytes, size_t& qnum);
};
