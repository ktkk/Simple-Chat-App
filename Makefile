CC = gcc

OUT_DIR = out
SRC_DIR = src

# find all the client source files
CLIENT_SRCS = $(shell find $(SRC_DIR)/client -name *.c)
# find all the server source files
SERVER_SRCS = $(shell find $(SRC_DIR)/server -name *.c)

# substitute object filenames from source files
CLIENT_OBJS = $(CLIENT_SRCS:%.c=$(OUT_DIR)/%.o)
SERVER_OBJS = $(SERVER_SRCS:%.c=$(OUT_DIR)/%.o)

all: client server


client: $(CLIENT_OBJS)
	# build client from client objects
	$(CC) $< -o out/$@ `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0`

server: $(SERVER_OBJS)
	# build server from server objects
	$(CC) $< -o out/$@ -lpthread 

$(CLIENT_OBJS): $(CLIENT_SRCS)
	# create out directory
	mkdir -p $(dir $@)
	# build the client object files
	$(CC) -c $< -o $@

$(SERVER_OBJS): $(SERVER_SRCS)
	# create out directory
	mkdir -p $(dir $@)
	# build the server object files
	$(CC) -c $< -o $@

.PHONY: clean
	rm -rf $(OUT_DIR)
