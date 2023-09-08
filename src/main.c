#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <malloc.h>
#include <stdarg.h>
#include <pthread.h>
#include "telnet.h"
#ifdef TESTING
#include "unity.h"
#endif

typedef struct CMD_DESCRIPTOR {
  char *name;
  int (*func)(TELNET_SESSION *, int argc, char **argv);
  char *help;
} CMD_DESCRIPTOR;

static int cmd_help(TELNET_SESSION *session, int argc, char **argv);
static int cmd_echo(TELNET_SESSION *session, int argc, char **argv);
static int cmd_exit(TELNET_SESSION *session, int argc, char **argv);

static CMD_DESCRIPTOR commands[] = {
    {"help", cmd_help, "show help"},
    {"echo", cmd_echo, "echo input"},
    {"exit", cmd_exit, "exit shell"},
};


static int cmd_help(TELNET_SESSION *s, int argc, char **argv) {
  telnet_printf(s, "Available commands:\r\n");
  for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i) {
    telnet_printf(s, "%s - %s\r\n", commands[i].name, commands[i].help);
  }
  return 0;
}

static int cmd_echo(TELNET_SESSION *s, int argc, char **argv) {
  for (int i = 1; i < argc; ++i) {
    telnet_printf(s, "%s\r\n", argv[i]);
  }
  return 0;
}

static int cmd_exit(TELNET_SESSION *s, int argc, char **argv) {
  // TODO ...
  return -1;
}

static int get_line_buf(char *in, int len) {
  int i;
  for (i = 0; i < len; ++i) {
    if (in[i] == '\r' || in[i] == '\n') {
      break;
    }
  }

  // no newline found
  if (i == len) {
    return -1;
  }

  // remove tailing \r\n
  while (i < len && (in[i] == '\r' || in[i] == '\n')) {
    in[i] = '\0';
    ++i;
  }
  return i;
}

static int get_telnet_opt(char *in, int len, int *cmd, int *subcmd) {
  unsigned char *p = (unsigned char *)in;
  if (len < 3) {
    return -1;
  }

  if (p[0] != 0xff) {
    return -1;
  }

  *cmd = p[1];
  *subcmd = p[2];
  return 3;
}

static int parse_cmdline(char *in, int len, char **argv, int max) {
  int argc = 0;
  char *p = in;
  char *end = in + len;
  char quote = '\0';

  while (p < end) {
    // skip space
    while (p < end && *p == ' ') {
      ++p;
    }

    if (p == end) {
      break;
    }

    // found arg
    argv[argc++] = p;

    // check if arg is quoted
    if (*p == '\'' || *p == '\"') {
      quote = *p++;
      argv[argc - 1] = p;
    }

    // skip arg
    while (p < end && (*p != ' ' || quote)) {
      if (*p == quote) {
        quote = '\0';
        *p = '\0';
      }
      ++p;
    }

    if (p == end) {
      break;
    }

    // terminate arg
    *p++ = '\0';

    if (argc >= max) {
      break;
    }
  }

  return argc;
}

static void *telnet_session(void *p) {
  int n;
  int argc;
  int cmd, subcmd;
  int offset = 0;
  char buf[1024];
  char *argv[32];
  const char welcome[] = "Welcome to telnet server\r\n";
  const char prompt[] = "SHELL> ";
  TELNET_SESSION *session = (TELNET_SESSION *)p;

  telnet_printf(session, welcome);
  telnet_printf(session, prompt);

  for (;;) {
    n = recv(session->fd, buf + offset, sizeof(buf) - offset, 0);
    if (n <= 0) {
      break;
    }
    offset += n;

    n = get_telnet_opt(buf, offset, &cmd, &subcmd);
    if (n > 0) {
      memmove(buf, buf + n, offset - n);
      offset -= n;
      continue;
    }

    n = get_line_buf(buf, offset);
    if (n < 0) {
      continue;
    }

    argc = parse_cmdline(buf, n, argv, sizeof(argv) / sizeof(argv[0]));
    if (argc > 0) {
      for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i) {
        if (strcmp(argv[0], commands[i].name) == 0) {
          commands[i].func(session, argc, argv);
          break;
        }
      }
    }

    memmove(buf, buf + n, offset - n);
    offset -= n;

    telnet_printf(session, prompt);
  }

  printf("** disconnected **\n");
  close(session->fd);
  free(session);
  return NULL;
}

static int telnet_server(int port) {
  // create tcp socket server
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    return -1;
  }

  // bind
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  // set reuse addr
  int opt = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  int ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
  if (ret < 0) {
    perror("bind");
    return -1;
  }

  // listen
  ret = listen(fd, 5);
  if (ret < 0) {
    perror("listen");
    return -1;
  }

  return fd;
}

#ifndef TESTING
int main() {
  int fd;
  fd = telnet_server(2323);
  if (fd < 0) {
    return -1;
  }

  printf("telnet server running on port: 2323...\n");

  for (;;) {
    pthread_t thread;
    struct sockaddr_storage peer;
    socklen_t len = sizeof(struct sockaddr_storage);
    TELNET_SESSION *session = (TELNET_SESSION *)malloc(sizeof(TELNET_SESSION));
    session->fd = accept(fd, (struct sockaddr *)&peer, &len);
    printf("** connected **\n");
    pthread_create(&thread, NULL, telnet_session, session);
    pthread_detach(thread);
    // telnet_session(session);
  }

  return 0;
}
#else
/*=======MAIN=====*/
void setUp(void) {}

