#!/bin/bash

# Ścieżka do programu serwera
SERVER_BINARY="./serwer"
PORT=2222

# Sprawdź, czy plik serwera istnieje
if [ ! -f "$SERVER_BINARY" ]; then
  echo "Błąd: Plik $SERVER_BINARY nie istnieje. Skompiluj program przed uruchomieniem."
  exit 1
fi

# Uruchom serwer w nowym terminalu
xterm -hold -e "$SERVER_BINARY $PORT" &
SERVER_PID=$!

# Poczekaj chwilę, aby upewnić się, że serwer się uruchomił
sleep 2

# Uruchom trzech klientów w nowych terminalach
for i in {1..3}; do
  xterm -hold -e "nc localhost $PORT" &
done

# Obsługa zakończenia procesu serwera po naciśnięciu Ctrl+C
trap "kill $SERVER_PID; exit" INT

# Pozostaw skrypt otwarty, dopóki serwer działa
wait $SERVER_PID
