all: server client
server: server.c utils.h utils.c list.h list.c
	gcc -std=gnu99 -Wall -lpthread -o server server.c utils.c list.c
client: client.c utils.h utils.c list.h list.c
	gcc -std=gnu99 -Wall -lpthread -o client client.c utils.c list.c
.PHONY: clean all
clean:
	rm server client
