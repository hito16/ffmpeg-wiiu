# Week 3: Stop!! time to address logging

_Caveat: Don't treat these as "proper" examples.  I'm just showing what I did to get up to speed with the homebrew environment.  Look at these, learn and avoid the same bruises_

### Problem:
* the "right" solution is to buy a USB serial cable.
* UDP is way to lossy to be useful
* SDL is the death of screen logging.  Outputting logs to the screen was annoying enough.  Once we delve into SDL graphics, we can no longer rely on outputting logs to the same screens we're drawing on.

### Stopgap:
TCP logging to rsyslogd server

A few devs suggested I implement tcp logging, and showed code samples.  That
gave me the idea of just doing rsyslog.  So here it is.

1. link in a library that redirects stdout and stderr to tcpip/syslog
2. run a docker config that simply starts rsyslogd in a container,
3. for ease of use, run a simple daemon that responds with the IP of the rsyslogd service

Result:  
Reliable printf() logging that feels like a normal posix app logging to syslog.
Easy setup with Docker, using services built into linux.
Standard, hi perf, configurable logging server, with formatting options and UI integrations.

* thanks to DanielKO and Serena for suggestion on TCP logging and pointers to redirecting stdout

## TODO
1. Look at the README.md in ../rsyslog for starting Dockerfile.rsyslog
2. As for this Wiiu app, Build it, run it.
3. docker exec into the Dockerfile.rsyslog and tail /var/log/remote/*

If you don't start the Docker image, the app will hang.

## Things I learned
1. With this last piece, we can now build a generic WiiU main() app
3. We can leave printf() fprintf() in place without code changes and see them in rsyslog.
2. single threaded, posix / SDL code that runs linux or MacOS could run on WiiU largely unaltered

