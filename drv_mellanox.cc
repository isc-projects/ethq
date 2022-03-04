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

static RegexParser mellanox_mlx5_core( 
	{ "mlx5_core", "mlx4_en" },
	{ "^(rx|tx)_(bytes|packets)$", { 1, 2} },
	{ "^(rx|tx)(\\d+)_(?:0_)?(bytes|packets)$", { 1, 3, 2 } }
);
