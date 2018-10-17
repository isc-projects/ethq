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

#include <stdexcept>
#include "interface.h"
#include "parser.h"

Interface::Interface(const std::string& name)
	: name(name)
{
	ethtool = new Ethtool(name);
	state = ethtool->stats();

	// find the right code to parse this NIC's stats output
	auto parser = StringsetParser::find(ethtool->driver());
	if (!parser) {
		throw std::runtime_error("Unsupported NIC driver");
	}

	// parse the list of stats strings
	build_queue_map(parser);
	if (qstats.size() == 0) {
		throw std::runtime_error("couldn't parse NIC stats");
	}
}

Interface::~Interface()
{
	delete ethtool;
}

const std::string Interface::info() const
{
	return ethtool->driver() + ":" + name;
}

void Interface::refresh()
{
	auto stats = ethtool->stats();

	// reset totals
	tstats.counts[0] = 0;
	tstats.counts[1] = 0;
	tstats.counts[2] = 0;
	tstats.counts[3] = 0;

	// reset active queue list
	for (size_t i = 0, n = qstats.size(); i < n; ++i) {
		qactive[i] = false;
	}

	for (const auto& pair: qmap) {

		auto index = pair.first;
		const auto& entry = pair.second;
		auto queue = entry.first;
		auto offset = entry.second;

		uint64_t prev = state[index];
		uint64_t current = stats[index];

		// record the difference in value
		uint64_t delta = (current > prev) ? (current - prev) : 0;
		qstats[queue].counts[offset] = delta;
		tstats.counts[offset] += delta;

		// tag active queues
		if (delta) {
			qactive[queue] = true;
		}
	}

	std::swap(stats, state);
}

size_t Interface::queue_count() const
{
	return qstats.size();
}

bool Interface::queue_active(size_t n) const
{
	return qactive[n];
}

const Interface::ifstats_t& Interface::queue_stats(size_t n) const
{
	return qstats[n];
}

const Interface::ifstats_t& Interface::total_stats() const
{
	return tstats;
}

void Interface::build_queue_map(StringsetParser* parser)
{
	size_t qcount = 0;
	auto names = ethtool->stringset(ETH_SS_STATS);

	//
	// iterate through all of the stats looking for names that
	// match the recognised strings
	//
	for (size_t i = 0, n = names.size(); i < n; ++i) {

		size_t queue = -1;
		auto rx = false;
		auto bytes = false;

		//
		// try to map the stringset entry to a queue - pass the initially
		// read value too, for those drivers (e.g. vmxnet3) that store the
		// queue number if a key-value pair
		//
		bool found = parser->match(names[i], state[i], queue, rx, bytes);

		//
		// remember the individual rows that make up the four stats
		// values for each NIC queue
		//
		if (found) {

			// calculate offset into the four entry structure
			auto offset = static_cast<size_t>(rx) + 2 * static_cast<size_t>(bytes);

			// and populate it
			qmap[i] = queue_entry_t { queue, offset };

			// count the number of queues
			qcount = std::max(queue + 1, qcount);
		}
	}

	qstats.resize(qcount);
	qactive.resize(qcount);
}
