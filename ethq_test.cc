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
#include <iomanip>
#include <fstream>
#include <stdexcept>
#include <regex>

#include "parser.h"
#include "util.h"

void test(StringsetParser* parser, std::istream& is)
{
	size_t lineno = 0;

	std::regex re("^\\s*(.*?): (\\d+)$");
	std::smatch ma;

	while (!is.eof()) {

		size_t queue = 0;
		bool rx, bytes;

		// parse input
		std::string line;
		std::getline(is, line);

		bool valid = std::regex_match(line, ma, re);
		if (!valid) continue;

		std::string key = std::ssub_match(ma[1]).str();
		size_t value = std::stoull(std::ssub_match(ma[2]).str());

		// test the input
		bool match = parser->match(key, value, queue, rx, bytes);

		// generate output
		std::cout << std::setw(3) << lineno++ << " | ";
		if (match) {
			std::cout << std::setw(3) << queue;
			std::cout << " " << (rx ? "RX" : "TX");
			std::cout << " " << (bytes ? "B" : "P");
			std::cout << " ";
		} else {
			std::cout << "         ";
		}
		std::cout << "| " << line << std::endl;
	}
}

int main(int argc, char *argv[])
{
	// parse command line args
	if (argc < 2 || argc > 3) {
		std::cerr << "usage: parsetest <driver> [infile]" << std::endl;
		return EXIT_FAILURE;
	}

	std::string driver = argv[1];
	std::string infile = (argc == 3) ? argv[2] : "-";

	try {
		auto parser = StringsetParser::find(driver);
		if (!parser) {
			throw std::runtime_error("couldn't find specified driver");
		}

		if (infile == "-") {
			test(parser, std::cin);
		} else {
			auto input = std::ifstream(infile);
			if (input.fail()) {
				throw_errno("file open");
			}
			test(parser, input);
		}

	} catch (const std::exception& e) {
		std::cerr << "error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}
