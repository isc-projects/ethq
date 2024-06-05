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

#include "parser.h"

class VMXNet3Parser : public StringsetParser {

private:
	std::regex	re1;
	std::regex	re2;
	std::smatch	ma;

	std::string     ms(size_t n) {
		return std::ssub_match(ma[n]).str();
	}

private:
	size_t		queue = 0;
	bool		rx = false;

public:
	VMXNet3Parser(const driverlist_t& drivers)
		: StringsetParser(drivers)
	{
		re1.assign("^(Rx|Tx) Queue#$");
		re2.assign("^\\s*[bum]cast (pkts|bytes) (rx|tx)$");
	}

	virtual ~VMXNet3Parser() = default;

	bool match_queue(const std::string& key, size_t value, bool& rx, bool& bytes, size_t& queue) {

		// check for match againt queue number
		if (std::regex_match(key, ma, re1)) {
			this->queue = value;
			this->rx = (ms(1) == "Rx");
			return false;
		}

		// check for data entry
		bool found = std::regex_match(key, ma, re2);
		if (found) {
			bytes = (ms(1) == "bytes");
			queue = this->queue;
			rx = this->rx;
		}

		return found;
	}
};

static VMXNet3Parser vmxnet3(
	{ "vmxnet3" }
);
