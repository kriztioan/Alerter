/**
 *  @file   dalert.cpp
 *  @brief  Alerter Daemon
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2021-07-19
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <arpa/inet.h>

#include <string.h>

#include <netdb.h>

#include <sys/errno.h>

#include <ctype.h>

#include <time.h>

#include <stdarg.h>

#define MYPORT 7777
#define BACKLOG 10
#define PASSWD "admin"

extern int errno;

int strtrim(char *str);

/* PACKAGE INTERFACE */

typedef struct _PACKET {

  int type;

  time_t timestamp;

  char nick[21], msg[127], data[256];

} PACKET;

#define BLOCK_SIZE sizeof(struct _PACKET)

typedef struct _CLIENT {

  int active;

  char nick[21];
} CLIENT;

#define CLIENTMAX 64

#define STANDARD 0
#define IM 1
#define SERVER 2

/* OUTPUT INTERFACE */
int tfprintf(FILE *restrict_stream, const char *restrict_format, ...);

int finished;

void signalhandler(int signal) {
  tfprintf(stdout, "Server SHUTTING DOWN : SIGNAL %s recieved\n",
           strsignal(signal));
  finished = 1;
}

int main(int argc, char *argv[], char **envp) {

  // Set up signal handlers

  signal(SIGHUP, signalhandler);
  signal(SIGINT, signalhandler);
  signal(SIGTERM, signalhandler);
  signal(SIGSTOP, signalhandler);

  // Set up message log

  fclose(stdin);

  if (freopen("dalert.log", "a", stdout) == NULL) {
    tfprintf(stderr, "freopen : %s\n", strerror(errno));
    exit(1);
  }

  if (freopen("dalert.log", "a", stderr) == NULL) {
    tfprintf(stderr, "freopen : %s\n", strerror(errno));
    exit(1);
  }

  // Startup server

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd == -1) {
    tfprintf(stderr, "socket : %s\n", strerror(errno));
    exit(1);
  }

  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    tfprintf(stderr, "setsockopt : %s\n", strerror(errno));
    exit(1);
  }

  struct sockaddr_in local;

  local.sin_family = AF_INET;

  int myport = MYPORT;

  if (argc > 1)
    myport = atoi(argv[1]);

  local.sin_port = htons(myport);
  local.sin_addr.s_addr = htonl(INADDR_ANY);
  memset(&(local.sin_zero), '\0', 8);

  if (bind(sockfd, (struct sockaddr *)&local, sizeof(struct sockaddr)) == -1) {
    tfprintf(stderr, "bind : %s\n", strerror(errno));
    exit(1);
  }

  if (listen(sockfd, BACKLOG) == -1) {
    tfprintf(stderr, "listen : %s\n", strerror(errno));
    exit(1);
  }

  CLIENT clients[CLIENTMAX];

  memset(clients, 0, CLIENTMAX * sizeof(CLIENT));

  fd_set readfds, testfds;

  FD_ZERO(&readfds);

  FD_SET(sockfd, &readfds);

  testfds = readfds;

  int fdmax = sockfd;

  struct sockaddr_in remote;

  socklen_t sin_size = sizeof(struct sockaddr_in);

  char message[BLOCK_SIZE + 1];

  int sel, new_fd, superuser = -1, mode = -1, recieved = 0;

  tfprintf(stdout, "Server STARTED\n");

  if (fflush(stdout) != 0)
    tfprintf(stderr, "fflush : %s\n", strerror(errno));

  finished = 0;
  while ((finished == 0) &&
         ((sel = select(fdmax + 1, &testfds, NULL, NULL, NULL)) > 0)) {

    for (int ii = 0; ii < (fdmax + 1); ii++) {

      if (FD_ISSET(ii, &testfds)) {

        if (ii == sockfd) {

          new_fd = accept(sockfd, (struct sockaddr *)&remote, &sin_size);
          if (new_fd == -1) {
            tfprintf(stderr, "accept : %s\n", strerror(errno));
            exit(1);
          }

          tfprintf(stdout, "Connection OPENED from %s:%d on socket %d\n",
                   inet_ntoa(remote.sin_addr), ntohs(remote.sin_port), new_fd);

          int val = 1;

          socklen_t val_size = sizeof(val);

          if (setsockopt(new_fd, SOL_SOCKET, SO_KEEPALIVE, &val, val_size))
            tfprintf(stderr, "setsockopt : %s\n", strerror(errno));
          else
            tfprintf(stdout, "Connection KEEPALIVE enabled\n");

          val = 180;
          if (setsockopt(new_fd, IPPROTO_TCP, TCP_KEEPALIVE, &val, val_size))
            tfprintf(stderr, "setsockopt : %s\n", strerror(errno));

          if (getsockopt(new_fd, IPPROTO_TCP, TCP_KEEPALIVE, &val, &val_size))
            tfprintf(stderr, "getsockopt : %s\n", strerror(errno));
          else
            tfprintf(stdout, "Connection KEEPALIVE idle time : %d seconds\n",
                     val);

          val = 60;
          if (setsockopt(new_fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, val_size))
            tfprintf(stderr, "setsockopt : %s\n", strerror(errno));

          if (getsockopt(new_fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, &val_size))
            tfprintf(stderr, "getsockopt : %s\n", strerror(errno));
          else
            tfprintf(stdout, "Connection KEEPALIVE interval : %d seconds\n",
                     val);

          val = 4;
          if (setsockopt(new_fd, IPPROTO_TCP, TCP_KEEPCNT, &val, val_size))
            tfprintf(stderr, "setsockopt : %s\n", strerror(errno));

          if (getsockopt(new_fd, IPPROTO_TCP, TCP_KEEPCNT, &val, &val_size))
            tfprintf(stderr, "getsockopt : %s\n", strerror(errno));
          else
            tfprintf(stdout, "Connection KEEPALIVE count : %d\n", val);

          FD_SET(new_fd, &readfds);

          if (new_fd > fdmax)
            fdmax = new_fd;

          if (fflush(stdout) != 0)
            tfprintf(stderr, "fflush : %s\n", strerror(errno));

          if (fflush(stderr) != 0)
            tfprintf(stderr, "fflush : %s\n", strerror(errno));

          continue;
        } else {

          if (getpeername(ii, (struct sockaddr *)&remote, &sin_size) == -1)
            tfprintf(stderr, "getpeername : %s\n", strerror(errno));

          if ((recieved = recv(ii, message, BLOCK_SIZE, 0)) <= 0) {

            if (recieved == -1)
              tfprintf(stderr, "recv : %s\n", strerror(errno));

            tfprintf(stdout, "Connection CLOSED from %s:%d on socket %d\n",
                     inet_ntoa(remote.sin_addr), ntohs(remote.sin_port), ii);

            if (fdmax == ii)
              --fdmax;

            if (ii == superuser) {
              superuser = -1;
              mode = -1;
              continue;
            }

            close(ii);

            FD_CLR(ii, &readfds);

            clients[ii].active = 0;

            PACKET *packet = (PACKET *)message;

            packet->type = SERVER;

            strcpy(packet->msg, "HANDSHAKE");

            *packet->data = '\0';

            for (int jj = 0; jj < CLIENTMAX; jj++) {

              if (clients[jj].active) {

                strncat(packet->data, clients[jj].nick, 256);

                strncat(packet->data, "\n", 256);
              }
            }

            recieved = sizeof(struct _PACKET);

            for (int jj = 0; jj < (fdmax + 1); jj++) {

              if (FD_ISSET(jj, &readfds)) {

                if (jj != sockfd) {

                  if (send(jj, message, recieved, 0) == -1)
                    tfprintf(stderr, "send : %s\n", strerror(errno));
                }
              }
            }

            if (fflush(stdout) != 0)
              tfprintf(stderr, "fflush : %s\n", strerror(errno));

            if (fflush(stderr) != 0)
              tfprintf(stderr, "fflush : %s\n", strerror(errno));

            continue;
          }

          if (ii == superuser) {

            message[recieved] = '\0';

            strtrim(message);

            if (mode == -1) {

              if (strcmp(message, PASSWD) == 0) {

                mode = 1;

                if (send(ii,
                         "Pass phrase accepted\nWelcome to DALERT\nThis is "
                         "version 1.5\n\n> ",
                         62, 0) == -1)
                  tfprintf(stderr, "send : %s\n", strerror(errno));
              } else {
                if (send(ii, "Pass phrase NOT accepted\n", 25, 0) == -1)
                  tfprintf(stderr, "send : %s\n", strerror(errno));

                superuser = -1;

                close(ii);

                FD_CLR(ii, &readfds);
              }
            } else if (mode == 1) {

              if (strcmp(message, "QUIT") == 0) {
                tfprintf(stdout, "Server SHUTTING DOWN : COMMAND %s recieved\n",
                         message);
                finished = 1;
              } else if (strcmp(message, "WHO") == 0) {

                for (int jj = 0; jj < (fdmax + 1); jj++) {

                  if (FD_ISSET(jj, &readfds)) {

                    if (jj != sockfd) {

                      if (getpeername(jj, (struct sockaddr *)&remote,
                                      &sin_size) == -1)
                        tfprintf(stderr, "getpeername : %s\n", strerror(errno));

                      if ((recieved =
                               snprintf(message, BLOCK_SIZE,
                                        "IP %s; PORT %d; SOCKET %d; NICK %s\n",
                                        inet_ntoa(remote.sin_addr),
                                        ntohs(remote.sin_port), jj,
                                        jj == ii ? "superuser"
                                                 : clients[jj].nick)) == -1)
                        tfprintf(stderr, "snprintf\n");

                      if (send(ii, message, recieved, 0) == -1)
                        tfprintf(stderr, "send : %s\n", strerror(errno));
                    }
                  }
                }
              } else if (strcmp(message, "HELP") == 0) {

                if ((recieved = snprintf(
                         message, BLOCK_SIZE,
                         "AVAILABLE COMMANDS:\nHELP\nLOGOUT\nQUIT\nWHO\n")) ==
                    -1)
                  tfprintf(stderr, "sprintf\n");

                if (send(ii, message, recieved, 0) == -1)
                  tfprintf(stderr, "send : %s\n", strerror(errno));

              } else if (strcmp(message, "LOGOUT") == 0) {
                superuser = -1;
                mode = -1;
              } else {

                if (send(ii, "Unable to comply, command phrase not known\n", 43,
                         0) == -1)
                  tfprintf(stderr, "send : %s\n", strerror(errno));
              }

              if (mode == 1) {
                if (send(ii, "> ", 2, 0) == -1)
                  tfprintf(stderr, "send : %s\n", strerror(errno));
              }
            }
          } else {

            if (strncmp(message, "LOGIN", 5) == 0) {
              superuser = ii;
              if (send(ii, "Enter pass phrase : ", 20, 0) == -1)
                tfprintf(stderr, "send : %s\n", strerror(errno));
            } else if (recieved) {

              PACKET *packet = (PACKET *)message;

              if (packet->type == SERVER &&
                  strcmp(packet->msg, "HANDSHAKE") == 0) {

                tfprintf(stdout, "Recieved HANDSHAKE FROM %s \n", packet->nick);

                strcpy(clients[ii].nick, packet->nick);

                clients[ii].active = 1;

                *packet->data = '\0';

                for (int jj = 0; jj < CLIENTMAX; jj++) {

                  if (clients[jj].active) {

                    strncat(packet->data, clients[jj].nick, 256);

                    strncat(packet->data, "\n", 256);
                  }
                }
              }

              for (int jj = 0; jj < (fdmax + 1); jj++) {

                if (FD_ISSET(jj, &readfds)) {

                  if (jj != sockfd) {

                    if (send(jj, message, recieved, 0) == -1)
                      tfprintf(stderr, "send : %s\n", strerror(errno));
                  }
                }
              }
            }
          }
        }
      }
    }

    if (fflush(stdout) != 0)
      tfprintf(stderr, "fflush : %s\n", strerror(errno));

    if (fflush(stderr) != 0)
      tfprintf(stderr, "fflush : %s\n", strerror(errno));

    testfds = readfds;

    memset(message, 0, BLOCK_SIZE + 1);

    recieved = 0;
  }

  if (sel == -1 && !finished) {
    tfprintf(stderr, "select : %s\n", strerror(errno));
    exit(1);
  }

  if (close(sockfd) == -1)
    tfprintf(stderr, "close : %s\n", strerror(errno));

  tfprintf(stdout, "Server STOPPED\n");

  // Normal exit

  return (0);
}

int strtrim(char *str) {

  size_t len = strlen(str);

  size_t iii = 0;
  for (size_t ii = 0; ii < len; ii++) {

    if (isspace(str[ii]))
      continue;

    str[iii++] = str[ii];
  }

  str[iii] = '\0';

  return (iii);
}

int tfprintf(FILE *restrict_stream, const char *restrict_format, ...) {

  time_t t = time(NULL);

  struct tm *tm_struct = localtime(&t);

  va_list args;

  va_start(args, restrict_format);

  fprintf(restrict_stream, "%02d/%02d/%02d %02d:%02d:%02d - ",
          tm_struct->tm_mday, tm_struct->tm_mon + 1, tm_struct->tm_year + 1900,
          tm_struct->tm_hour, tm_struct->tm_min, tm_struct->tm_sec);
  int ret = vfprintf(restrict_stream, restrict_format, args);

  va_end(args);

  return (ret);
}
