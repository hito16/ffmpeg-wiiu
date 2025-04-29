#ifndef RSYSLOG_H
#define RSYSLOG_H

int rsyslog_send_tcp(const char *syslog_server_ip, int port, int priority,
                     const char *message);

#endif  // RSYSLOG_H
