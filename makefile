CC = gcc
CFLAGS = -Wall -Wextra -O2 -pedantic

all: telemetry_daemon sensor_node

telemetry_daemon: telemetry_daemon.c telemetry_common.h
	$(CC) $(CFLAGS) telemetry_daemon.c -o telemetry_daemon

sensor_node: sensor_node.c telemetry_common.h
	$(CC) $(CFLAGS) sensor_node.c -o sensor_node

clean:
	rm -f telemetry_daemon sensor_node /tmp/sensor_telemetry_fifo
