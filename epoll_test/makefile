# Kompilacja programu, uruchomienie serwera i 3 klientów socat

main:
	c++ rooms.cpp question.cpp -Wall -o serwer
	./serwer 2222
listen: SHELL:=/bin/bash
listen:
	socat - tcp:localhost:2222

debug:
	g++ -g rooms.cpp question.cpp -o serwer_debug