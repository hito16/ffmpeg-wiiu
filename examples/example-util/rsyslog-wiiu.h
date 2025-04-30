#ifndef RSYSLOG_WIIU
#define RSYSLOG_WIIU

#include <stddef.h>
#include <sys/iosupport.h>  // devoptab_list, devoptab_t

extern char SYSLOG_IP[18];

int init_rsyslogger();

#endif  // RSYSLOG_WIIU