void tearDown(void) {}

void test_parse_cmdline_no_args(void) {
  char in[64] = "";
  char *argv[10];
  int argc = parse_cmdline(in, strlen(in), argv, 10);
  TEST_ASSERT_EQUAL(0, argc);
}

void test_parse_cmdline_one_arg(void) {
  char in[64] = "hello";
  char *argv[10];
  int argc = parse_cmdline(in, strlen(in), argv, 10);
  TEST_ASSERT_EQUAL(1, argc);
  TEST_ASSERT_EQUAL_STRING("hello", argv[0]);
}

void test_parse_cmdline_multiple_args(void) {
  char in[64] = "hello world";
  char *argv[10];
  int argc = parse_cmdline(in, strlen(in), argv, 10);
  TEST_ASSERT_EQUAL(2, argc);
  TEST_ASSERT_EQUAL_STRING("hello", argv[0]);
  TEST_ASSERT_EQUAL_STRING("world", argv[1]);
}

void test_parse_cmdline_quoted_args(void) {
  char in[64] = "hello 'world of programming'";
  char *argv[10];
  int argc = parse_cmdline(in, strlen(in), argv, 10);
  TEST_ASSERT_EQUAL(2, argc);
  TEST_ASSERT_EQUAL_STRING("hello", argv[0]);
  TEST_ASSERT_EQUAL_STRING("world of programming", argv[1]);
}

void test_parse_cmdline_quoted_args_with_spaces(void) {
  char in[64] = "hello 'world of programming' \"is amazing\"";
  char *argv[10];
  int argc = parse_cmdline(in, strlen(in), argv, 10);
  TEST_ASSERT_EQUAL(3, argc);
  TEST_ASSERT_EQUAL_STRING("hello", argv[0]);
  TEST_ASSERT_EQUAL_STRING("world of programming", argv[1]);
  TEST_ASSERT_EQUAL_STRING("is amazing", argv[2]);
}

void test_parse_cmdline_max_args(void) {
  char in[64] = "a b c d e f g h i j k l m n o p q r s t u v w x y z";
  char *argv[26];
  int argc = parse_cmdline(in, strlen(in), argv, 26);
  TEST_ASSERT_EQUAL(26, argc);
  for (int i = 0; i < argc; ++i) {
    TEST_ASSERT_EQUAL_STRING_LEN(&in[i * 2], argv[i], 1);
  }
}

void test_parse_cmdline_too_many_args(void) {
  char in[64] = "a b c d e f g h i j k l m n o p q r s t u v w x y z";
  char *argv[25];
  int argc = parse_cmdline(in, strlen(in), argv, 25);
  TEST_ASSERT_EQUAL(25, argc);
  for (int i = 0; i < argc; ++i) {
    TEST_ASSERT_EQUAL_STRING_LEN(&in[i * 2], argv[i], 1);
  }
}

void test_getlinebuf_notFound(void) {
  char in[64] = "hello world";
  TEST_ASSERT_EQUAL(-1, get_line_buf(in, strlen(in)));
}

void test_getlinebuf_withNewLine(void) {
  char in[64] = "hello world\nanother line";
  TEST_ASSERT_EQUAL(12, get_line_buf(in, strlen(in)));
}

void test_getlinebuf_withCrLf(void) {
  char in[64] = "hello world\r\nanother line";
  TEST_ASSERT_EQUAL(13, get_line_buf(in, strlen(in)));
}

void test_getlinebuf_moreThanOneLines(void) {
  char in[64] = "hello world\r\nanother line\r\nthird line";
  int len = strlen(in);
  TEST_ASSERT_EQUAL(13, get_line_buf(in, len));
  memmove(in, in + 13, len - 13);
  len -= 13;
  TEST_ASSERT_EQUAL(14, get_line_buf(in, len));
  memmove(in, in + 14, len - 14);
  len -= 13;
  TEST_ASSERT_EQUAL(-1, get_line_buf(in, len));
}

int main(void) {
  UnityBegin("test/TestProductionCode.c");
  RUN_TEST(test_getlinebuf_notFound);
  RUN_TEST(test_getlinebuf_withNewLine);
  RUN_TEST(test_getlinebuf_withCrLf);
  RUN_TEST(test_getlinebuf_moreThanOneLines);
  RUN_TEST(test_parse_cmdline_no_args);
  RUN_TEST(test_parse_cmdline_one_arg);
  RUN_TEST(test_parse_cmdline_multiple_args);
  RUN_TEST(test_parse_cmdline_quoted_args);
  RUN_TEST(test_parse_cmdline_quoted_args_with_spaces);
  RUN_TEST(test_parse_cmdline_max_args);
  RUN_TEST(test_parse_cmdline_too_many_args);

  return (UnityEnd());
}
#endif
