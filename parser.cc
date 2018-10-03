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

StringsetParser::parsermap_t StringsetParser::parsers;

void StringsetParser::save(const std::string& name) {
	parsers[name] = this;
}

void StringsetParser::save(const std::vector<std::string>& drivers) {
	for (const auto& driver: drivers) {
		save(driver);
	}
}

StringsetParser::ptr_t StringsetParser::find(const std::string& driver) {
	auto iter = parsers.find(driver);
	if (iter != parsers.end()) {
		return iter->second;
	} else {
		return nullptr;
	}
}

std::string Stringset_RE_Parser::ms(size_t n) {
	return std::ssub_match(ma[n]).str();
}

Stringset_RE_Parser::Stringset_RE_Parser(const driverlist_t& drivers, const std::string& match)
{
	save(drivers);
	re.assign(match);
};

RE_DNT_Parser::RE_DNT_Parser(const driverlist_t& drivers, const std::string& match)
	: Stringset_RE_Parser(drivers, match)
{
}

bool RE_DNT_Parser::match(const std::string& key, size_t value, size_t& queue, bool& rx, bool& bytes)
{
	auto found = std::regex_match(key, ma, re);
	if (found) {
		rx = (ms(1) == "rx");
		queue = std::stoi(ms(2));
		bytes = (ms(3) == "bytes");
	}
	return found;
}
