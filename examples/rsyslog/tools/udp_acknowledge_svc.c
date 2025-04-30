#include <arpa/inet.h>
#include <stdio.h>

#include "announce.h"

int main(int argc, char *argv[]) {
    // our rsyslogd is running on 9514, so we'll start our daemon on +1
    int port = 9515;

    char server_ip_buffer[17];
    server_acknowledge(port);
    return 0;
}
