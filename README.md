EthQ
====

Displays an auto-updating per-second count of the number of packets
and bytes being handled by each queue on a multi-queue NIC.

Usage: `ethq <interface>`.

Requirements
------------

This software only runs on Linux.  It requires a C++11 compiler, the
NCurses library and the C++ bindings for NCurses.

The format of the names of the statistics values from a NIC is driver
specific.   The code currently supports the names used by the Intel
X540 and X710 drivers.
