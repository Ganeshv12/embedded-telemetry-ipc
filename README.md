# Multi-Process Linux Telemetry Gateway (C / POSIX IPC)

A lightweight Linux telemetry ingestion engine written in C. Demonstrates inter-process communication (IPC) via POSIX Named Pipes (FIFOs), byte-level stream framing, CRC-8 integrity verification, and deterministic signal handling.

## Packet Protocol
- **SOF (1 Byte):** 0xAA start-of-frame marker
- **Sensor ID (1 Byte):** Source node identifier
- **Data Type (1 Byte):** Metric format descriptor
- **Payload Length (1 Byte):** Dynamic payload size
- **Payload (N Bytes):** Binary telemetry data
- **CRC-8 (1 Byte):** Checksum computed over header and payload (Polynomial 0x07)

## Build and Run
1. Compile the binaries:
   make
2. Start the telemetry daemon:
   ./telemetry_daemon
3. In another terminal, start a sensor node:
   ./sensor_node 1
