CC = gcc
CFLAGS = -Wall -g
SERVER = server
CLIENT = client
SRCS_SERVER = server.c
SRCS_CLIENT = client.c
OBJS_SERVER = $(SRCS_SERVER:.c=.o)
OBJS_CLIENT = $(SRCS_CLIENT:.c=.o)

all: $(SERVER) $(CLIENT)

$(SERVER): $(OBJS_SERVER)
	$(CC) $(CFLAGS) -o $(SERVER) $(OBJS_SERVER)

$(CLIENT): $(OBJS_CLIENT)
	$(CC) $(CFLAGS) -o $(CLIENT) $(OBJS_CLIENT)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER) $(CLIENT) $(OBJS_SERVER) $(OBJS_CLIENT)