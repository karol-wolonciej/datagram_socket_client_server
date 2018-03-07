#g++ -c -O2 -Wall -std=c++14 -o misc.o misc.c
#g++ -c -O2 -Wall -std=c++14 -o game_state.o game_state.cpp
#g++ -O2 -Wall -std=c++14 -o siktacka-server siktacka-server.c misc.o game_state.o
#g++ -O2 -Wall -std=c++14 -o siktacka-client siktacka-klient.c misc.o


all:
	g++ -O2 -Wall -std=c++11 -o siktacka-client main_client.cpp client.cpp datagram.cpp client_datagram.cpp
	g++ -O2 -Wall -std=c++11 -o siktacka-server main_server.cpp server.cpp datagram.cpp server_datagram.cpp
	
	
clean:
	rm -f siktacka-server
	rm -f siktacka-client
	
