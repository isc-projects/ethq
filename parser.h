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
#include <map>

class StringsetParser {

public:
	typedef StringsetParser* ptr_t;
	typedef std::map<std::string, ptr_t> parsermap_t;

private:
	static parsermap_t parsers;

public:
	virtual bool match(const std::string& key, size_t value, size_t& queue, bool& rx, bool& bytes) = 0;

public:
	static ptr_t find(const std::string& driver) {
		auto iter = parsers.find(driver);
		if (iter != parsers.end()) {
			return iter->second;
		} else {
			return nullptr;
		}
	}
};
