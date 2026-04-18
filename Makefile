CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -I./include -D_XOPEN_SOURCE=700
LDFLAGS = -lm -lpthread

all: dir authserver worldserver

dir:
	@mkdir -p build/src/authserver build/src/worldserver build/src/shared build/src/scripting build/src/game build/bin

authserver: dir
	$(CC) $(CFLAGS) -c -o build/src/authserver/main.o src/authserver/main.c
	$(CC) $(CFLAGS) -c -o build/src/authserver/AuthServer.o src/authserver/AuthServer.c
	$(CC) $(CFLAGS) -c -o build/src/authserver/AuthClient.o src/authserver/AuthClient.c
	$(CC) $(CFLAGS) -c -o build/src/shared/log.o src/shared/log.c
	$(CC) $(CFLAGS) -c -o build/src/shared/bytebuffer.o src/shared/bytebuffer.c
	$(CC) $(CFLAGS) -c -o build/src/shared/network.o src/shared/network.c
	$(CC) $(CFLAGS) -o bin/authserver build/src/authserver/main.o build/src/authserver/AuthServer.o build/src/authserver/AuthClient.o build/src/shared/log.o build/src/shared/bytebuffer.o build/src/shared/network.o $(LDFLAGS)

worldserver: dir
	$(CC) $(CFLAGS) -c -o build/src/worldserver/main.o src/worldserver/main.c
	$(CC) $(CFLAGS) -c -o build/src/worldserver/WorldServer.o src/worldserver/WorldServer.c
	$(CC) $(CFLAGS) -c -o build/src/worldserver/WorldPlayer.o src/worldserver/WorldPlayer.c
	$(CC) $(CFLAGS) -c -o build/src/shared/log.o src/shared/log.c
	$(CC) $(CFLAGS) -c -o build/src/shared/bytebuffer.o src/shared/bytebuffer.c
	$(CC) $(CFLAGS) -c -o build/src/shared/network.o src/shared/network.c
	$(CC) $(CFLAGS) -c -o build/src/scripting/wss_lexer.o src/scripting/wss_lexer.c
	$(CC) $(CFLAGS) -c -o build/src/scripting/wss_parser.o src/scripting/wss_parser.c
	$(CC) $(CFLAGS) -c -o build/src/scripting/wss_vm.o src/scripting/wss_vm.c
	$(CC) $(CFLAGS) -c -o build/src/scripting/wss_bindings.o src/scripting/wss_bindings.c
	$(CC) $(CFLAGS) -o bin/worldserver build/src/worldserver/main.o build/src/worldserver/WorldServer.o build/src/worldserver/WorldPlayer.o build/src/shared/log.o build/src/shared/bytebuffer.o build/src/shared/network.o build/src/scripting/wss_lexer.o build/src/scripting/wss_parser.o build/src/scripting/wss_vm.o build/src/scripting/wss_bindings.o $(LDFLAGS)

clean:
	rm -rf build bin

.PHONY: all dir clean
