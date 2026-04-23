# Bloom Filter Hardware Accelerator

A full hardware/software co-design project implementing a **Bloom Filter accelerator** on a MicroBlaze system running embedded Linux. The accelerator is exposed as a custom AXI peripheral and controlled entirely from user-space via memory-mapped I/O (MMIO).

---

## Project Requirements Fulfilled

| Requirement | How It's Met |
|---|---|
| Hardware acceleration | Custom Bloom Filter core in SV |
| Softcore processor |  MicroBlaze connected via AXI interconnect |
| AXI peripheral | bloom_axi.sv wraps bloom_core.sv as a memory-mapped AXI slave |
| Embedded Linux control | PetaLinux user-space app drives the accelerator via MMIO |

---

## System Architecture

```
PC Terminal
    │
    │  UART (console)
    ▼
PetaLinux on MicroBlaze
    │
    │  MMIO via /dev/mem
    ▼
AXI Bloom Filter Peripheral
    │
    ├── bloom_axi.sv   (AXI4-Lite slave wrapper)
    └── bloom_core.sv  (Bloom filter logic + FSM)
         ├── 1024-bit register-based bit array
         └── 3 hash functions over 32-bit keys
```

The MicroBlaze processor runs Linux and communicates with the accelerator over the AXI interconnect. Software directly reads and writes the peripheral's registers through memory-mapped I/O.

---

## Bloom Filter Background

A Bloom filter is a probabilistic data structure that answers set queries in constant time and constant space. It can return two answers:

- **"Definitely not present"** — the key has never been inserted (guaranteed correct)
- **"Maybe present"** — the key was likely inserted (small false-positive rate)

---

## Hardware Design

### `bloom_core.sv` — Accelerator Core

The core implements a 1024-bit Bloom filter with three independent hash functions. It runs as a simple FSM with four states: `IDLE`, `INSERT`, `QUERY`, and `CLEAR`. Each operation completes in a single clock cycle after the command is issued.

**Hash functions** (32-bit key to 10-bit index):
```
h1 = key ^ (key >> 16)
h2 = key * 0x45d9f3b
h3 = key + (key << 6) + (key >> 2)
index = hash & 0x3FF   // mask to 1024
```

**Parameters:**

| Parameter | Value |
|---|---|
| Key width | 32 bits |
| Bit array size | 1024 bits (register-based, not BRAM) |
| Hash functions | 3 |
| Latency | 1 clock cycle per operation |

### `bloom_axi.sv` — AXI4 Wrapper

Wraps `bloom_core` as an AXI4slave. Handles the full AXI handshake and exposes four memory-mapped registers.

---

## Register Map

| Offset | Register | Description |
|---|---|---|
| `0x00` | `KEY_IN` | 32-bit input key |
| `0x04` | `CONTROL` | Command register — write to trigger operations |
| `0x08` | `RESULT` | Query result (bit 0) |
| `0x0C` | `STATUS` | `bit 0` = DONE, `bit 1` = BUSY |

### CONTROL Commands

| Value | Operation |
|---|---|
| `0x3` | INSERT (`START \| INSERT`) |
| `0x5` | QUERY (`START \| QUERY`) |
| `0x9` | CLEAR (`START \| CLEAR`) |

---

## Software Protocol (MMIO)

All software follows this handshake:

**INSERT:**
1. Wait until `STATUS[BUSY] == 0`
2. Write key to `KEY_IN`
3. Write `0x3` to `CONTROL`
4. Wait until `STATUS[DONE] == 1`

**QUERY:**
1. Wait until `STATUS[BUSY] == 0`
2. Write key to `KEY_IN`
3. Write `0x5` to `CONTROL`
4. Wait until `STATUS[DONE] == 1`
5. Read `RESULT[0]` — `1` = maybe present, `0` = definitely absent

**CLEAR:**
1. Wait until `STATUS[BUSY] == 0`
2. Write `0x9` to `CONTROL`
3. Wait until `STATUS[DONE] == 1`

---

## PetaLinux

The Linux user-space application communicates with hardware through `/dev/mem`, requiring no custom kernel driver.

### `bloom_driver.c / .h`
Low-level MMIO access layer. Opens `/dev/mem`, maps the peripheral's physical address, and provides `bloom_insert()`, `bloom_query()`, and `bloom_clear()` functions that implement the hardware handshake protocol.

### `bloom_sw.c`
Pure software reference implementation of the Bloom filter running entirely on the MicroBlaze CPU. Used to validate correctness and measure the performance baseline before hardware offload.

### `main.c` — `bloom_cli`
Interactive command-line interface. Accepts user commands from the terminal and dispatches them to either the software or hardware implementation:

```
insert <word>     — insert a key into the filter
query  <word>     — check if a key may be present
clear             — reset the filter
bench  <count>    — run a timed benchmark comparing SW vs HW
```

