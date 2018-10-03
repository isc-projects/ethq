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
#include <map>
#include <regex>

//
// abstrat base class for stats string parsers, includes a static
// registry that maps from NIC driver names to parsers
//

class StringsetParser {

public:
	typedef StringsetParser* ptr_t;
	typedef std::map<std::string, ptr_t> parsermap_t;
	typedef std::vector<std::string> driverlist_t;

private:
	static parsermap_t parsers;

public:
	virtual bool match(const std::string& key, size_t value, size_t& queue, bool& rx, bool& bytes) = 0;

protected:
	void save(const std::string& driver);
	void save(const driverlist_t& drivers);

public:
	static ptr_t find(const std::string& driver);

};

//
// abstract base class for parsers that need to remember the
// values seen in previous lines of input
//
class StatefulParser : public StringsetParser {

protected:
	size_t		queue = -1;
	bool		rx = false;
	bool		bytes = false;

};

//
// abstract base classes for parsers that need a single
// regular expression to match all three fields
//
class Stringset_RE_Parser : public StringsetParser {

protected:
	std::regex	re;
	std::smatch	ma;
	std::string	ms(size_t n);

public:
	Stringset_RE_Parser(const driverlist_t& drivers, const std::string& match);
};

//
// concrete class that knows how to extract the three fields
// from groups in a regex so long as they appear in the order
// (direction, queue number, metric type)
//
class RE_DNT_Parser : public Stringset_RE_Parser {

public:
	RE_DNT_Parser(const driverlist_t& drivers, const std::string& match);
	virtual bool match(const std::string& key, size_t value, size_t& queue, bool& rx, bool& bytes);
};
