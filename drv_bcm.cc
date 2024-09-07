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

#include "parser.h"

static RegexParser bnxt_en(
	{ "bnxt_en" },
	{ "^(rx|tx)_(bytes|[bum]cast_frames)$", { 1, 2 } },
	{ "^\\[(\\d+)\\]: (rx|tx)_(bytes|[bum]cast_packets)$", { 2, 3, 1 } }
);

static RegexParser bnx2(
	{ "bnx2" },
	{ "^(rx|tx)_(bytes|[bum]cast_packets)$", { 1, 2 } },
	{ "^\\[(\\d+)\\]: (rx|tx)_(bytes|[bum]cast_packets)$", { 2, 3, 1 } }
);

static RegexParser bnx2x(
	{ "bnx2x" },
	{ "^(rx|tx)_(bytes|[bum]cast_packets)$", { 1, 2 } },
	RegexParser::queue_nomatch()
);

static RegexParser tg3(
	{ "tg3" },
	{ "^(rx|tx)_(octets|[bum]cast_packets)$", { 1, 2 } },
	RegexParser::queue_nomatch()
);
