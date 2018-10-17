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

#include <string>
#include <vector>
#include <map>

#include "ethtool++.h"
#include "parser.h"

class Interface {

public:
	//
	// the four combinations of rx/tx and packets/bytes are stored
	// in this union, so that they can either be addressed by index
	// or by name
	//
	typedef union {
		uint64_t		counts[4];
		struct {
			uint64_t	tx_packets;
			uint64_t	rx_packets;
			uint64_t	tx_bytes;
			uint64_t	rx_bytes;
		} q;
	} ifstats_t;

private:
	// index to queue table, offset to value within
	typedef std::pair<size_t, size_t> queue_entry_t;

	// string entry number -> queue_entry_t
	typedef std::map<size_t, queue_entry_t> queue_map_t;

private:
	std::string			name;
	Ethtool*			ethtool = nullptr;
	Ethtool::stats_t		state;

	ifstats_t			tstats;
	std::vector<ifstats_t>		qstats;
	std::vector<bool>		qactive;
	queue_map_t			qmap;

private:
	std::vector<std::string> 	get_string_names();
	void				build_queue_map(StringsetParser *parser);

public:
	Interface(const std::string& name);
	~Interface();

public:
	const std::string		info() const;
	void				refresh();

	size_t				queue_count() const;
	bool				queue_active(size_t n) const;
	const ifstats_t&		queue_stats(size_t n) const;
	const ifstats_t&		total_stats() const;
};