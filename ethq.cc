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
#include <sstream>
#include <iomanip>
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

#include "interface.h"
#include "util.h"

//
// main application wrapper class
//
class EthQApp {

private:	// command line parameters
	bool			winmode = true;

private:	// network state
	Interface*		iface = nullptr;

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

static void usage(int status = EXIT_SUCCESS)
{
	using namespace std;

	cerr << "usage: ethq [-t] <interface>" << endl;

	exit(status);
}

static std::array<size_t, 7> cols = { 5, 8, 8, 10, 10, 10, 10 };

static std::string out_bars()
{
	using namespace std;

	ostringstream out;
	for (size_t n = 0; n < cols.size(); ++n) {
		for (size_t i = 0; i < cols[n]; ++i) {
			out << "-";
		}
		if (n != cols.size() - 1) {
			out << " ";
		}
	}

	return out.str();
}

static std::string out_hdr(const std::array<std::string, 7>& hdrs)
{
	using namespace std;

	ostringstream out;
	for (size_t n = 0; n < 7; ++n) {
		out << setw(cols[n]) << hdrs[n];
		if (n != cols.size() - 1) {
			out << " ";
		}
	}

	return out.str();
}

static std::string out_data(const std::string& label, const Interface::ifstats_t& stats)
{
	using namespace std;

	ostringstream out;
	out << setw(cols[0]) << label << " ";

	auto& q = stats.counts;
	for (size_t n = 1; n < 5; ++n) {
		out << setw(cols[n]) << q[n - 1].to_string() << " ";
	}

	for (size_t n = 5; n < 7; ++n) {
		out << setw(cols[n]);
		const auto& bps = q[n - 3];
		if (bps) {
			auto mbps = static_cast<uint64_t>(bps) * 8 / 1e6;
			out << fixed << setprecision(3) << mbps;
		} else {
			out << "-";
		}
		if (n != cols.size() - 1) {
			out << " ";
		}
	}

	return out.str();
}

void EthQApp::winmode_redraw()
{
	static auto header = out_hdr({ "Queue", "TX pkts", "RX pkts", "TX bytes", "RX bytes", "TX Mbps", "RX Mbps" });
	static auto bars = out_bars();

	int y = 0;
	auto w = sub;

	auto nextline = [&]() { wmove(w, ++y, 0); };

	wclear(w);

	// show driver and interface name
	wmove(w, y, 0);
	waddstr(w, iface->info().c_str());

	// show current time and skip a line
	wmove(w, y, 47);
	waddch(w, ' ');
	waddstr(w, timebuf);
	nextline(); nextline();

	// show header and bars
	waddstr(w, header.c_str()); nextline();
	waddstr(w, bars.c_str()); nextline();

	// show queue data
	for (size_t i = 0, n = iface->queue_count(); i < n; ++i) {
		const auto& out = out_data(std::to_string(i), iface->queue_stats(i));
		waddstr(w, out.c_str()); nextline();
	}

	// show bars
	waddstr(w, bars.c_str()); nextline();

	// show totals
	const auto& out = out_data("Total", iface->total_stats());
	waddstr(w, out.c_str());

	wrefresh(w);
}

void EthQApp::textmode_init()
{
	static auto header = out_hdr({ "q", "txp", "rxp", "txb", "rxb", "txmbps", "rxmbps" });
	puts(header.c_str());
}

void EthQApp::textmode_redraw()
{
	for (size_t i = 0, n = iface->queue_count(); i < n; ++i) {
		auto out = out_data(std::to_string(i), iface->queue_stats(i));
		puts(out.c_str());
	}

	auto out = out_data("Total", iface->total_stats());
	puts(out.c_str());
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

	sub = newwin(iface->queue_count() + 8, 67, 1, 1);
}

void EthQApp::winmode_exit()
{
	endwin();
}

void EthQApp::run()
{
	time_get();

	while (true) {
		if (winmode && winmode_should_exit()) break;
		time_wait();
		iface->refresh();

		if (winmode) {
			winmode_redraw();
		} else {
			textmode_redraw();
		}
	}
}

EthQApp::EthQApp(int argc, char *argv[])
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

	// connect to the interface
	iface = new Interface(argv[optind]);

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

	delete iface;
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
