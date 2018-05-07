#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

#include "utils.h"

#define BUFFER_SIZE 1

#define ERR(source) (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
  perror(source), exit(EXIT_FAILURE) )

volatile sig_atomic_t last_signal=0 ;

void sigalrm_handler(int sig) {
  last_signal=sig;
}

int sethandler( void (*f)(int), int sigNo) {
  struct sigaction act;
  memset(&act, 0, sizeof(struct sigaction));
  act.sa_handler = f;
  if (-1==sigaction(sigNo, &act, NULL))
    return -1;
  return 0;
}

void usage(char *fileName) {
  fprintf(stderr, "Usage: %s hostname port\n", fileName);
  exit(EXIT_FAILURE);
}

void parse_client_arguments(int argc, char **argv, char **server, char **port) {
  if (argc != 3)
    usage(argv[0]);
  *server = argv[1];
  *port = argv[2];
}

struct sockaddr_in make_address(char *serverHostname, char *port) {
  int ret;
  struct sockaddr_in addr;
  struct addrinfo *result;
  struct addrinfo hints = {};
  hints.ai_family = AF_INET;
  if ((ret = getaddrinfo(serverHostname, port, &hints, &result))) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
    exit(EXIT_FAILURE);
  }
  addr = *(struct sockaddr_in *)(result->ai_addr);
  freeaddrinfo(result);
  return addr;
}

int connect_tcp_socket(char *name, char *port) {
  struct sockaddr_in addr;
  int socketDes;
  socketDes = make_socket(AF_INET, SOCK_STREAM);
  addr = make_address(name, port);
  if (connect(socketDes, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
    if (errno != EINTR)
      ERR("connect");
    else {
      fd_set writeFds;
      int status;
      socklen_t size = sizeof(int);
      FD_ZERO(&writeFds);
      FD_SET(socketDes, &writeFds);
      if (select(socketDes + 1, NULL, &writeFds, NULL, NULL) < 0)
        ERR("select");
      if (getsockopt(socketDes, SOL_SOCKET, SO_ERROR, &status, &size) < 0)
        ERR("getsockopt");
      if (status != 0)
        ERR("connect");
    }
  }
  return socketDes;
}

int main(int argc, char **argv) {
  char *server, *port;
  parse_client_arguments(argc, argv, &server, &port);
  if(sethandler(sigalrm_handler,SIGALRM)) ERR("Seting SIGALRM:");

  printf("Connecting to %s on port %s\n", server, port);
  int socketDes = connect_tcp_socket(server, port);
  printf("Connected\n");
  alarm(2);
  while(1){
    if(SIGALRM==last_signal) break;
  }

  printf("Closing connection\n");
  if (close(socketDes) < 0)
    ERR("close");
  printf("Terminating\n");
  return EXIT_SUCCESS;
}
