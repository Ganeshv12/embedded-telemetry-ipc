#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include "telemetry_common.h"

int main(int argc, char *argv[]) {
    uint8_t sensor_id = (argc > 1) ? (uint8_t)atoi(argv[1]) : 1;
    printf("[Sensor %u] Starting simulation...\n", sensor_id);

    int fd = open(FIFO_PATH, O_WRONLY);
    if (fd < 0) {
        perror("[Sensor] open FIFO failed");
        return EXIT_FAILURE;
    }

    srand((unsigned int)(time(NULL) ^ (getpid() << 16)));

    while (1) {
        /* Wire buffer: Header (4) + Payload (4) + CRC (1) */
        uint8_t wire_buf[sizeof(packet_header_t) + sizeof(float) + 1];

        packet_header_t *hdr = (packet_header_t *)wire_buf;
        hdr->sof = FRAME_SOF;
        hdr->sensor_id = sensor_id;
        hdr->data_type = 1; /* Floating point */
        hdr->payload_len = sizeof(float);

        /* Generate simulated value */
        float simulated_val = 20.0f + ((float)rand() / (float)RAND_MAX) * 15.0f;
        memcpy(wire_buf + sizeof(packet_header_t), &simulated_val, sizeof(float));

        /* Compute CRC over Header + Payload */
        size_t bytes_to_crc = sizeof(packet_header_t) + sizeof(float);
        uint8_t crc = compute_crc8(wire_buf, bytes_to_crc);

        /* Place CRC right after the payload */
        wire_buf[bytes_to_crc] = crc;

        size_t total_packet_size = bytes_to_crc + 1;
        ssize_t bytes_written = write(fd, wire_buf, total_packet_size);

        if (bytes_written < 0) {
            if (errno == EPIPE) {
                fprintf(stderr, "[Sensor %u] Daemon closed pipe. Exiting.\n", sensor_id);
                break;
            }
            perror("[Sensor] write error");
        } else {
            printf("[Sensor %u] Emitted: %.2f | CRC: 0x%02X | Sent: %zd bytes\n", 
                   sensor_id, simulated_val, crc, bytes_written);
        }

        usleep(500000);
    }

    close(fd);
    return EXIT_SUCCESS;
}
