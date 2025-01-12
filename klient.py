import tkinter as tk
from tkinter import simpledialog
import time
import socket
import select
import threading
import queue
import sys

class MathQuizClient:
    def __init__(self, root, server_address=('127.0.0.1', 2222)):
        self.root = root
        self.root.title("Math Quiz Game")
        self.root.geometry("500x400")
        self.nick = None
        self.room = None
        self.client_socket = None
        self.message_queue = queue.Queue()
        self.running_time=False
        self.start_quiz_flag=False
        self.host = False
        self.rooms = []
        self.brakpokoi = False
        self.cant_start = False
        self.mess_start = ""

        self.listenForChat = False
        self.validate_command_s = self.root.register(self.limit_length_short)
        self.validate_command_l = self.root.register(self.limit_length_long)
        
        self.ip = "0"
        self.port = 0
        
        self.ask_ip()
        
    def connect_to_server(self):
        try:
            self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.client_socket.connect(self.server_address)
            print("Connected to server")
        except ConnectionRefusedError:
            print("Could not connect to server")
            self.root.destroy()
            sys.exit("Application terminated: Unable to connect to server.")
        except socket.gaierror:
            self.root.destroy()
            sys.exit("Application terminated: Invalid server address.")
            
                
    def ask_ip(self):
        for widget in self.root.winfo_children():
            widget.destroy()

        tk.Label(self.root, text="Enter IP: (format: 127.0.0.1)", font=("Arial", 16)).pack(pady=20)

        self.ip_entry = tk.Entry(self.root, font=("Arial", 14), validate="key", validatecommand=(self.validate_command_s, "%P"))
        self.ip_entry.pack(pady=10)

        
        self.ip_entry.bind("<Return>", lambda event: self.submit_ip())
        tk.Button(self.root, text="Submit", command=self.submit_ip).pack(pady=10)
        self.ip_entry.focus()
        
        print(self.ip_entry)
        
    def ask_port(self):
        for widget in self.root.winfo_children():
            widget.destroy()

        tk.Label(self.root, text="Enter port: (format: 1234)", font=("Arial", 16)).pack(pady=20)

        self.port_entry = tk.Entry(self.root, font=("Arial", 14), validate="key", validatecommand=(self.validate_command_s, "%P"))
        self.port_entry.pack(pady=10)

        
        self.port_entry.bind("<Return>", lambda event: self.submit_port())
        tk.Button(self.root, text="Submit", command=self.submit_port).pack(pady=10)
        self.port_entry.focus()
        
        print(self.port_entry)
            
    def submit_ip(self):
        self.ip = self.ip_entry.get()
        self.ask_port()
    def submit_port(self):
        self.port = self.port_entry.get()
        try:
            self.server_address = (self.ip, int(self.port))
        except:
            self.root.destroy()
            sys.exit("Application terminated: Unable to connect to server.")
            
        self.connect_to_server()
        self.create_nick_entry()
            
    def create_nick_entry(self):
    
        if self.nick==None:
            data = self.client_socket.recv(1024).decode()
            print(data)
        for widget in self.root.winfo_children():
            widget.destroy()

        tk.Label(self.root, text="Enter your nickname:", font=("Arial", 16)).pack(pady=20)

        self.nick_entry = tk.Entry(self.root, font=("Arial", 14), validate="key", validatecommand=(self.validate_command_s, "%P"))
        self.nick_entry.pack(pady=10)

        
        self.nick_entry.bind("<Return>", lambda event: self.submit_nick())
        tk.Button(self.root, text="Submit", command=self.submit_nick).pack(pady=10)
        self.nick_entry.focus()
        
        print(self.nick)
        if self.nick:
            self.nick_entry.insert(tk.END, self.nick)
            tk.Label(self.root, text="Ten nick jest juz zajety!", font=("Arial", 16)).pack(pady=20)
            

    def submit_nick(self):
        self.nick = self.nick_entry.get().strip().replace(" ", "_")
        if self.nick:
            self.client_socket.sendall(self.nick.encode('utf-8') + b'\n')
            expect = 4
            data = self.client_socket.recv(1024).decode()
            if data[0]=='N':
                self.create_nick_entry()
                return
            expect -= len(data.splitlines())
            while expect>0:
                data = self.client_socket.recv(1024).decode()
                expect -= len(data.splitlines())
                
            self.choose_room_option()
    def choose_room_option(self):
        for widget in self.root.winfo_children():
            widget.destroy()

        tk.Label(self.root, text="Choose an option:", font=("Arial", 16)).pack(pady=20)

        tk.Button(self.root, text="Create Room", command=self.create_room).pack(pady=10)
        tk.Button(self.root, text="Join Room", command=self.show_available_rooms).pack(pady=10)
        print(self.brakpokoi)
        if self.brakpokoi:
            tk.Label(self.root, text="Brak pokoi do dołączenia", font=("Arial", 16)).pack(pady=20)
    
    def back_menu(self):
        self.rooms = []
        self.client_socket.sendall("/back".encode('utf-8') + b'\n')
        data = self.client_socket.recv(1024).decode()
        print(data)
        self.choose_room_option()
        
    def create_room(self):
        self.client_socket.sendall('2'.encode('utf-8') + b'\n')
        data = self.client_socket.recv(1024).decode()
        print(data)
        
        for widget in self.root.winfo_children():
            widget.destroy()

        tk.Button(self.root, text="back", command=self.back_menu).place(x=0,y=0)

        tk.Label(self.root, text="Enter room name:", font=("Arial", 16)).pack(pady=20)

        self.room_entry = tk.Entry(self.root, font=("Arial", 14), validate="key", validatecommand=(self.validate_command_s, "%P"))
        self.room_entry.pack(pady=10)


        self.room_entry.bind("<Return>", lambda event: self.submit_create_room())
        tk.Button(self.root, text="Submit", command=self.submit_create_room).pack(pady=10)
        self.RoomExistError = tk.Label(self.root, text="", font=("Arial", 16))
        self.RoomExistError.pack(pady=10)
        self.room_entry.focus()
    
    def submit_create_room(self):
        self.room = self.room_entry.get().strip().replace(" ", "_")
        self.client_socket.sendall(self.room.encode('utf-8') + b'\n')
        self.show_room_menu()
			

    def show_available_rooms(self):
        if self.rooms:
            self.rooms = []
            self.client_socket.sendall('/back'.encode('utf-8') + b'\n')
            expect = 4
            mess = self.client_socket.recv(1024).decode()
            expect -= len(mess.splitlines())
            while expect>0:
                mess = self.client_socket.recv(1024).decode()
                expect -= len(mess.splitlines())
                
            self.client_socket.sendall('1'.encode('utf-8') + b'\n')
            mess = self.client_socket.recv(1024).decode()
            mess = mess.splitlines()
            if mess[0].split(" ")[0]=="Brak":
                self.brakpokoi = True
                self.choose_room_option()
                return
            while "Dołącz" not in mess[-1]:
                print("Jeszcze zbieram dane")
                mess2 = self.client_socket.recv(1024).decode()
                mess += mess2.splitlines()
        else:

            self.rooms = []
            self.client_socket.sendall('1'.encode('utf-8') + b'\n')
            mess = self.client_socket.recv(1024).decode()
            mess = mess.splitlines()
            if mess[0].split(" ")[0]=="Brak":
                self.brakpokoi = True
                self.choose_room_option()
                return
            while "Dołącz do pokoju nr:" not in mess[-1]:
                print("Jeszcze zbieram dane")
                mess2 = self.client_socket.recv(1024).decode()
                mess += mess2.splitlines()
            
        self.brakpokoi=False
        data=mess
        test = data[0][0]
        data.pop()
        if test == '1':
            for line in data:
                parts = line.split(" ")
                self.rooms.append([parts[2], parts[4][-1], parts[-1]])
        else:
            self.choose_room_option()
            return


        for widget in self.root.winfo_children():
            widget.destroy()


        tk.Button(self.root, text="back", command=self.back_menu).place(x=0,y=0)
        tk.Label(self.root, text="Available rooms:", font=("Arial", 16)).pack(pady=10)
        
        refresh_button = tk.Button(self.root, text="Refresh Rooms", command=self.show_available_rooms)
        refresh_button.pack(pady=10)

        canvas = tk.Canvas(self.root)
        canvas.pack(fill=tk.BOTH, expand=True, padx=10, pady=10, side="left")

        scrollbar = tk.Scrollbar(self.root, orient="vertical", command=canvas.yview)
        scrollbar.pack(side="right", fill="y")

        canvas.configure(yscrollcommand=scrollbar.set)

        rooms_frame = tk.Frame(canvas)

        canvas.create_window((0, 0), window=rooms_frame, anchor="nw")

        for i, room_name in enumerate(self.rooms):
            frame = tk.Frame(rooms_frame)
            frame.pack(pady=5, fill="x", padx=10)
            tk.Label(frame, text=f"pokoj: {room_name[0]} \ngracze: {room_name[2]}", font=("Arial", 14)).grid(row=0, column=0, padx=10)
            if room_name[1] == "3/3":
                btn_state = "disabled"
            else:
                btn_state = "normal"
            tk.Button(frame, text=f"Join {room_name[0]}", state=btn_state, command=lambda room_name=room_name[0], roomId=i+1, roomStatus=room_name[1]: self.join_room(room_name, roomId, roomStatus)).grid(row=0, column=1, padx=10)
            

        rooms_frame.update_idletasks()
        canvas.config(scrollregion=canvas.bbox("all"))

    def join_room(self, roomName, roomId, roomStatus):
        room_name = roomName
        room_id = roomId
        room_status = roomStatus
        self.client_socket.sendall(str(room_id).encode('utf-8') + b'\n')
        mess = "dołączył do pokoju."
        self.room = room_name
        if room_status=="1":
            self.client_socket.sendall('wrong answer'.encode() + b'\n')
            self.client_socket.sendall('/listplayers'.encode() + b'\n')
            data = self.client_socket.recv(1024).decode().replace("\x00", "")
            data = data.splitlines()
            self.players = []
            while "END" not in data[-1]:
                data2 = self.client_socket.recv(1024).decode()
                data2 = data2.splitlines()
                for line in data2:
                    data.append(line)
            
            data.pop(0)
            data.pop(0)
            data.pop(0)
            data.pop()

            if self.nick == data[0].split(" ")[2]:
                self.host = True
            else:
                self.host = False
            self.players.append([data[0].split(" ")[2], data[0].split(" ")[5]])
            data.pop(0)
            for line in data:
                self.players.append([line.split(" ")[2][:-1], line.split(" ")[4]])
            
            self.client_socket.sendall('/start'.encode() + b'\n')
            self.start_quiz_quest()
            return
        self.show_room_menu()

    def show_room_menu(self):
        for widget in self.root.winfo_children():
            widget.destroy()

        self.root.grid_columnconfigure(0, weight=1)
        self.root.grid_columnconfigure(1, weight=0)

        self.root.grid_rowconfigure(0, weight=1)

        self.frame1 = tk.Frame(master=self.root)
        self.frame1.grid(row=0, column=0, sticky="nsew")

        self.frame1.grid_columnconfigure(0, weight=1)
        self.frame1.grid_rowconfigure(0, weight=1)
        self.frame1.grid_rowconfigure(1, weight=1) 
        self.frame1.grid_rowconfigure(2, weight=1)
        self.frame1.grid_rowconfigure(3, weight=1) 

        tk.Button(self.frame1, text="Leave", command=self.leave_room).grid(row=0, column=0, padx=10, pady=10, sticky="nw")

        if self.listenForChat:
            self.listenForChat = False
            self.listen_thread.join()
            self.listen_thread = None

        self.show_players_in_room()
        if self.host:
            stateB = "normal"
        else:
            stateB = "disabled"
        tk.Label(self.frame1, text=f"Welcome to {self.room}!", font=("Arial", 20)).grid(row=0, column=0, pady=(50,10))
        tk.Button(self.frame1, text="Start Quiz", state=stateB, command=self.start_quiz_host).grid(row=1, column=0, pady=5)

        self.chat_display = tk.Text(self.frame1, width=40, height=10, state=tk.DISABLED)
        self.chat_display.grid(row=2, column=0, padx=10, pady=0)

        self.scrollchat = tk.Scrollbar(self.frame1, command=self.chat_display.yview)
        self.scrollchat.grid(row=2, column=1, sticky='ns')
        self.chat_display.config(yscrollcommand=self.scrollchat.set)
        

        self.message_input = tk.Entry(self.frame1, width=40, validate="key", validatecommand=(self.validate_command_l, "%P"))
        self.message_input.grid(row=3, column=0, padx=10, pady=5)
        tk.Button(self.frame1, text="Chat", command=self.chat).grid(row=3, column=1, pady=5)
        self.message_input.bind("<Return>", lambda event: self.chat())

        self.update_chat()
        
        self.listen_thread = threading.Thread(target=self.listen_for_messages)
        self.listen_thread.daemon = True
        self.listen_thread.start()

    def show_players_in_room(self):
        self.frame2 = tk.Frame(master=self.root, relief=tk.RAISED, bd=2)
        self.frame2.grid(row=0, column=1, sticky="ns")

        tk.Label(self.frame2, text="Players:", font=("Arial", 16)).grid(row=0, column=0, sticky="ew", padx=10, pady=5)
        self.players = []
        self.client_socket.sendall('/listplayers'.encode() + b'\n')
        data = self.client_socket.recv(1024).decode()
        data = data.splitlines()
        while "END" not in data[-1]:
            data2 = self.client_socket.recv(1024).decode()
            data2 = data2.splitlines()
            for line in data2:
                data.append(line)
        data.pop(0)
        data.pop(0)
        data.pop()

        if self.nick == data[0].split(" ")[2]:
            self.host = True
            print("jestem hostem")
        else:
            self.host = False
            print("nie jestem hostem")
        self.players.append([data[0].split(" ")[2], data[0].split(" ")[5]])
        data.pop(0)
        for line in data:
            self.players.append([line.split(" ")[2][:-1], line.split(" ")[4]])
        for i, player in enumerate(self.players):
            tk.Label(self.frame2, text=player[0] + ": "+player[1]+"pkt", font=("Arial", 14)).grid(row=1 + i, column=0, sticky="ew", padx=5, pady=5)
        self.listenForChat = True
            
    def listen_for_messages(self):
        while self.listenForChat:
            ready_to_read, _, _ = select.select([self.client_socket], [], [], 0.5)
            if ready_to_read:
                message = self.client_socket.recv(1024).decode()
                print(f"Received message: {message}")
                if "dołączył do pokoju." in message or "opuścił pokój." in message:
                    # Wywołanie planowane w wątku głównym zamiast bezpośrednio
                    self.root.after(0, self.show_room_menu)
                elif message.startswith("Pokój ID: ") and not self.host:
                    self.mess_start = message
                    self.start_quiz_flag=True
                    self.listenForChat=False
                    break
                if message:
                    self.message_queue.put(message)
        print("koniec listen for mess")
                    
    def update_chat(self):
        try:
            while not self.message_queue.empty():
                message = self.message_queue.get_nowait()
                self.chat_display.config(state=tk.NORMAL)
                self.chat_display.insert(tk.END, f"{message}\n")
                self.chat_display.see(tk.END)
                self.chat_display.config(state=tk.DISABLED)
        except queue.Empty:
            pass
            
        if not self.listenForChat and self.listen_thread:
            self.start_quiz()

        self.root.after(100, self.update_chat)
        
    def chat(self):
        message = self.message_input.get().strip()

        if message:
            if message[0] == "/":
                self.message_input.delete(0, tk.END)
                return
            self.chat_display.config(state=tk.NORMAL) 
            self.chat_display.insert(tk.END, f"You: {message}\n") 
            self.chat_display.see(tk.END)
            self.chat_display.config(state=tk.DISABLED) 
            self.client_socket.sendall(message.encode() + b'\n')
            self.message_input.delete(0, tk.END)
    
    def leave_room(self):
        self.listenForChat=False
        self.listen_thread.join()
        self.listen_thread = None
        self.client_socket.sendall('/leave'.encode() + b'\n')
        self.rooms = []
        expect = 4
        data = self.client_socket.recv(1024).decode()
        expect -= len(data.splitlines())
        while expect>0:
            data = self.client_socket.recv(1024).decode()
            expect -= len(data.splitlines())
        self.choose_room_option()
        
    def start_quiz_host(self):
        if self.host:
            self.start_quiz()
        else:
            self.show_room_menu()
        
    def start_quiz(self):
        for widget in self.root.winfo_children():
                widget.destroy()
        self.listenForChat=False
        self.listen_thread.join()
        self.listen_thread = None
        if self.host:
            self.client_socket.sendall("/start".encode() + b'\n')
        if self.start_quiz_flag:
            data = self.mess_start
        else:
            data = self.client_socket.recv(1024).decode()
        data = data.splitlines()
        self.start_quiz_flag = False
        print(data)
        if data[0][0] == "N" or data[0][0] == "T":
            self.cant_start = True
            self.show_room_menu()
            self.message_queue.put("Nie mozna rozpoczac gry")
            return
        while "Pytanie: " not in data[-1]:
            data2 = self.client_socket.recv(1024).decode()
            data2 = data2.splitlines()
            for line in data2:
                data.append(line)
        print(data)
        data.pop(0)
        data.pop(0)
        
        self.question_text = data[-1].replace("Pytanie: ", "")


        self.root.grid_columnconfigure(0, weight=1)
        self.root.grid_columnconfigure(1, weight=0)
        self.root.grid_rowconfigure(0, weight=1)
        
        self.frame1 = tk.Frame(master=self.root)
        self.frame1.grid(row=0, column=0, sticky="ew")
        
        self.frame1.grid_columnconfigure(0, weight=1)
        self.frame1.grid_rowconfigure(0, weight=1)
        self.frame1.grid_rowconfigure(1, weight=1) 
        self.frame1.grid_rowconfigure(2, weight=1)
        
        self.frame2 = tk.Frame(master=self.root, relief=tk.RAISED, bd=2)
        self.frame2.grid(row=0, column=1, sticky="ns")
        
        self.time = 30 * 100
        self.timer_label = tk.Label(self.frame1, text=self.format_time(self.time))
        self.timer_label.grid(row=0, column=0)

        self.question = tk.Label(self.frame1, text="What is " + self.question_text)
        self.question.grid(row=1, column=0)

        self.answer_input = tk.Entry(self.frame1, font=("Arial", 14), validate="key", validatecommand=(self.validate_command_s, "%P"))
        self.answer_input.grid(row=2, column=0, pady=(30, 200))

        self.answer_input.bind("<Return>", lambda event: self.submit_answer())

        tk.Label(self.frame2, text="Players:", font=("Arial", 16)).grid(row=0, column=0, sticky="ew", padx=10, pady=5)
        

        for i, player in enumerate(self.players):
            tk.Label(self.frame2, text=player[0] + ": 0pkt", font=("Arial", 14)).grid(row=1 + i, column=0, sticky="ew", padx=5, pady=5)

        self.running_time = True
        self.time_thread = threading.Thread(target=self.update_timer)
        self.time_thread.daemon = True
        self.time_thread.start()
        
    def start_quiz_quest(self):
        for widget in self.root.winfo_children():
                widget.destroy()
        data = self.client_socket.recv(1024).decode().replace("\x00", "")
        data = data.splitlines()
        print("zaczynamy")
        print(data)
        for i, item in enumerate(data):
            if "dołączył do pokoju." in item:
                self.players.append([data[i].split(" ")[0], "0"])
                data.pop(i)
                break
        if len(data)==1:
            data2 = self.client_socket.recv(1024).decode()
            data2 = data2.splitlines()
            for line in data2:
                data.append(line)
        
        for i, item in enumerate(data):
            if "dołączył do pokoju." in item:
                self.players.append([data[i].split(" ")[0], "0"])
                data.pop(i)
                break
            if "Koniec gry!" in item:
                while "END" not in data[-1]:
                    data2 = self.client_socket.recv(1024).decode()
                    data2 = data2.splitlines()
                    for line in data2:
                        data.append(line)
                self.show_room_menu()
                return
        while "Pytanie: " not in data[-1] and "Koniec gry!" not in data:
            data3 = self.client_socket.recv(1024).decode()
            data3 = data3.splitlines()
            for line in data3:
                data.append(line)
        if "Koniec gry!" in data:
            self.show_room_menu()
            return
                
        for i, item in enumerate(data):
            if "dołączył do pokoju." in item:
                self.players.append([data[i].split(" ")[0], "0"])
                data.pop(i)
                break
        print(data)
        data.pop(0)
        data.pop(0)
        data.pop(0)
        
        self.question_text = data[-1].replace("Pytanie: ", "")


        self.root.grid_columnconfigure(0, weight=1)
        self.root.grid_columnconfigure(1, weight=0)
        self.root.grid_rowconfigure(0, weight=1)
        
        self.frame1 = tk.Frame(master=self.root)
        self.frame1.grid(row=0, column=0, sticky="ew")
        
        self.frame1.grid_columnconfigure(0, weight=1)
        self.frame1.grid_rowconfigure(0, weight=1)
        self.frame1.grid_rowconfigure(1, weight=1) 
        self.frame1.grid_rowconfigure(2, weight=1)
        
        self.frame2 = tk.Frame(master=self.root, relief=tk.RAISED, bd=2)
        self.frame2.grid(row=0, column=1, sticky="ns")
        
        self.time = 30 * 100
        self.timer_label = tk.Label(self.frame1, text=self.format_time(self.time))
        self.timer_label.grid(row=0, column=0)

        self.question = tk.Label(self.frame1, text="What is " + self.question_text)
        self.question.grid(row=1, column=0)

        self.answer_input = tk.Entry(self.frame1, font=("Arial", 14), validate="key", validatecommand=(self.validate_command_s, "%P"))
        self.answer_input.grid(row=2, column=0, pady=(30, 200))

        self.answer_input.bind("<Return>", lambda event: self.submit_answer())

        punkty=[]
        punkty.append(data[0].split(" ")[5])
        data.pop(0)
        data.pop(-1)
        data.pop(-1)
        for line in data:
            punkty.append(line.split(" ")[4])
        tk.Label(self.frame2, text="Players:", font=("Arial", 16)).grid(row=0, column=0, sticky="ew", padx=10, pady=5)
        
        
        for i, player in enumerate(self.players):
            tk.Label(self.frame2, text=player[0] + ": "+punkty[i]+"pkt", font=("Arial", 14)).grid(row=1 + i, column=0, sticky="ew", padx=5, pady=5)

        self.running_time = True
        self.time_thread = threading.Thread(target=self.update_timer)
        self.time_thread.daemon = True
        self.time_thread.start()
        
    def submit_answer(self):
        time_str = str(self.time)
        if len(time_str)==4:
            self.answer = self.answer_input.get()+ " " + time_str[0]
        else:
            self.answer = self.answer_input.get()
            if not self.answer:
                self.answer = "wrong 0"
            else:
                self.answer = self.answer+ " 0"
        print(self.answer)
        if not self.answer:
            self.answer = "wrong 0"
        self.running_time = False
        self.client_socket.sendall(self.answer.encode() + b'\n')
        self.start_quiz_quest()
        
    def timer(self):
        print("Timer start")
        self.update_timer()
        self.show_room_menu()
        
    def update_timer(self):
        while self.time>0 and self.running_time:
            self.timer_label.config(text=self.format_time(self.time))
            self.time -= 1
            threading.Event().wait(0.01)
        if self.time<=0:
            self.running_time = False
            self.root.after(0, self.submit_answer)
            
    def format_time(self, czas):
        seconds = czas // 100
        czas = czas % 100
        return f"{seconds:02}:{czas:02}"
        
    def show_result(self, result):
        tk.Label(self.root, text=f"Your answer: {result}", font=("Arial", 16)).pack(pady=20)
        tk.Button(self.root, text="Play Again", command=self.start_quiz).pack(pady=10)

    def show_chat_message(self, message):
        """Wyświetla wiadomość w pokoju"""
        tk.Label(self.root, text=f"{self.nick}: {message}", font=("Arial", 14)).pack(pady=5)
        
    def limit_length_short(self, value):
        return len(value) <= 16
        
    def limit_length_long(self, value):
        return len(value) <= 64


if __name__ == "__main__":
    root = tk.Tk()
    app = MathQuizClient(root)
    root.mainloop()