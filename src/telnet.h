#ifndef _TELNET_H_
#define _TELNET_H_

#define TELNET_IAC 0xff
#define TELNET_CMD_DONT 0xfe
#define TELNET_CMD_DO 0xfd
#define TELNET_CMD_WONT 0xfc
#define TELNET_CMD_WILL 0xfb
#define TELNET_SUBCMD_LINEMODE 0x22
#define TELNET_SUBCMD_SGA 0x03
#define TELNET_SUBCMD_ECHO 0x01
#define TELNET_SUBCMD_BINARY 0x01

typedef struct TELNET_SESSION {
  int fd;
} TELNET_SESSION;

int telnet_iac(int fd, int cmd, int subcmd);

int telnet_printf(TELNET_SESSION *session, const char *format, ...);

#endif // _TELNET_H_
