#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>


#include <pthread.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define ERR(source) (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
  perror(source), exit(EXIT_FAILURE) )

typedef struct thread {
    pthread_t tid;
    int id;
    struct thread *next;
} thread_t;

void add_thread(thread_t **head, pthread_t tid, int id);
void remove_thread(thread_t **head, pthread_t tid);
void block_remove_thread(thread_t **head, pthread_t tid, pthread_mutex_t *mutex);
void remove_all_threads(thread_t **head);
thread_t *find_thread(thread_t *head, pthread_t tid);
void join_threads(thread_t *head, pthread_mutex_t *listMutex);

typedef struct client {
  int clientSocket;
  struct sockaddr_in clientAddr;
  socklen_t clientAddrLen;
  struct client *next;
} client_t;

void add_client(client_t **head, int clientSocket, struct sockaddr_in *clientAddr, socklen_t *clientAddrLen);
client_t* pop_client(client_t **head);
client_t* block_pop_client(client_t **head, pthread_mutex_t *mutex);
void remove_all_clients(client_t **head);
