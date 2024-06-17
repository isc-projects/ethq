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

StringsetParser::parsermap_t *StringsetParser::parsers = nullptr;

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
	if (!parsers) {
		parsers = new parsermap_t();
	}

	(*parsers)[name] = this;
}

StringsetParser::ptr_t StringsetParser::find(const std::string& driver) {
	auto iter = parsers->find(driver);
	if (iter != parsers->end()) {
		return iter->second;
	} else {
		return nullptr;
	}
}

// parser for the most common generic output format
RegexParser::total_str_t RegexParser::total_generic() {
       return total_str_t { "^(rx|tx)_(bytes|packets)$", { 1, 2 } };
}

// parsers used when the driver does not support total counters or per-queue counters
RegexParser::total_str_t RegexParser::total_nomatch() {
	return total_str_t { "", { 0, 0 } };
}
RegexParser::queue_str_t RegexParser::queue_nomatch() {
	return queue_str_t { "", { 0, 0, 0 } };
}

RegexParser::RegexParser(
	const driverlist_t& drivers,
	const total_str_t& total,
	const queue_str_t& queue
) : StringsetParser(drivers),
    total(std::regex(total.first), total.second),
    queue(std::regex(queue.first), queue.second)
{
}

std::string RegexParser::ms(size_t n) {
	return std::ssub_match(ma[n]).str();
}

bool RegexParser::match_total(const std::string& key, size_t value, bool& rx, bool& bytes)
{
	// ignore blank REs
	if (total.first.mark_count() == 0) return false;

	// transform key to lower case
	std::string lower(key);
	std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

	auto found = std::regex_match(lower, ma, total.first);
	if (found) {
		auto& order = total.second;

		// extract direction and type
		auto direction = ms(order[0]);
		auto type = ms(order[1]);

		rx = (direction == "rx");
		bytes = (type == "bytes") || (type == "octets");
	}
	return found;
}

bool RegexParser::match_queue(const std::string& key, size_t value, bool& rx, bool& bytes, size_t& qnum)
{
	// ignore blank REs
	if (queue.first.mark_count() == 0) return false;

	// transform key to lower case
	std::string lower(key);
	std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

	auto found = std::regex_match(lower, ma, queue.first);
	if (found) {
		auto& order = queue.second;

		// extract direction and type
		auto direction = ms(order[0]);
		auto type = ms(order[1]);
		auto qstr = ms(order[2]);

		rx = (direction == "rx");
		bytes = (type == "bytes") || (type == "octets");
		qnum = std::stoi(qstr);
	}
	return found;
}
