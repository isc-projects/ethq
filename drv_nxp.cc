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

static RegexParser nxp_daap2(
	{ "fsl_dpaa2_eth" },
	{ "^\\[hw\\] (rx|tx) (bytes|frames)$", { 1, 2 } },
	RegexParser::queue_nomatch()
);
