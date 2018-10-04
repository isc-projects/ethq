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

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <vector>
#include <map>
#include <algorithm>

#include <getopt.h>
#include <time.h>
#include <poll.h>
#include <ncurses.h>

#include "ethtool++.h"
#include "parser.h"
#include "util.h"

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
} queuestats_t;

typedef std::vector<queuestats_t>		stats_table_t;

// index to queue table, offset to value within
typedef std::pair<size_t, size_t>		queue_entry_t;

// string entry number -> queue_entry_t
typedef std::map<size_t, queue_entry_t>		queue_map_t;

//
// main application wrapper class
//
class EthQApp {

private:	// command line parameters
	std::string		ifname;
	bool			winmode = true;

private:	// network state
	Ethtool*		ethtool = nullptr;
	size_t			qcount = 0;
	queue_map_t		qmap;
	stats_table_t		prev;
	stats_table_t		delta;
	queuestats_t		total;

	std::vector<std::string> get_string_names();
	void			build_queue_map(StringsetParser *parser, const Ethtool::stringset_t& names);

	stats_table_t		get_stats();
	void			get_deltas();

private:	// time handling
	timespec		now;
	clockid_t		clock = CLOCK_REALTIME;
	char			timebuf[26];

	void			time_get();
	void			time_wait();

private:	// text mode handling
	void			textmode_init();
	void			textmode_redraw();

private:	// curses mode handling
	WINDOW*			sub;
	void			winmode_redraw();
	void			winmode_init();
	bool			winmode_should_exit();
	void			winmode_exit();

public:
				EthQApp(int argc, char *argv[]);
				~EthQApp();

	void			run();
};

void EthQApp::build_queue_map(StringsetParser* parser, const Ethtool::stringset_t& names)
{
	// some drivers (e.g. vmnet3) store the queue number in a key-value pair
	auto raw = ethtool->stats();

	size_t queue = -1;
	auto rx = false;
	auto bytes = false;

	//
	// iterate through all of the stats looking for names that
	// match the recognised strings
	//
	for (size_t i = 0, n = names.size(); i < n; ++i) {

		//
		// try to map the stringset entry to a queue
		//
		bool found = parser->match(names[i], raw[i], queue, rx, bytes);

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
}

//
// return the latest (absolute) counters for the four values for each queue
//
stats_table_t EthQApp::get_stats()
{
	stats_table_t results(qcount);

	auto raw = ethtool->stats();

	for (const auto& pair: qmap) {
		auto id = pair.first;
		auto& entry = pair.second;
		auto queue = entry.first;
		auto offset = entry.second;

		results[queue].counts[offset] = raw[id];
	}

	return results;
}

//
// get the latest counters, calculate difference from the previous set,
// and then store the latest values for next time around
//
void EthQApp::get_deltas()
{
	stats_table_t stats = get_stats();

	for (size_t j = 0; j < 4; ++j) {
		total.counts[j] = 0;
	}

	for (size_t i = 0; i < qcount; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			int64_t d = stats[i].counts[j] - prev[i].counts[j];
			delta[i].counts[j] = d;
			total.counts[j] += d;
		}
	}

	std::swap(stats, prev);
}

static void usage(int status = EXIT_SUCCESS)
{
	using namespace std;

	cerr << "usage: ethq [-t] <interface>" << endl;

	exit(status);
}

const auto bar = "------------";

const auto fmt_sssss = "%5.5s %12s %12s %12s %12s\n";
const auto fmt_nnnnn = "%5ld %12ld %12ld %12ld %12ld\n";
const auto fmt_snnnn = "%5.5s %12ld %12ld %12ld %12ld\n";
const auto fmt_sssff = "%5.5s %12s %12s %12.3f %12.3f\n";

