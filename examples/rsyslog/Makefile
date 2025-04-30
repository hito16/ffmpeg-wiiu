# Makefile

CC = gcc
CFLAGS = -I.

# Source file definitions
SRCS_TEST_RSYSLOG = *.c tools/test_rsyslog.c
SRCS_ANNOUNCE_CLI = *.c tools/udp_announce_cli.c
SRCS_ACK_SVC = *.c tools/udp_acknowledge_svc.c

# Output executable definitions
TARGET_TEST_RSYSLOG = test_rsyslog
TARGET_ANNOUNCE_CLI = udp_announce_cli
TARGET_ACK_SVC = udp_acknowledge_svc

# Combine targets and sources into lists
TARGETS = $(TARGET_TEST_RSYSLOG) $(TARGET_ANNOUNCE_CLI) $(TARGET_ACK_SVC)

all: $(TARGETS)

$(TARGET_TEST_RSYSLOG): $(SRCS_TEST_RSYSLOG)
	$(CC) $(CFLAGS) $(SRCS_TEST_RSYSLOG) -o $(TARGET_TEST_RSYSLOG)

$(TARGET_ANNOUNCE_CLI): $(SRCS_ANNOUNCE_CLI)
	$(CC) $(CFLAGS) $(SRCS_ANNOUNCE_CLI) -o $(TARGET_ANNOUNCE_CLI)

$(TARGET_ACK_SVC): $(SRCS_ACK_SVC)
	$(CC) $(CFLAGS) $(SRCS_ACK_SVC) -o $(TARGET_ACK_SVC)

clean:
	rm -f $(TARGET_TEST_RSYSLOG) $(TARGET_ANNOUNCE_CLI) $(TARGET_ACK_SVC)

.PHONY: all clean