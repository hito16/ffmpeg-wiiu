#ifndef ANNOUNCE_H
#define ANNOUNCE_H

int client_announce(char *server_ip_buffer, int port);
int server_acknowledge(int server_port);

#endif  // ANNOUNCE_H