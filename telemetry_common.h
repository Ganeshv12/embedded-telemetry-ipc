#ifndef TELEMETRY_COMMON_H
#define TELEMETRY_COMMON_H

#include <stdint.h>
#include <stddef.h>

#define FIFO_PATH           "/tmp/sensor_telemetry_fifo"
#define FRAME_SOF           0xAA
#define MAX_PAYLOAD_LEN     32

typedef struct __attribute__((packed)) {
    uint8_t sof;
    uint8_t sensor_id;
    uint8_t data_type;
    uint8_t payload_len;
} packet_header_t;

typedef struct __attribute__((packed)) {
    packet_header_t header;
    uint8_t payload[MAX_PAYLOAD_LEN];
    uint8_t crc8;
} telemetry_packet_t;

static inline uint8_t compute_crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

#endif
