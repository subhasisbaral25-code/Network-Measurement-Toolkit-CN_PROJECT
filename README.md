# Network-Measurement-Toolkit-CN_PROJECT
 This repository contains a custom network measurement system built entirely from scratch in C for CS-30003 Coding Assignment 1. Operating directly at the network and transport layers, this toolkit bypasses standard OS networking libraries to manually construct ICMP/UDP headers and execute internet checksums over raw sockets.

## Setup and Requirements
Because this toolkit utilizes raw sockets, all modules strictly require administrator privileges (`sudo`) to execute on Linux. 

## Features and Usage
### 1) Rupen's Ping module part:
**Description:** A custom ping implementation built directly over raw sockets. It bypasses OS libraries to manually construct ICMP Echo Request headers, calculates RFC 1071 internet checksums, and mathematically tracks Round Trip Time (RTT), network jitter, and packet loss percentages.

**Compilation:**    
`gcc ping.c -o ping`

**Execution:** 
`sudo ./ping <Target_IP>`
-------------------------

### 2) Subhasis's Traceroute module part:
**Description:** A custom traceroute implementation built directly over raw sockets that handles both UDP and ICMP packet crafting. It manually manipulates IP TTL headers to trace the hop sequence and implements strict probe-reply correlation by parsing nested ICMP error messages.

**Compilation:**    
`gcc traceroute.c -o traceroute`

**Execution:** 
`sudo ./traceroute <Target_IP> <-i|-u>`
-------------------------

### 3) Diptalal Pradhan's PMTU and Bandwidth Estimation modules:

**Description:** Implements IPv4 Path MTU Discovery using raw sockets, ICMP probes, the Don't Fragment (DF) flag, and binary search to estimate the largest packet size that can traverse a network path without fragmentation. It also estimates network bandwidth using packet-pair dispersion by measuring the time difference between corresponding ICMP replies.

#### **A) Path MTU Discovery (PMTU)**

**Compilation:**

```bash
gcc -Wall -Wextra -Wpedantic pmtu/pmtu.c -o pmtu_tool
```

**Execution:**

```bash
sudo ./pmtu_tool 1.1.1.1
```

**Documentation:** For more details, refer to the [PMTU README](pmtu/README.md).

#### **B) Bandwidth Estimation**

**Compilation:**

```bash
gcc -Wall -Wextra -Wpedantic bandwidth/bandwidth.c -o bandwidth_tool
```

**Execution:**

```bash
sudo ./bandwidth_tool <Target_IP>
```

**Documentation:** For more details, refer to the [Bandwidth Estimation README](bandwidth/README.md).

**-------------------------**
