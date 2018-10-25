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
#include <array>

#include <getopt.h>
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
	std::vector<std::shared_ptr<Interface>>	ifaces;

private:	// time handling
	timespec		now;
	clockid_t		clock = CLOCK_REALTIME;
	char			timebuf[9];

	void			time_get();
	void			time_wait();

private:	// text mode handling
	void			textmode_redraw();

private:	// curses mode handling
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

	cerr << "usage: ethq [-g] [-t] <interface> [interface ...]" << endl;
	cerr << "  -g : attempt generic driver fallback" << endl;
	cerr << "  -t : use text mode" << endl;

	exit(status);
}

static std::array<size_t, 7> cols = { IFNAMSIZ, 8, 8, 10, 10, 10, 10 };

static std::string out_hdr(const std::array<std::string, cols.size()>& hdrs)
{
	using namespace std;

	ostringstream out;
	for (size_t n = 0; n < cols.size(); ++n) {
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
		out << setw(cols[n]) << fixed << setprecision(3);
		const auto& bps = q[n - 3];
		if (bps) {
			out << static_cast<uint64_t>(bps) * 8 / 1e6;
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
	static auto header = out_hdr({ "NIC", "TX pkts", "RX pkts", "TX bytes", "RX bytes", "TX Mbps", "RX Mbps" });
	auto w = stdscr;

	// output clamped to screen size
	auto wstr = [&](const std::string& s, bool pad = false) {
		auto maxx = getmaxx(w);
		auto maxy = getmaxy(w);
		auto curx = getcurx(w);
		auto cury = getcury(w);
		if (cury < maxy) {
			waddnstr(w, s.c_str(), maxx);
			if (pad) {
				while (curx++ < maxx) {
					waddch(w, ' ');
				}
			}
			wmove(w, cury + 1, 0);
		}
	};

	// reset screen
	werase(w);

	// show time and header
	wattron(w, A_REVERSE);
	wstr(header, true);
	mvwaddstr(w, 0, 2, timebuf);
	wattroff(w, A_REVERSE);
	wmove(w, 1, 0);

	for (auto& iface: ifaces) {
		// show totals
		wattron(w, A_BOLD);
		wstr(out_data(iface->name(), iface->total_stats()));
		wattroff(w, A_BOLD);

		// show per-queue data
		for (size_t i = 0, n = iface->queue_count(); i < n; ++i) {
			wstr(out_data(std::to_string(i), iface->queue_stats(i)));
		}
	}

	wrefresh(w);
}

void EthQApp::textmode_redraw()
{
	static auto header = out_hdr({ "nic", "txp", "rxp", "txb", "rxb", "txmbps", "rxmbps" });

	std::cout << header << std::endl;

	for (auto& iface: ifaces) {
		std::cout << out_data(iface->name(), iface->total_stats());
		std::cout << std::endl;
		for (size_t i = 0, n = iface->queue_count(); i < n; ++i) {
			std::cout << out_data(std::to_string(i), iface->queue_stats(i));
			std::cout << std::endl;
		}
	}

	std::cout << std::endl;
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
	strftime(timebuf, sizeof timebuf, "%T", gmtime(&now.tv_sec));
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
		for (auto& iface: ifaces) {
			iface->refresh();
		}

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
	bool generic = false;

	while ((opt = getopt(argc, argv, "ght")) != -1) {
		switch (opt) {
			case 'g':
				generic = true;
				break;
			case 't':
				winmode = false;
				break;
			case 'h':
				usage();
			default:
				usage(EXIT_FAILURE);
		}
	}

	// connect to the interface(s)
	while (optind < argc) {
		ifaces.emplace_back(std::make_shared<Interface>(argv[optind++], generic));
	}

	if (ifaces.size() == 0) {
		usage(EXIT_FAILURE);
	}

	// set up display mode
	if (winmode) {
		winmode_init();
	}
}

EthQApp::~EthQApp()
{
	if (winmode) {
		winmode_exit();
	}
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
