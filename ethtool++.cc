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

#include <algorithm>
#include <cstring>

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <linux/sockios.h>

#include "ethtool++.h"
#include "util.h"

void Ethtool::ioctl(void* data)
{
	ifr.ifr_data = reinterpret_cast<char*>(data);
	if (::ioctl(fd, SIOCETHTOOL, &ifr, sizeof(ifr)) < 0) {
		throw_errno("ioctl(SIOCETHTOOL)");
	}
}

size_t Ethtool::stringset_size(ethtool_stringset ss)
{
	// memoize the table
	auto iter = sizes.find(ss);
	if (iter != sizes.end()) {
		return iter->second;
	}

	// allocate memory on stack
	auto size = sizeof(ethtool_sset_info) + sizeof(__u32);
	auto p = reinterpret_cast<char*>(alloca(size));
	if (!p) {
		throw_errno("alloca");
	}
	std::fill(p, p + size, 0);

	// shadow the allocation
	auto& sset_info = *reinterpret_cast<ethtool_sset_info*>(p);

	// get the data
	sset_info.cmd = ETHTOOL_GSSET_INFO;
	sset_info.reserved = 0;
	sset_info.sset_mask = (1 << ss);
	ioctl(&sset_info);

	auto result = sset_info.data[0];
	sizes[ss] = result;

	return result;
}

Ethtool::stringset_t Ethtool::stringset(ethtool_stringset ss)
{
	// determine size
	size_t count = stringset_size(ss);

	// allocate sufficient memory on stack
	auto size = sizeof(ethtool_gstrings) + count * ETH_GSTRING_LEN;
	auto p = reinterpret_cast<char*>(alloca(size));
	if (!p) {
		throw_errno("alloca");
	}
	std::fill(p, p + size, 0);

	// shadow the allocation
	auto& gstrings = *reinterpret_cast<ethtool_gstrings*>(p);

	// get the data
	gstrings.cmd = ETHTOOL_GSTRINGS;
	gstrings.string_set = ss;
	ioctl(&gstrings);

	// build the result set
	stringset_t result;
	result.reserve(count);
	for (unsigned int i = 0; i < gstrings.len; ++i) {
		auto p = reinterpret_cast<char *>(gstrings.data) + i * ETH_GSTRING_LEN;
		result.emplace_back(p);		// assume NUL terminated
	}

	return result;
}

Ethtool::stats_t Ethtool::stats()
{
	size_t count = stringset_size(ETH_SS_STATS);

	// allocate memory on stack
	auto size = sizeof(ethtool_stats) + count * sizeof(__u64);
	auto p = reinterpret_cast<char *>(alloca(size));
	if (!p) {
		throw_errno("alloca");
	}
	std::fill(p, p + size, 0);

	// shadow the allocation
	auto& stats = *reinterpret_cast<ethtool_stats*>(p);

	// get the data
	stats.cmd = ETHTOOL_GSTATS;
	ioctl(&stats);

	// build the result set
	stats_t result(count);
	std::copy(stats.data, stats.data + stats.n_stats, result.begin());

	return result;
}

Ethtool::Ethtool(const std::string& ifname)
{
	fd = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		throw_errno("socket");
	}

	stpncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ);

	// retrieve the driver information
	drvinfo.cmd = ETHTOOL_GDRVINFO;
	ioctl(&drvinfo);
}

Ethtool::~Ethtool()
{
	::close(fd);
}
