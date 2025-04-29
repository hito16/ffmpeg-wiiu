#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define MAX_MSG_LEN 1024
/* A fairly cookie cutter approach to writing to a remote syslogd over
   TCP.  I feel like remote syslog forwarding should be available in an
   aroma plugin.
*/
int rsyslog_send_tcp(const char *syslog_server_ip, int port, int priority,
                     const char *message) {
    int sockfd;
    struct sockaddr_in servaddr;
    char syslog_msg[MAX_MSG_LEN];
    time_t now;
    struct tm *tm_info;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        return -1;
    }

    // Prepare server address
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    if (inet_pton(AF_INET, syslog_server_ip, &servaddr.sin_addr) <= 0) {
        perror("inet_pton failed");
        close(sockfd);
        return -2;
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect failed");
        close(sockfd);
        return -3;
    }

    // Format syslog message (RFC 3164)
    time(&now);
    tm_info = localtime(&now);
    strftime(syslog_msg, MAX_MSG_LEN, "<%d> %b %d %H:%M:%S WIIU: ", tm_info);
    strncat(syslog_msg, message, MAX_MSG_LEN - strlen(syslog_msg) - 1);

    // Send the message
    if (send(sockfd, syslog_msg, strlen(syslog_msg), 0) < 0) {
        perror("send failed");
        close(sockfd);
        return 1;
    }

    //printf("Sent to %s: %s\n", syslog_server_ip, syslog_msg);

    close(sockfd);
    return 0;
}