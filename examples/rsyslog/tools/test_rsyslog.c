#include <stdio.h>

#include "rsyslog.h"
#include "announce.h"

int find_syslog_ip(char *server_ip_buffer) {
    int port = 9515;
    int i = 0;
    int res = -1;
    for (int i = 0; i < 3; i++) {
        res = client_announce(server_ip_buffer, port);
        if (res == 0) {
            return 0;
        }
    }
    return 1;
}

int main() {
    const char *server_address = "127.0.0.1";
    int port = 9514;
    int priority = 6;  // debug
    const char *message = "1 This is a test message from my C program using TCP!";

    if (rsyslog_send_tcp(server_address, port, priority, message) == 0) {
        printf("Syslog message 1 sent successfully to localhost!\n");
    } else {
        perror("Failed to send syslog message.");
    }

    // Example with a different priority and message
    priority = 7;  // debug
    message = "2 An error occurred in the application!";
    if (rsyslog_send_tcp(server_address, port, priority, message) == 0) {
        printf("Syslog message 2 sent successfully to localhost!\n");
    } else {
        perror("Failed to send syslog message.");
    }

    // Discover the IP
    char server_ip_buffer[17];
    int res = find_syslog_ip(server_ip_buffer);
    if (res == 0) {
        printf("Discovered Server IP %s\n", server_ip_buffer);
        message = "3 We discovered your IP on our own";
        if (rsyslog_send_tcp(server_ip_buffer, port, priority, message) == 0) {
            printf("Syslog message sent successfully to %s!\n",
                   server_ip_buffer);
        } else {
            perror("Failed to send syslog message.");
        }
    } else {
        perror("Failed to find Server IP");
    }

    return 0;
}
