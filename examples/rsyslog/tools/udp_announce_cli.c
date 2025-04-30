#include <arpa/inet.h>
#include <stdio.h>

#include "announce.h"

int main(int argc, char *argv[]) {
    // our rsyslogd is running on 9514, so we'll start our daemon on +1
    int port = 9515;
    char server_ip_buffer[17];
    int i = 0;
    int res = -1;
    for (int i = 0; i < 10; i++) {
        res = client_announce(server_ip_buffer, port);
        if (res == 0) {
            printf("SERVER_IP=%s\n", server_ip_buffer);
            return 0;
        }
    }
    return 1;
}
