# Firewall Rule Management System (C)

This repository contains two related Operating Systems projects:

1. **Firewall Rules Parser & Packet Checker**  
2. **Firewall Client–Server System**

---

## Project Overview

### Part 1: Firewall Rules Parser & Packet Checker
- Parses firewall rule files defining allowed IP and port ranges.
- Validates and sorts rules by IP address and port number.
- Checks whether a given packet (IP + port) should be accepted or rejected.
- Handles malformed input gracefully.

**Main files:**
- `annotated-readFirewall-2.c`
- `annotated-checkPacket.c`

---

### Part 2: Firewall Client–Server System
- Multi-threaded server that stores firewall rules and handles concurrent client requests.
- Clients can:
  - Add or delete rules.
  - Query whether an IP and port are accepted.
  - List stored rules and matched queries.
- The server ensures valid rule formats and manages a dynamic rule set safely.

**Main files:**
- `server.c`
- `client.c`

---

## Compilation & Execution

### Build the programs
```bash
make
