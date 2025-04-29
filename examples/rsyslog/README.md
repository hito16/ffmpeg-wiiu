## Remote Syslog debugging

There are are two parts to this, 

* Server - a standard linux rsyslog daemon runnining in a docker container.

* Client - simple libraries that write to rsyslogd over TCP.


### Server

```
# Build the docker container
docker build -t rsyslogd-wiiu-9514 -f Dockerfile.rsyslogd  .

# Start the container in the back
docker run -d --name rsyslogd-wiiu-9514  -p 9514:9514/tcp -p 9514:9514/udp -v rsyslogd-wiiu-9514:/var/log/remote rsyslogd-wiiu-9514

# send a "remote" log line
echo "<14>Test Syslog message" | nc  localhost 9514

# Browse / tail the remote log files 
docker exec -it rsyslogd-wiiu-9514 /bin/bash

tail -f /var/log/remote/*
2025-04-29T00:24:28.461614+00:00 Test Syslog message
```

If you change the ports, be sure to change the -p arguments and the rsyslogd.conf file

A quick refresher on syslogd
* multi client UDP / TCP support
* simple clear text protocol
* line oriented
* Pretty UI integration
* log framework, with rotation, forwarding, aggregation, filtering

### Client

```
echo "<14>And I'd do it again" | nc  localhost 9514
```

or 
```

gcc -o test_rsyslogc rsyslog.c -I. test/test_rsyslog.c
./test_rsyslog
Sent to 127.0.0.1: <28> Apr 28 17:28:38 WIIU: This is a test message from my C program using TCP!
Syslog message sent successfully!
Sent to 127.0.0.1: <28> Apr 28 17:28:38 WIIU: An error occurred in the application!
Syslog message sent successfully!
```

```
# server side
root@118c5c18fbbe:/# tail -f /var/log/remote/*
2025-04-29T00:24:28.461614+00:00 Test Syslog message
2025-04-29T00:25:08.556891+00:00 And I'd do it again
2025-04-28T17:28:38+00:00 192.168.65.1 WIIU: This is a test message from my C program using TCP!
2025-04-28T17:28:38+00:00 192.168.65.1 WIIU: An error occurred in the application!
```
