#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>

#include "telnet.h"

int telnet_iac(int fd, int cmd, int subcmd) {
  char buf[3] = {TELNET_IAC, cmd, subcmd};
  return send(fd, buf, sizeof(buf), 0);
}

int telnet_printf(TELNET_SESSION *session, const char *format, ...) {
  char buf[1024];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  int len = strlen(buf);
  int sent = 0;
  while (sent < len) {
    int n = send(session->fd, buf + sent, len - sent, 0);
    if (n < 0) {
      perror("send");
      return -1;
    }
    sent += n;
  }

  return sent;
}
