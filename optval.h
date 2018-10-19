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

#include <cstdint>
#include <stdexcept>

class OptVal {

	typedef uint64_t	value_t;

private:
	value_t			value;
	bool			set;

public:
	OptVal() :
		value(0), set(false)
	{
	}

	OptVal(value_t value) :
		value(value), set(true)
	{
	}

	void reset()
	{
		value = 0;
		set = false;
	}

	operator bool() const {
		return set;
	}

	operator uint64_t() const {
		if (set) {
			return value;
		} else {
			throw std::runtime_error("unset optional value unwrapped");
		}
	}

	value_t operator =(value_t rhs) {
		set = true;
		value = rhs;
		return value;
	}

	value_t operator +=(value_t rhs) {
		set = true;
		value += rhs;
		return value;
	}

	std::string to_string() const
	{
		if (set) {
			return std::to_string(value);
		} else {
			return "-";
		}
	}
};
