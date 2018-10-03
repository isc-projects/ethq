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

#include <regex>
#include "parser.h"

class StatefulParser : public StringsetParser {

protected:
	size_t		queue = -1;
	bool		rx = false;
	bool		bytes = false;

public:
	StatefulParser(const std::string& name) : StringsetParser(name) {};
};

class Stringset_RE_Parser : public StringsetParser {

protected:
	std::regex	re;
	std::smatch	ma;

	std::string	ms(size_t n) {
		return std::ssub_match(ma[n]).str();
	}

public:
	Stringset_RE_Parser(const std::string& name, const std::string& match)
		: StringsetParser(name)
	{
		re.assign(match);
	}
};

class RE_DNT_Parser : public Stringset_RE_Parser {

public:
	RE_DNT_Parser(const std::string& name, const std::string& match)
		: Stringset_RE_Parser(name, match)
	{
	}

	virtual bool match(const std::string& key, size_t value, size_t& queue, bool& rx, bool& bytes) {
		auto found = std::regex_match(key, ma, re);
		if (found) {
			rx = (ms(1) == "rx");
			queue = std::stoi(ms(2));
			bytes = (ms(3) == "bytes");
		}
		return found;
	}
};

class VMXNet3Parser : public StatefulParser {

private:
	std::regex	re1;
	std::regex	re2;
	std::smatch	ma;

	std::string     ms(size_t n) {
		return std::ssub_match(ma[n]).str();
	}

public:
	VMXNet3Parser(const std::string& name) : StatefulParser(name)
	{
		re1.assign("^(Rx|Tx) Queue#$");
		re2.assign("^\\s*ucast (pkts|bytes) (rx|tx)$");
	}

	bool match(const std::string& key, size_t value, size_t& queue, bool& rx, bool& bytes) {

		// check for match againt queue number
		if (std::regex_match(key, ma, re1)) {
			this->queue = value;
			this->rx = (ms(1) == "Rx");
			return false;
		}

		// check for data entry
		bool found = std::regex_match(key, ma, re2);
		if (found) {
			this->bytes = (ms(1) == "bytes");
		}

		// copy state to caller
		if (found) {
			queue = this->queue;
			rx = this->rx;
			bytes = this->bytes;
		}

		return found;
	}

};

StringsetParser::parsermap_t StringsetParser::parsers;

RE_DNT_Parser i40e("i40e", "^(rx|tx)-(\\d+)\\.\\1_(bytes|packets)$");
RE_DNT_Parser ixgbe("ixgbe", "^(rx|tx)_queue_(\\d+)_(bytes|packets)$");
VMXNet3Parser vmxnet3("vmxnet3");
