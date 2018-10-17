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

const auto bar = "------------";

const auto fmt_sssssss = "%5.5s %8.8s %8.8s %10.10s %10.10s %10.10s %10.10s\n";
const auto fmt_nnnnnff = "%5ld %8ld %8ld %10ld %10ld %10.3f %10.3f\n";
const auto fmt_snnnnff = "%5.5s %8ld %8ld %10ld %10ld %10.3f %10.3f\n";
const auto fmt_sssff = "%5.5s %8.8s %8.8s %10.3f %10.3f\n";

static double mbps(uint64_t n)
{
	return (n * 8) / 1e6;
};

void EthQApp::winmode_redraw()
{
	int y = 0;
	auto w = sub;

	auto nextline = [&]() { wmove(w, ++y, 0); };

	wclear(w);

	// show driver and interface name
	wmove(w, y, 0);
	wprintw(w, "%s", iface->info().c_str());

	// show current time
	wmove(w, y, 47);
	wprintw(w, " %s", timebuf);

	// skip line
	nextline();
	nextline();

	wprintw(w, fmt_sssssss, "Queue", "TX pkts", "RX pkts", "TX bytes", "RX bytes", "TX Mbps", "RX Mbps");
	nextline();

	wprintw(w, fmt_sssssss, bar, bar, bar, bar, bar, bar, bar);
	nextline();

	for (size_t i = 0, n = iface->queue_count(); i < n; ++i) {
		// skip quiescent queues
		if (!iface->queue_active(i)) continue;

		auto& q = iface->queue_stats(i).counts;
		wprintw(w, fmt_nnnnnff, i, q[0], q[1], q[2], q[3], mbps(q[2]), mbps(q[3]));
		nextline();
	}

	wprintw(w, fmt_sssssss, bar, bar, bar, bar, bar, bar, bar);
	nextline();

	auto& q = iface->total_stats().counts;
	wprintw(w, fmt_snnnnff, "Total", q[0], q[1], q[2], q[3], mbps(q[2]), mbps(q[3]));

	wrefresh(w);
}

void EthQApp::textmode_init()
{
	printf(fmt_sssssss, "q", "txp", "rxp", "txb", "rxb", "txmbps", "rxmbps");
}

void EthQApp::textmode_redraw()
{
	for (size_t i = 0, n = iface->queue_count(); i < n; ++i) {
		// skip quiescent queues
		if (!iface->queue_active(i)) continue;

		auto& q = iface->queue_stats(i).counts;
		printf(fmt_nnnnnff, i, q[0], q[1], q[2], q[3], mbps(q[2]), mbps(q[3]));
	}

	auto& q = iface->total_stats().counts;
	printf(fmt_snnnnff, "T", q[0], q[1], q[2], q[3], mbps(q[2]), mbps(q[3]));
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