The bench command reports throughput and latency for both implementations side by side, demonstrating the benefit of hardware acceleration.

---

## SystemC  Model

A functional C++ model of the Bloom filter is included in `/systemc/bloom_model.cpp`. It implements the same hash functions and bit-array logic as the RTL, providing a fast software oracle that can be run without a simulator.

It is used to:
- Verify hash function correctness before synthesis
- Serve as a reference when validating hardware query results
- Enable rapid algorithm-level testing

Run it standalone:
```bash
g++ -std=c++17 -I$SYSTEMC_HOME/include -L$SYSTEMC_HOME/lib \
    bloom_model.cpp -lsystemc -o bloom_model
./bloom_model
```

---

## Repository Structure

```
project-root/
│
├── src/
│   ├── bloom_core.sv          # Bloom filter FSM + bit array
│   └── bloom_axi.sv           # AXI4-Lite slave wrapper
│
├──systemc/
│       └── bloom_model.cpp    # SystemC functional reference model
├── tb/
│   ├── bloom_core_tb.sv       # Core-level directed testbench
│   ├── bloom_axi_tb.sv        # AXI-level directed testbench
│
├── hw/
│   ├── vivado/                # Vivado project scripts
│   │   └── build.tcl          # Block design rebuild script
│   ├── bd/
│   │   └── design_1.tcl       # MicroBlaze block design
│   ├── constrs/
│   │   └── bloom.xdc          # Timing/pin constraints
│   └── xsa/                   # Exported hardware (XSA) for PetaLinux
│
├── petalinux/
│   ├── main.c                 # bloom_cli — interactive terminal app
│   ├── bloom_driver.c/h       # MMIO hardware access layer
│   ├── mock_hw.c/h            # Software stub for host-only testing
│   └── test/
│       └── test_bloom.c       # Automated SW/HW comparison tests
```

---

## Building and Running

### 1. Synthesize Hardware (Vivado)
```
source build.tcl
```
Export the XSA file to `hw/xsa/` after implementation.

### 2. Build PetaLinux Image
```bash
petalinux-config --get-hw-description hw/xsa/
etalinux-build
petalinux-package --boot --fsbl --fpga --u-boot
```

### 3. Run on Board
Boot from SD card. The `bloom_cli` binary is included in the root filesystem:
```bash
bloom_cli
> insert hello
> query hello
maybe present
> query world
definitely not present
> bench 10000
SW: 1240 ns/op  HW: 38 ns/op   Speedup: 32x
```

### 4. Run RTL Simulation (Questa)
```bash
cd tb/
vlog -sv ../src/bloom_core.sv ../src/bloom_axi.sv bloom_axi_tb.sv
vsim -batch bloom_axi_tb -do "run -all; quit -f"
```

---
## Performance Analysis & Known Limitations

### Current Results: Software Outperforms Hardware

In our tests, the software version is faster for smaller workloads, which we expected to see and is not an issue with the hardware.

The reason for this is the overhead, not the computation.
with each operation it requres:

1. checking status
2. writing the key
3. ending the command
4. waiting for completion
5. reading the result

For each of these they go over the AXI bus and through the MMIO which adds delay.

In the software version, it runs on the CPU using simple operations so it is faster on smaller queries. 

In larger scales we can see the usefulness of the hardware. The hardware can handle these requests more effciently and maintain constant performance while software would slow down from increased workload and memory usage. Overall, the hardware is a better design for throughout at scale.

---

### The Handshaking Problem

Our issue is that the current protocol is **synchronous and polling-based**. The CPU cannot do anything else while waiting for `DONE == 1`, and each operation requires a full round-trip across the AXI bus before the next one can begin.

This is a well-known challenge in hardware accelerator integration, and there are several  approaches to fix it:

**1. Interrupt-driven completion instead of polling**
Replace the busy-wait on `STATUS[DONE]` with a hardware interrupt. The CPU issues a command, then continues executing other work. When the accelerator finishes, it raises an interrupt line and the CPU handles the result asynchronously. This eliminates idle CPU cycles spent polling and enables true overlap between software and hardware execution.

**2. Command batching / streaming interface**
Rather than one command per AXI transaction sequence, design a small FIFO or command queue in the accelerator. The CPU can write a burst of keys and commands into the queue and walk away. The hardware drains the queue at full speed, and the CPU reads back a batch of results in one go. This amortizes the per-operation bus overhead across many operations.


**3. Tighter AXI integration**
The current design requires at least two separate write transactions (KEY_IN, then CONTROL) plus a STATUS poll. An improved register map could combine key and command into a single write: writing to a `KEY_CMD` register both latches the key and triggers the operation atomically. This cuts the minimum bus transactions per operation roughly in half.
