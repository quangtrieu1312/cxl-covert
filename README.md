# CXL Covert Channel Attack

A covert channel attack that exploits PCIe bus contention on a CXL memory expander to transmit data between two isolated processes without any direct communication channel.

## Attack Model

In cloud environments, multiple VMs may share a CXL memory expander attached via PCIe. Although the memory regions are isolated, the physical PCIe bus is shared. This creates a timing side channel: a sender process can modulate bus contention by flooding the CXL memory with cache-line evictions, and a reader process can observe the resulting latency increase using `RDTSCP` to decode the signal.

This attack demonstrates that memory isolation at the address space level does not prevent covert communication if the underlying interconnect is shared.

## Mechanism

**Sender:** For each bit window, if the bit is `1`, the sender allocates a memory buffer pinned to the CXL NUMA node and continuously writes and flushes cache lines using `clflush` + `sfence`, saturating the PCIe bus. If the bit is `0`, the sender idles for the duration of the window.

**Reader:** Continuously reads from a buffer on the CXL NUMA node and measures access latency using `RDTSCP`. At the end of each bit window, it computes `average_cycles = total_cycles / total_accesses`. If `average_cycles >= threshold`, the bit is decoded as `1` (contention detected), otherwise `0`.

## Environment

- **CXL Device:** SMART Modular Technologies CXA-4F1W (CXL 2.0 Type 3 Memory Expander)
- Linux with CXL memory exposed as a memory-only NUMA node via `daxctl`
- Sender and reader are two separate processes pinned to the CXL NUMA node using `numactl`

## Results

| Metric | Value |
|---|---|
| Bit rate | 1 bit/second |
| Accuracy | ~100% |
| Cycle threshold | 800 average cycles |
| Cycles with contention | ~867 |
| Cycles without contention | ~783 |
| Contention delta | ~84 cycles (~10% latency increase) |
| Noise testing | Not yet evaluated |

The ~10% latency increase under contention provides a reliable and consistent signal for bit decoding under controlled conditions.

## Prerequisite

- Linux
- gcc
- numactl

## Usage

```bash
# Launch the attack (sender + reader)
./run.sh <buffer_mb> <bit_window_us> <binary_message>

# Decode the captured log
./get_covert_message.sh <threshold> <input_file>
```

**Example:**
```bash
./run.sh 64 1000000 10110010
./get_covert_message.sh 800 reader.log
```

## Limitations and Future Work

- Bit rate is currently limited to 1 bit/second; higher rates have not been tested
- Attack has not yet been validated across VM boundaries, only between two processes on the same host
- Noise robustness under concurrent workloads has not been evaluated
- Error correction and signal calibration could improve reliability at higher bit rates
