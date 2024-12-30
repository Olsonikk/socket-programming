#include <unistd.h> 
#include <string>
#include <iostream>
#include <vector>

using namespace std;

class Room
{
    public:
        unsigned int room_id;
        string name;
        static unsigned int next_room_id;

    Room()
    {
        cout << "Podaj nazwę pokoju: ";
        cin >> name;
        room_id = next_room_id++;
        cout << "Stworzono pokój o nazwie " + name << " z ID " << room_id << endl;
    }
};

unsigned int Room::next_room_id = 1;

int main()
{
    vector<Room> rooms;
    rooms.emplace_back(); // tworzy obiekt Room i od razu dodaje go do wektora
    rooms.emplace_back();
    rooms.emplace_back();

    cout << "ID pokoju 1: " << rooms[0].room_id << endl;
    cout << "ID pokoju 2: " << rooms[1].room_id << endl;
    cout << "ID pokoju 3: " << rooms[2].room_id << endl;
}