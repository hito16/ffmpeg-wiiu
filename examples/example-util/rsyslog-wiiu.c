#include "rsyslog-wiiu.h"

#include <stdio.h>
#include <string.h>
#include <sys/iosupport.h>  // devoptab_list, devoptab_t

#include "announce.h"
#include "rsyslog.h"

// Global variable definition
char SYSLOG_IP[18];

// Static instance of devoptab
static devoptab_t devoptab;

int find_syslog_ip(char *server_ip_buffer) {
    int port = 9515;
    int i = 0;
    int res = -1;
    for (int i = 0; i < 10; i++) {
        res = client_announce(server_ip_buffer, port);
        if (res == 0) {
            return 0;
        }
    }
    return 1;
}

ssize_t write_msg_to_syslog(struct _reent *r, void *fd, const char *ptr,
                            size_t len) {
    char buffer[1024];
    snprintf(buffer, 1024, "%*.*s", len, len, ptr);
    rsyslog_send_tcp(SYSLOG_IP, 9514, 14, buffer);
    return len;
}

void init_stdout() {
    devoptab.name = "STDOUT";
    devoptab.structSize = sizeof devoptab;
    devoptab.write_r = &write_msg_to_syslog;

    devoptab_list[STD_OUT] = &devoptab;
    devoptab_list[STD_ERR] = &devoptab;
}

int init_rsyslogger() {
    char server_ip_buffer[18];
    if (find_syslog_ip(server_ip_buffer) != 0) {
        return 1;
    }
    // tbd - figure out WIIU api for pthread_mutex_lock
    strncpy(SYSLOG_IP, server_ip_buffer, sizeof(SYSLOG_IP) - 1);
    SYSLOG_IP[sizeof(SYSLOG_IP) - 1] = '\0';
    // pthread_mutex_unlock
    init_stdout();
    return 0;
}
