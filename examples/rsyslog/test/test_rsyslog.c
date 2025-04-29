#include <stdio.h>

#include "rsyslog.h"

// gcc -I.. ../rsyslog.c test_rsyslog.c -o test_rsyslog
int main() {
    const char *server_address = "127.0.0.1";
    int port = 9514;
    int priority = 6;  // debug
    const char *message = "This is a test message from my C program using TCP!";

    if (rsyslog_send_tcp(server_address, port, priority, message) == 0) {
        printf("Syslog message sent successfully!\n");
    } else {
        printf("Failed to send syslog message.\n");
    }

    // Example with a different priority and message
    priority = 7;  // debug
    message = "An error occurred in the application!";
    if (rsyslog_send_tcp(server_address, port, priority, message) == 0) {
        printf("Syslog message sent successfully!\n");
    } else {
        printf("Failed to send syslog message.\n");
    }

    return 0;
}
