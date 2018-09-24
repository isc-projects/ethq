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

#pragma once

#include <vector>
#include <string>

#include <net/if.h>
#include <linux/ethtool.h>

class Ethtool {

public:
	typedef std::vector<std::string> stringset_t;
	typedef std::vector<__u64> stats_t;

private:
	int			fd;
	ifreq			ifr;
	ssize_t			stats_size;

private:
	void			ioctl(void *data);

public:
				Ethtool(const std::string& ifname);
				~Ethtool();

public:
	size_t			stringset_size(ethtool_stringset ss);
	stringset_t		stringset(ethtool_stringset);
	stats_t			stats();

};
