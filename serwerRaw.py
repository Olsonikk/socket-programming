# module.py

# Zmienna
my_variable = "Hello from module!"
RoomList = ["Spelarnia", "Sigmoza", "Husaria"]
PlayerList = ["Olek", "Pawel", "Bartek"]
# Funkcja
def send(komenda):
    if komenda=="RoomList":
        return RoomList
    elif komenda=="PlayerList":
        return PlayerList
    elif komenda[0]=="CreateRoom":
        RoomList.append(komenda[1])
    elif komenda[0]=="CreatePlayer":
        PlayerList.append(komenda[1])

