all: out Client Server


Client: src/client/ClientGUI.c
	gcc src/client/ClientGUI.c -o out/ClientGUI `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0`

Server: src/server/Server.c
	gcc -o out/Server src/server/Server.c -lpthread 

out:
	mkdir out
