# Multi-Process Linux Telemetry Gateway (C / POSIX IPC)

A lightweight Linux telemetry ingestion engine in C emulating an embedded sensor hub where independent processes stream binary framed data to a central daemon over POSIX Named Pipes (FIFOs).

## 1. System Architecture

    [ Sensor Node 1 ] ---+
                         +---> ( /tmp/sensor_telemetry_fifo ) ---> [ Telemetry Daemon ]
    [ Sensor Node 2 ] ---+                                          |- 1. Sync Hunt (0xAA)
                                                                    |- 2. Header & Len Check
                                                                    |- 3. CRC-8 Validation
                                                                    |- 4. SIGINT Cleanup

## 2. Custom Wire Protocol

    +------------+-----------+-----------+-------------+--------------+-------+
    | SOF (0xAA) | Sensor ID | Data Type | Payload Len | Payload Data | CRC-8 |
    |  (1 Byte)  | (1 Byte)  | (1 Byte)  |  (1 Byte)   |  (N Bytes)   | (1 B) |
    +------------+-----------+-----------+-------------+--------------+-------+

- **SOF (0xAA):** Start-of-frame marker for stream re-synchronization.
- **Sensor ID:** Source process/node identifier.
- **Data Type:** Payload format (1 = IEEE-754 Float, 2 = Int32).
- **Payload Len:** Dynamic payload length (up to 32 bytes).
- **CRC-8:** Checksum (Polynomial 0x07) computed over Header + Payload.

## 3. Key Embedded Concepts
- **Packed Wire Serialization:** Uses `__attribute__((packed))` headers and contiguous byte buffers to eliminate struct padding mismatches.
- **EOF Churn Prevention:** Opens the FIFO with `O_RDWR` so the daemon blocks cleanly when producers disconnect.
- **POSIX Signal Safety:** Uses `sigaction()` (`SIGINT`/`SIGTERM`) to close descriptors and `unlink()` the FIFO gracefully.
- **Fortified Bounds:** Chunked reads satisfy GCC `-O2` `_FORTIFY_SOURCE` static analysis with zero warnings.

## 4. Build and Run
1. Compile: `make`
2. Terminal 1 (Daemon): `./telemetry_daemon`
3. Terminal 2 & 3 (Sensors): `./sensor_node 1` and `./sensor_node 2`
