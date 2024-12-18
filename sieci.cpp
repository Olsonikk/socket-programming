#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <algorithm>
#include <thread>
#include <mutex>
#include <netinet/in.h>
#include <unistd.h>

std::mutex roomMutex;

class Room {
public:
    std::string name;
    bool gameStarted = false;
    std::deque<std::string> players; // Kolejka graczy w pokoju (pierwszy jest hostem)
    const size_t maxPlayers = 10;

    Room(const std::string& name) : name(name) {}

    void addPlayer(const std::string& playerName) {
        std::lock_guard<std::mutex> lock(roomMutex);
        if (players.size() < maxPlayers) {
            players.push_back(playerName);
            std::cout << "\nGracz '" << playerName << "' dołączył do pokoju '" << name << "'.\n";
            displayHost();
        } else {
            std::cout << "\nPokój '" << name << "' jest pełny. Maksymalna liczba graczy to " << maxPlayers << ".\n";
        }
    }

    void removePlayer(const std::string& playerName) {
        std::lock_guard<std::mutex> lock(roomMutex);
        auto it = std::find(players.begin(), players.end(), playerName);
        if (it != players.end()) {
            players.erase(it);
            std::cout << "\nGracz '" << playerName << "' opuścił pokój '" << name << "'.\n";
            displayHost();
        } else {
            std::cout << "\nGracz '" << playerName << "' nie znajduje się w pokoju.\n";
        }
    }

    void startGame() {
        std::lock_guard<std::mutex> lock(roomMutex);
        if (!gameStarted) {
            if (players.empty()) {
                std::cout << "\nGra nie może zostać rozpoczęta. W pokoju nie ma graczy!\n";
                return;
            }
            gameStarted = true;
            std::cout << "\nGra w pokoju '" << name << "' została rozpoczęta przez hosta '" << players.front() << "'.\n";
        } else {
            std::cout << "\nGra w pokoju '" << name << "' już trwa!\n";
        }
    }

    void displayHost() {
        if (!players.empty()) {
            std::cout << "\nHostem pokoju '" << name << "' jest: " << players.front() << "\n";
        } else {
            std::cout << "\nPokój '" << name << "' nie ma aktualnie graczy.\n";
        }
    }

    void displayPlayers() {
        std::cout << "\nGracze w pokoju '" << name << "':\n";
        if (players.empty()) {
            std::cout << "Brak graczy w pokoju.\n";
        } else {
            size_t count = 1;
            for (const auto& player : players) {
                std::cout << count++ << ". " << player << (count == 2 ? " (Host)" : "") << "\n";
            }
        }
    }
};

std::vector<Room> rooms;
std::mutex roomsMutex;

void handleClient(int clientSocket) {
    char buffer[1024];
    std::string playerName;

    // Powitanie klienta
    send(clientSocket, "Podaj swoją nazwę gracza: ", 27, 0);
    recv(clientSocket, buffer, 1024, 0);
    playerName = buffer;
    playerName.erase(playerName.find_last_not_of(" \n\r\t") + 1);

    int choice;
    bool inRoom = false;
    Room* currentRoom = nullptr;

    do {
        if (!inRoom) {
            // Menu główne
            std::string menu = "\n=== MENU GŁÓWNE ===\n1. Dołącz do istniejącego pokoju\n2. Stwórz nowy pokój\n3. Wyjście\nTwój wybór: ";
            send(clientSocket, menu.c_str(), menu.size(), 0);
            recv(clientSocket, buffer, 1024, 0);
            choice = std::stoi(buffer);

            switch (choice) {
            case 1: {
                std::lock_guard<std::mutex> lock(roomsMutex);
                if (rooms.empty()) {
                    std::string message = "Brak pokoi do dołączenia. Stwórz najpierw nowy pokój!\n";
                    send(clientSocket, message.c_str(), message.size(), 0);
                } else {
                    std::string roomList = "\n=== ISTNIEJĄCE POKOJE ===\n";
                    for (size_t i = 0; i < rooms.size(); ++i) {
                        roomList += std::to_string(i + 1) + ". " + rooms[i].name + (rooms[i].gameStarted ? " (Gra w toku)" : "") + "\n";
                    }
                    send(clientSocket, roomList.c_str(), roomList.size(), 0);

                    send(clientSocket, "\nWybierz numer pokoju, do którego chcesz dołączyć: ", 52, 0);
                    recv(clientSocket, buffer, 1024, 0);
                    int roomChoice = std::stoi(buffer);

                    if (roomChoice > 0 && roomChoice <= static_cast<int>(rooms.size())) {
                        Room& selectedRoom = rooms[roomChoice - 1];
                        selectedRoom.addPlayer(playerName);
                        currentRoom = &selectedRoom;
                        inRoom = true;

                        std::string success = "\nDołączyłeś do pokoju: " + currentRoom->name + "\n";
                        send(clientSocket, success.c_str(), success.size(), 0);
                    } else {
                        std::string error = "Nieprawidłowy numer pokoju.\n";
                        send(clientSocket, error.c_str(), error.size(), 0);
                    }
                }
                break;
            }
            case 2: {
                send(clientSocket, "\nPodaj nazwę nowego pokoju: ", 31, 0);
                recv(clientSocket, buffer, 1024, 0);
                std::string roomName = buffer;
                roomName.erase(roomName.find_last_not_of(" \n\r\t") + 1);

                std::lock_guard<std::mutex> lock(roomsMutex);
                rooms.emplace_back(roomName);
                Room& newRoom = rooms.back();
                newRoom.addPlayer(playerName);
                currentRoom = &newRoom;
                inRoom = true;

                std::string success = "\nStworzono nowy pokój: " + roomName + "\n";
                send(clientSocket, success.c_str(), success.size(), 0);
                break;
            }
            case 3:
                send(clientSocket, "\nWyjście z programu. Do zobaczenia!\n", 39, 0);
                break;
            default:
                send(clientSocket, "\nNieprawidłowy wybór. Spróbuj ponownie.\n", 43, 0);
                break;
            }
        } else {
            // Menu pokoju
            std::string roomMenu = "\n=== MENU POKOJU ===\n1. Wyświetl graczy\n2. Opuszczanie pokoju\n3. Rozpocznij grę\nTwój wybór: ";
            send(clientSocket, roomMenu.c_str(), roomMenu.size(), 0);
            recv(clientSocket, buffer, 1024, 0);
            choice = std::stoi(buffer);

            switch (choice) {
            case 1:
                currentRoom->displayPlayers();
                break;
            case 2:
                currentRoom->removePlayer(playerName);
                inRoom = false;
                currentRoom = nullptr;
                break;
            case 3:
                currentRoom->startGame();
                break;
            default:
                send(clientSocket, "\nNieprawidłowy wybór. Spróbuj ponownie.\n", 43, 0);
                break;
            }
        }
    } while (choice != 3);

    close(clientSocket);
}

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Błąd przy tworzeniu gniazda!\n";
        return -1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    int one = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR,&one, sizeof(one));
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Błąd przy bindowaniu gniazda!\n";
        return -1;
    }

    if (listen(serverSocket, 5) == -1) {
        std::cerr << "Błąd przy nasłuchiwaniu na gnieździe!\n";
        return -1;
    }

    std::cout << "Serwer działa na porcie 8080. Oczekiwanie na połączenia...\n";

    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientSize = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);

        if (clientSocket == -1) {
            std::cerr << "Błąd przy akceptowaniu połączenia!\n";
            continue;
        }

        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }

    close(serverSocket);
    return 0;
}
