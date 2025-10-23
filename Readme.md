# 🧠 TCP/IP Chat Room

**Author:** VINODH KUMAR J  
**Project Name:** TCP/IP Chat Room

---

## 🗂️ Project Overview

**TCP/IP Chat Room** is a multi-client chat application written in C that allows users to communicate in real-time over a network. The server handles multiple clients concurrently using **TCP sockets** and **multithreading**, broadcasting messages to all connected users.

This project demonstrates networking, concurrency, and system-level programming in Linux.

---

## ✅ Features

- ✔️ Multi-client chat room
- ✔️ User registration & login
- ✔️ Single and group chat
- ✔️ Online user listing
- ✔️ Logout & account deletion
- ✔️ Notifications when users join or leave
- ✔️ Thread-safe message broadcasting using mutex

---

## 📁 File Structure

```
.
├── chat_room_ser      # Server-side program
|   └── makefile       # To compile server easily
├── char_room_cli      # Client-side program      
├── data/              # Folder containing user database
│   └── users.txt
└── README.md          # Project documentation
```

---

## ⚙️ Build Instructionss

### Server:

```bash
make
```

This generates an executable named `./ser`.

To clean server build files:

```bash
make clean
```

### Client:

```bash
gcc *.c
./a.out
```

---

## 🚀 Usage

1. Run the **server** first:

```bash
./ser
```

2. Run the **client**:

```bash
./a.out
```

3. Follow the on-screen menu to:
   - Register
   - Login
   - Chat (single or group)
   - View online users
   - Logout or delete account

---

## 📌 Notes

- Server is currently configured for **loopback mode (127.0.0.1)**.  
  - To use over a **LAN**, replace the server IP macro with your **WAN IP** or local network IP.  
- Displays online users after login  
- Notifies all users when someone joins or leaves  
- User accounts are stored in `data/users.txt`  
- Designed for Linux systems  

---

## 🧠 Example Chat Menu

```
[You]
1. Single Chat
2. Group Chat
3. Online Users
4. Logout to Main Menu
5. Delete Account & Exit
Choice:
```

---

## ✍️ Author

**VINODH KUMAR J**  
*Project: TCP/IP Chat Room – Multi-client Chat System in C*

