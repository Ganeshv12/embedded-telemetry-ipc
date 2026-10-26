#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include "telemetry_common.h"

static volatile sig_atomic_t g_running = 1;

static void handle_signal(int sig) {
    (void)sig;
    g_running = 0;
}

/* Helper to read exact bytes safely into a fixed buffer */
static int read_exact(int fd, uint8_t *dest, size_t count) {
    size_t total = 0;
    while (total < count && g_running) {
        uint8_t temp[64];
        size_t chunk = count - total;
        if (chunk > sizeof(temp)) {
            chunk = sizeof(temp);
        }

        ssize_t n = read(fd, temp, chunk);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;
            return -1;
        }
        memcpy(dest + total, temp, (size_t)n);
        total += (size_t)n;
    }
    return (total == count) ? 0 : -1;
}

int main(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    if (mkfifo(FIFO_PATH, 0666) < 0 && errno != EEXIST) {
        perror("[Daemon] mkfifo failed");
        return EXIT_FAILURE;
    }

    printf("[Daemon] FIFO ready at %s. Awaiting sensor nodes...\n", FIFO_PATH);

    int fifo_fd = open(FIFO_PATH, O_RDWR);
    if (fifo_fd < 0) {
        perror("[Daemon] Failed to open FIFO");
        unlink(FIFO_PATH);
        return EXIT_FAILURE;
    }

    uint32_t valid_packets = 0;
    uint32_t corrupted_packets = 0;

    while (g_running) {
        uint8_t sync_byte = 0;

        /* Step 1: Hunt for Start-of-Frame (SOF) */
        ssize_t n = read(fifo_fd, &sync_byte, 1);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;
            continue;
        }

        if (sync_byte != FRAME_SOF) {
            continue;
        }

        /* Step 2: Read remainder of header (3 bytes) */
        packet_header_t hdr;
        hdr.sof = sync_byte;

        uint8_t remaining_hdr[sizeof(packet_header_t) - 1];
        if (read_exact(fifo_fd, remaining_hdr, sizeof(remaining_hdr)) < 0) {
            break;
        }
        memcpy(((uint8_t *)&hdr) + 1, remaining_hdr, sizeof(remaining_hdr));

        if (hdr.payload_len > MAX_PAYLOAD_LEN) {
            corrupted_packets++;
            fprintf(stderr, "[Daemon] Invalid payload size: %u bytes\n", hdr.payload_len);
            continue;
        }

        /* Step 3: Read Payload + 1 byte CRC */
        uint8_t rx_buffer[MAX_PAYLOAD_LEN + 1];
        size_t total_payload_bytes = (size_t)hdr.payload_len + 1;

        if (read_exact(fifo_fd, rx_buffer, total_payload_bytes) < 0) {
            break;
        }

        /* Step 4: Verify CRC-8 */
        uint8_t received_crc = rx_buffer[hdr.payload_len];

        telemetry_packet_t packet;
        packet.header = hdr;
        memcpy(packet.payload, rx_buffer, hdr.payload_len);

        size_t bytes_to_crc = sizeof(packet_header_t) + (size_t)hdr.payload_len;
        uint8_t calculated_crc = compute_crc8((const uint8_t *)&packet, bytes_to_crc);

        if (received_crc != calculated_crc) {
            corrupted_packets++;
            fprintf(stderr, "[Daemon] CRC Mismatch! Expected: 0x%02X, Got: 0x%02X\n",
                    calculated_crc, received_crc);
            continue;
        }

        /* Step 5: Process valid frame */
        valid_packets++;
        if (hdr.data_type == 1 && hdr.payload_len == sizeof(float)) {
            float val = 0.0f;
            memcpy(&val, packet.payload, sizeof(float));
            printf("[Daemon Rx] Node %02u | Metric: %.2f | OK: %u | Corrupt: %u\n",
                   hdr.sensor_id, val, valid_packets, corrupted_packets);
        }
    }

    printf("\n[Daemon] Cleaning up FIFO and exiting...\n");
    close(fifo_fd);
    unlink(FIFO_PATH);

    return EXIT_SUCCESS;
}
