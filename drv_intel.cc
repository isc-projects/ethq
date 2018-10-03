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

static RE_DNT_Parser intel_generic(
	{ "ixgbe", "igb" },
	"^(rx|tx)_queue_(\\d+)_(bytes|packets)$"
);

static RE_DNT_Parser intel_i40e(
	{ "i40e" },
	"^(rx|tx)-(\\d+)\\.\\1_(bytes|packets)$"
);