void EthQApp::winmode_redraw()
{

	int y = 0;
	auto w = sub;

	auto nextline = [&]() { wmove(w, ++y, 0); };

	wclear(w);

	// show driver and interface name
	wmove(w, y, 0);
	wprintw(w, "%s:%s", ethtool->driver().c_str(), ifname.c_str());

	// show current time
	wmove(w, y, 37);
	wprintw(w, " %s", timebuf);

	// skip line
	nextline();
	nextline();

	wprintw(w, fmt_sssss, "Queue", "TX packets", "RX packets", "TX bytes", "RX bytes");
	nextline();

	wprintw(w, fmt_sssss, bar, bar, bar, bar, bar);
	nextline();

	for (size_t i = 0; i < qcount; ++i) {
		// skip quiescent queues
		auto& p = prev[i].counts;
		if (p[0] == 0 && p[1] == 0) continue;

		auto& q = delta[i].counts;
		wprintw(w, fmt_nnnnn, i, q[0], q[1], q[2], q[3]);
		nextline();
	}

	wprintw(w, fmt_sssss, bar, bar, bar, bar, bar);
	nextline();

	auto& q = total.counts;
	wprintw(w, fmt_snnnn, "Total", q[0], q[1], q[2], q[3]);
	nextline();

	wprintw(w, fmt_sssff, "Mbps", "", "", 8.0 * q[2] / 1e6, 8.0 * q[3] / 1e6);

	wrefresh(w);
}

void EthQApp::textmode_init()
{
	printf(fmt_sssss, "q", "txp", "rxp", "txb", "rxb");
}

void EthQApp::textmode_redraw()
{
	for (size_t i = 0; i < qcount; ++i) {
		// skip quiescent queues
		auto& p = prev[i].counts;
		if (p[0] == 0 && p[1] == 0) continue;

		auto& q = delta[i].counts;
		printf(fmt_nnnnn, i, q[0], q[1], q[2], q[3]);
	}

	auto& q = total.counts;
	printf(fmt_snnnn, "T", q[0], q[1], q[2], q[3]);
}

void EthQApp::time_get()
{
	clock_gettime(clock, &now);
}

void EthQApp::time_wait()
{
	now.tv_nsec = 0;
	now.tv_sec += 1;

	while (true) {
		auto res = clock_nanosleep(clock, TIMER_ABSTIME, &now, nullptr);
		if (res == 0) {
			break;
		} else if (res == EINTR) {
			continue;
		} else {
			errno = res;
			throw_errno("clock_nanosleep");
		}
	}
	strftime(timebuf, sizeof timebuf, "%Y-%m-%d %T", gmtime(&now.tv_sec));
}

bool EthQApp::winmode_should_exit()
{
	auto ch = ::getch();
	return (ch == 'q' || ch == 'Q');
}

void EthQApp::winmode_init()
{
	initscr();
	cbreak();
	noecho();
	nonl();
	nodelay(stdscr, TRUE);
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
	curs_set(0);

	sub = newwin(qcount + 8, 58, 1, 1);
}

void EthQApp::winmode_exit()
{
	endwin();
}

void EthQApp::run()
{
	prev = get_stats();
	time_get();

	while (true) {
		if (winmode && winmode_should_exit()) break;
		time_wait();
		get_deltas();

		if (winmode) {
			winmode_redraw();
		} else {
			textmode_redraw();
		}
	}
}

EthQApp::EthQApp(int argc, char *argv[]) : qcount(0)
{
	int opt;

	while ((opt = getopt(argc, argv, "th")) != -1) {
		switch (opt) {
			case 't':
				winmode = false;
				break;
			case 'h':
				usage();
			default:
				usage(EXIT_FAILURE);
		}
	}

	if (optind != (argc - 1)) {
		usage(EXIT_FAILURE);
	}

	// connect to the ethtool API
	ifname.assign(argv[optind]);
	ethtool = new Ethtool(ifname);

	// find the write code to parse this class of NIC
	auto parser = StringsetParser::find(ethtool->driver());
	if (!parser) {
		throw std::runtime_error("Unsupported NIC driver");
	}

	// parse the list of stats strings for this driver
	build_queue_map(parser, ethtool->stringset(ETH_SS_STATS));
	if (qcount == 0) {
		throw std::runtime_error("couldn't parse NIC stats");
	}
	delta.reserve(qcount);

	// set up display mode
	if (winmode) {
		winmode_init();
	} else {
		textmode_init();
	}
}

EthQApp::~EthQApp()
{
	if (winmode) {
		winmode_exit();
	}

	delete ethtool;
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");
	int res = EXIT_SUCCESS;

	try {
		EthQApp app(argc, argv);
		app.run();
	} catch (const std::exception& e) {
		std::cerr << "error: " << e.what() << std::endl;
		res = EXIT_FAILURE;
	}

	return res;
}
