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

Interface::Interface(const std::string& name, bool generic)
	: _name(name)
{
	ethtool = new Ethtool(name);
	state = ethtool->stats();

	// find the right code to parse this NIC's stats output
	auto driver = ethtool->driver();
	const auto info = driver + ":" + name;

	auto parser = StringsetParser::find(driver);
	if (!parser) {
		if (!generic) {
			throw std::runtime_error("Unsupported NIC driver " + info);
		}

		parser = StringsetParser::find("generic");
		if (!generic) {
			throw std::runtime_error("Failed fallback from " + info + " to generic");
		}
	}

	// parse the list of stats strings
	build_stats_map(parser);
	if (tmap.size() == 0 && qmap.size() == 0) {
		throw std::runtime_error("couldn't parse NIC stats for " + info);
	}
}

Interface::~Interface()
{
	delete ethtool;
}

const std::string Interface::name() const
{
	return _name;
}

void Interface::refresh()
{
	auto stats = ethtool->stats();

	// reset total counters
	for (size_t i = 0; i < 4; ++i) {
		tstats.counts[i].reset();
	}

	// reset queue counters
	for (auto& stats: qstats) {
		for (size_t i = 0; i < 4; ++i) {
			stats.counts[i].reset();
		}
	}

	// accumulate total counters
	for (const auto& pair: tmap) {

		auto index = pair.first;
		auto offset = pair.second;

		uint64_t prev = state[index];
		uint64_t current = stats[index];

		// record the difference in value
		uint64_t delta = (current > prev) ? (current - prev) : 0;
		tstats.counts[offset] += delta;
	}

	// accumulate queue counters
	for (const auto& pair: qmap) {

		auto index = pair.first;
		const auto& entry = pair.second;
		auto queue = entry.first;
		auto offset = entry.second;

		uint64_t prev = state[index];
		uint64_t current = stats[index];

		// record the difference in value
		uint64_t delta = (current > prev) ? (current - prev) : 0;
		qstats[queue].counts[offset] += delta;

		// auto-copy into the total if there's no explicit map of total fields
		if (tmap.size() == 0) {
			tstats.counts[offset] += delta;
		}
	}

	std::swap(stats, state);
}

size_t Interface::queue_count() const
{
	return qstats.size();
}

const Interface::ifstats_t& Interface::queue_stats(size_t n) const
{
	return qstats[n];
}

const Interface::ifstats_t& Interface::total_stats() const
{
	return tstats;
}

size_t get_offset(bool rx, bool bytes)
{
	return rx + 2 * bytes;
}

void Interface::build_stats_map(StringsetParser* parser)
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
		// try to map the stringset entry to a NIC total
		//
		bool total_found = parser->match_total(names[i], state[i], rx, bytes);
		if (total_found) {
			// save offset into the four entry structure
			tmap[i] = get_offset(rx, bytes);
		}

		//
		// try to map the stringset entry to a queue - pass the initially
		// read value too, for those drivers (e.g. vmxnet3) that store the
		// queue number in a key-value pair
		//
		bool queue_found = parser->match_queue(names[i], state[i], rx, bytes, queue);

		//
		// remember the individual rows that make up the four stats
		// values for each NIC queue
		//
		if (queue_found && (state[i] > 0)) {	// ignore zero-counters

			// calculate offset into the four entry structure
			auto offset = get_offset(rx, bytes);

			// and populate it
			qmap[i] = queue_entry_t { queue, offset };

			// count the number of queues
			qcount = std::max(queue + 1, qcount);
		}
	}

	qstats.resize(qcount);
}
