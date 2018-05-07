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

#include "utils.h"

#define BACKLOG 10

typedef struct {
  int id;
  struct server_data *server_data;
} thread_arg;

void set_thread_signals(sigset_t *oldmask) {
  sigset_t blockMask;
  sigemptyset(&blockMask);
  sigaddset(&blockMask, SIGINT);
  sigaddset(&blockMask, SIGPIPE);
  if (pthread_sigmask(SIG_BLOCK, &blockMask, oldmask) < 0)
    ERR("pthread_sigmask");
}

void end_thread(thread_arg *workerArgs, sigset_t *oldmask) {
  if (!exit_flag)
    block_remove_thread(workerArgs->server_data->threads, pthread_self(), workerArgs->server_data->threadMutex);

  free(workerArgs);
  if (pthread_sigmask(SIG_SETMASK, oldmask, NULL) < 0)
    ERR("pthread_sigmask");
}

void *thread_work(void *args) {
	pthread_detach(pthread_self());
	int n;
	thread_arg *workerArgs = (thread_arg*)args;
	sigset_t oldmask;
	set_thread_signals(&oldmask);
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 4;
		
	pthread_mutex_lock(workerArgs->server_data->timedMutex);
	while(workerArgs->server_data->connections >= 1) {
		if ((n = pthread_cond_timedwait(workerArgs->server_data->cond, workerArgs->server_data->timedMutex, &ts)) != 0) {
			if(n == ETIMEDOUT) {
				printf("CHUJU ZLOTY NIE MA DLA CIEBIE MIEJSCA ELOO\n");
				pthread_mutex_unlock(workerArgs->server_data->timedMutex);
				client_t *client = block_pop_client(workerArgs->server_data->clients, workerArgs->server_data->timedMutex);
				// bulk_write(client->clientSocket, "xD\n", 3);
				printf("[%d] closing connection to the client\n", workerArgs->id);
				// if (TEMP_FAILURE_RETRY(close(client->clientSocket)) < 0)
				if (close(client->clientSocket) < 0)
					ERR("close");
				free(client);
				pthread_exit(NULL);
			} else {
				ERR("pthread_cond_timedwait");
			}
		}
	}
	printf("Przeszedlem mutex.\n");
	pthread_mutex_unlock(workerArgs->server_data->timedMutex);
	workerArgs->server_data->connections++;
	
	printf("[%d] started\n", workerArgs->id);

	printf("connected clients: %d\n", workerArgs->server_data->connections);
	for(;;);


	client_t *client = block_pop_client(workerArgs->server_data->clients, workerArgs->server_data->timedMutex);
	printf("[%d] closing connection to the client\n", workerArgs->id);
	// if (TEMP_FAILURE_RETRY(close(client->clientSocket)) < 0)
	if (close(client->clientSocket) < 0)
		ERR("close");
	free(client);

	int threadId = workerArgs->id;
	end_thread(workerArgs, &oldmask);
	pthread_mutex_lock(workerArgs->server_data->timedMutex);
	workerArgs->server_data->connections--;
	pthread_mutex_unlock(workerArgs->server_data->timedMutex);
	if (pthread_cond_signal(workerArgs->server_data->cond) != 0) ERR("pthread_cond_signal");
	printf("[%d] terminated\n", threadId);
	return NULL;
}


void acceptNewClient(struct server_data *server_data, int threadId) {
	printf("[SERVER] new client is trying to connect\n");
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(struct sockaddr_in);
	// int clientSocket = TEMP_FAILURE_RETRY(accept(server_data->serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen));
	int clientSocket = accept(server_data->serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
	if (clientSocket < 0) ERR("accept");

  if (pthread_mutex_lock(server_data->clientMutex) != 0) ERR("pthread_mutex_lock");
	add_client(server_data->clients, clientSocket, &clientAddr, &clientAddrLen);

	pthread_t tid;
	thread_arg *workerArgs = malloc(sizeof(thread_arg));
	if (workerArgs == NULL) ERR("malloc");
	workerArgs->id = threadId;
	workerArgs->server_data = server_data;
	// workerArgs->client_addr_len =
	if (pthread_create(&tid, NULL, thread_work, (void*)workerArgs) != 0)
		ERR("pthread_create");

	add_thread(server_data->threads, tid, threadId);

	if (pthread_mutex_unlock(server_data->clientMutex) != 0)
		ERR("pthread_mutex_unlock");
}

void server_work(struct server_data *server_data) {
	int nextThreadId = 1;

	fd_set readFds;
	while (!exit_flag) {
		FD_ZERO(&readFds);
		FD_SET(server_data->serverSocket, &readFds);
		if (pselect(server_data->serverSocket + 1, &readFds, NULL, NULL, NULL, server_data->oldmask) < 0) {
			if (errno == EINTR) continue;
			ERR("pselect");	
		}
		acceptNewClient(server_data, nextThreadId++);
	}
}

int main(int argc, char **argv) {
  int16_t PORT;
	int condition = 0;
  parse_arguments(argc, argv, &PORT);

  printf("[SERVER] Trying to establish connection on port number %d.\n", PORT);

	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
  thread_t *threads = NULL;
	pthread_mutex_t threadMutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t timedMutex = PTHREAD_MUTEX_INITIALIZER;
	client_t *clients = NULL;
	pthread_mutex_t clientMutex = PTHREAD_MUTEX_INITIALIZER;
	sigset_t mask, oldmask;
	set_signal_handlers(&mask, &oldmask);

	struct server_data server_data;
	server_data.serverSocket = make_socket(AF_INET, SOCK_STREAM);
	server_data.oldmask = &oldmask;
	server_data.threads = &threads;
	server_data.threadMutex = &threadMutex;
	server_data.timedMutex = &timedMutex;
	server_data.clients = &clients;
	server_data.clientMutex = &clientMutex;
	server_data.connections = 0;
	server_data.cond = &cond;
	server_data.condition = condition;
	printf("[SERVER] Structure with server data has been prepared.\n");
	
	bind_tcp_socket(server_data.serverSocket, PORT, BACKLOG);
	printf("[SERVER] Server is listening on port number %d\n", PORT);
	server_work(&server_data);
	printf("[SERVER] End of the server main thread work.\n[SERVER] Deleting remaining data.\n");
	remove_all_server_data(&server_data);
	printf("[SERVER] Server data has been removed.\n");
	printf("[SERVER] Shutting down the server.\n");
	return EXIT_SUCCESS;
}
