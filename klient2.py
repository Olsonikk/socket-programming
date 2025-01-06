import tkinter as tk
from tkinter import simpledialog
import serwerRaw as Serwer
import time
import socket

class MathQuizClient:
    def __init__(self, root, server_address=('127.0.0.1', 2222)):
        self.root = root
        self.root.title("Math Quiz Game")
        self.root.geometry("500x400")
        self.nick = None
        self.room = None
        self.client_socket = None
        self.server_address = server_address

        self.flag = True
        self.connect_to_server()


        self.create_nick_entry()
    def connect_to_server(self):
        """Nawiązywanie połączenia z serwerem"""
        try:
            self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.client_socket.connect(self.server_address)
            print("Connected to server")
        except ConnectionRefusedError:
            print("Could not connect to server")
            self.root.quit()
    def create_nick_entry(self):
    
        data = self.client_socket.recv(1024).decode()
        print(data)
        for widget in self.root.winfo_children():
            widget.destroy()

        tk.Label(self.root, text="Enter your nickname:", font=("Arial", 16)).pack(pady=20)

        self.nick_entry = tk.Entry(self.root, font=("Arial", 14))
        self.nick_entry.pack(pady=10)

        tk.Button(self.root, text="Submit", command=self.submit_nick).pack(pady=10)
        self.nick_entry.focus()

    def submit_nick(self):
        #Serwer.send(["CreatePlayer", self.nick_entry.get()])
        #print(Serwer.send("PlayerList"))
        self.nick = self.nick_entry.get().strip()
        if self.nick:
            self.client_socket.sendall(self.nick.encode('utf-8') + b'\n')
            data = self.client_socket.recv(1024).decode()
            print(data)
            self.choose_room_option()  # Po podaniu nicku, gracz może wybrać pokój

    def choose_room_option(self):
        for widget in self.root.winfo_children():
            widget.destroy()

        tk.Label(self.root, text="Choose an option:", font=("Arial", 16)).pack(pady=20)

        tk.Button(self.root, text="Create Room", command=self.create_room).pack(pady=10)
        tk.Button(self.root, text="Join Room", command=self.show_available_rooms).pack(pady=10)

    def create_room(self):
        self.client_socket.sendall('2'.encode('utf-8') + b'\n')
        data = self.client_socket.recv(1024).decode()
        print(data)
        
        for widget in self.root.winfo_children():
            widget.destroy()

        tk.Button(self.root, text="back", command=self.choose_room_option).place(x=0,y=0)

        tk.Label(self.root, text="Enter room name:", font=("Arial", 16)).pack(pady=20)

        self.room_entry = tk.Entry(self.root, font=("Arial", 14))
        self.room_entry.pack(pady=10)

        tk.Button(self.root, text="Submit", command=self.submit_create_room).pack(pady=10)
        self.RoomExistError = tk.Label(self.root, text="", font=("Arial", 16))
        self.RoomExistError.pack(pady=10)
        self.room_entry.focus()

    def submit_create_room(self):
        if self.room_entry.get() in Serwer.send("RoomList"):
            self.RoomExistError.config(text="This room already exist!")
            #time.sleep(2)
            #self.create_room()
        else:
            #Serwer.send(["CreateRoom", self.room_entry.get()])
            #print(Serwer.send("RoomList"))
            self.room = self.room_entry.get()
            self.client_socket.sendall(self.room.encode('utf-8') + b'\n')
            data = self.client_socket.recv(1024).decode()
            print(data)
            self.client_socket.sendall('1'.encode('utf-8') + b'\n')
            data = self.client_socket.recv(1024).decode()
            data = data.splitlines()[0].split(" ", 2)
            print(data)
            if data[2]==self.room:
                    print('jestem tutaj')
                    print(data[0])
                    self.client_socket.sendall(data[0].encode('utf-8') + b'\n')
                    data = self.client_socket.recv(1024).decode()
                    print(data)
                    data = self.client_socket.recv(1024).decode()
                    print(data)
                    self.choose_room_option()
            else:
                print("QGYHUIJKOLP")
                data = self.client_socket.recv(1024).decode()
                print(data)
                print(data.splitlines())
                for line in data.splitlines():
                    parts = line.split(" ", 2)
                    print(parts)
                    if parts[2]==self.room:
                        self.client_socket.sendall(parts[0].encode('utf-8') + b'\n')
                        data = self.client_socket.recv(1024).decode()
                        print(data)
                        
                self.choose_room_option()
                #self.show_room_menu()

    def show_available_rooms(self):
        """Pokazuje dostępne pokoje"""
        
        self.rooms=[]
        self.client_socket.sendall('1'.encode('utf-8') + b'\n')
        time.sleep(1)
        data = self.client_socket.recv(1024).decode()
        #data = data.splitlines()[0].split(" ", 2)
        print(data.splitlines())
        test = data.splitlines()[0][0]
        if test=='1':
            for line in data.splitlines():
                parts = line.split(" ", 2)
                if len(parts)==3:
                    print(parts)
                    self.rooms.append(parts[2])
        else:
            self.choose_room_option
            return
        self.rooms.pop()
        for widget in self.root.winfo_children():
            widget.destroy()

        tk.Button(self.root, text="back", command=self.choose_room_option).place(x=0,y=0)
        tk.Label(self.root, text="Available rooms:", font=("Arial", 16)).pack(pady=20)

        print("rooms: \n")
        print(self.rooms)
        #rooms = Serwer.send("RoomList")
        #rooms = ["Spelarnia", "Sigmoza", "Husaria"]
        for i, room_name in enumerate(self.rooms):
            frame = tk.Frame(self.root)
            frame.pack(pady=5)
            tk.Label(frame, text=room_name, font=("Arial", 14)).grid(row=0, column=0, padx=10)
            tk.Button(frame, text=f"Join {room_name}", command=lambda room_name=room_name, roomId=i+1: self.join_room(room_name, roomId)).grid(row=0, column=1, padx=10)

    def join_room(self, roomName, roomId):
        """Dołącza do pokoju"""
        room_name = roomName
        room_id = roomId
        self.client_socket.sendall(str(room_id).encode('utf-8') + b'\n')
        time.sleep(0.5)
        data = self.client_socket.recv(1024).decode()
        print(data)
        self.room = room_name
        self.show_room_menu()

    def show_room_menu(self):
        for widget in self.root.winfo_children():
            widget.destroy()

        # Ustawienie proporcji kolumn i wierszy
        self.root.grid_columnconfigure(0, weight=1)  # Kolumna 0 (frame1) zajmuje resztę przestrzeni
        self.root.grid_columnconfigure(1, weight=0)  # Kolumna 1 (frame2) ma stałą szerokość

        self.root.grid_rowconfigure(0, weight=1)  # Kolumna 0 (frame1) zajmuje całą dostępną wysokość

        # Frame1 - główny panel
        frame1 = tk.Frame(master=self.root)
        frame1.grid(row=0, column=0, sticky="nsew")  # Rozciągnięcie na całą dostępną przestrzeń

        # Konfiguracja układu w grid, aby elementy były wyśrodkowane
        frame1.grid_columnconfigure(0, weight=1)  # Wyśrodkowanie w poziomie
        frame1.grid_rowconfigure(0, weight=1)  # Wyśrodkowanie w pionie
        frame1.grid_rowconfigure(1, weight=1)  # Wyśrodkowanie w pionie
        frame1.grid_rowconfigure(2, weight=1)  # Wyśrodkowanie w pionie
        frame1.grid_rowconfigure(3, weight=1)  # Wyśrodkowanie w pionie

        tk.Button(frame1, text="Leave", command=self.choose_room_option).grid(row=0, column=0, padx=10, pady=10, sticky="nw")

        tk.Label(frame1, text=f"Welcome to {self.room}!", font=("Arial", 20)).grid(row=0, column=0, pady=(50,10))
        tk.Button(frame1, text="Start Quiz", command=self.start_quiz).grid(row=1, column=0, pady=5)

        #######################################



        #######################################
        #output
        #tk.Text(frame1, width=40, height=10).grid(row=2, column=0)
        self.chat_display = tk.Text(frame1, width=40, height=10, state=tk.DISABLED)  # Nie edytowalne pole
        self.chat_display.grid(row=2, column=0, padx=10, pady=0)

        #input
        self.message_input = tk.Text(frame1, width=40, height=2)
        self.message_input.grid(row=3, column=0, padx=10, pady=5)
        tk.Button(frame1, text="Chat", command=self.chat).grid(row=3, column=1, pady=5)

        # Pokazuje listę graczy w pokoju
        self.show_players_in_room()

    def show_players_in_room(self):
        """Wyświetla listę graczy w danym pokoju"""
        # Frame2 - lista graczy
        frame2 = tk.Frame(master=self.root, relief=tk.RAISED, bd=2)
        frame2.grid(row=0, column=1, sticky="ns")  # Ustawienie po prawej stronie

        tk.Label(frame2, text="Players:", font=("Arial", 16)).grid(row=0, column=0, sticky="ew", padx=10, pady=5)

        #######################################



        #######################################
        players = Serwer.send("PlayerList")
        for i, player in enumerate(players):
            tk.Label(frame2, text=player + ": 2137pkt", font=("Arial", 14)).grid(row=1 + i, column=0, sticky="ew", padx=5, pady=5)

    def chat(self):

        #######################################



        #######################################
        message = self.message_input.get("1.0", tk.END).strip()  # Pobranie tekstu z pola

        if message:  # Jeśli wiadomość nie jest pusta
            # Wypisanie wiadomości na ekranie (w chat_display)
            self.chat_display.config(state=tk.NORMAL)  # Umożliwienie edytowania
            self.chat_display.insert(tk.END, f"You: {message}\n")  # Dodanie wiadomości
            self.chat_display.config(state=tk.DISABLED)  # Zablokowanie edytowania

            # Wyczyść pole tekstowe po wysłaniu wiadomości
            self.message_input.delete("1.0", tk.END)

    def start_quiz(self):
        question = "What is 5 + 3?"
        answer = simpledialog.askstring("Question", question)
        if answer:
            self.show_result(answer)

    def show_result(self, result):
        tk.Label(self.root, text=f"Your answer: {result}", font=("Arial", 16)).pack(pady=20)
        tk.Button(self.root, text="Play Again", command=self.start_quiz).pack(pady=10)

    def show_chat_message(self, message):
        """Wyświetla wiadomość w pokoju"""
        tk.Label(self.root, text=f"{self.nick}: {message}", font=("Arial", 14)).pack(pady=5)


if __name__ == "__main__":
    root = tk.Tk()
    app = MathQuizClient(root)
    root.mainloop()
