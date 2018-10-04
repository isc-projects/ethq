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

StringsetParser::StringsetParser(const driverlist_t& drivers)
{
	save(drivers);
}

void StringsetParser::save(const driverlist_t& drivers) {
	for (const auto& driver: drivers) {
		save(driver);
	}
}

void StringsetParser::save(const std::string& name) {
	parsers[name] = this;
}

StringsetParser::ptr_t StringsetParser::find(const std::string& driver) {
	auto iter = parsers.find(driver);
	if (iter != parsers.end()) {
		return iter->second;
	} else {
		return nullptr;
	}
}

RegexParser::RegexParser(
	const driverlist_t& drivers,
	const std::string& match,
	const order_t& order
) : StringsetParser(drivers)
{
	re.assign(match);
	std::copy(order.cbegin(), order.cend(), this->order.begin());
}

std::string RegexParser::ms(size_t n) {
	return std::ssub_match(ma[n]).str();
}

bool RegexParser::match(const std::string& key, size_t value, size_t& queue, bool& rx, bool& bytes)
{
	auto found = std::regex_match(key, ma, re);
	if (found) {
		rx = (ms(order[0]) == "rx");
		queue = std::stoi(ms(order[1]));
		bytes = (ms(order[2]) == "bytes");
	}
	return found;
}
