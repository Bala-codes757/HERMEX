# HERMEX

### Your Journey. Simplified. 🚌

HERMEX is a C-based bus ticket booking system designed to provide a realistic online bus reservation experience while demonstrating core Operating System concepts through the backend.

The project combines a customer-friendly web interface with a C backend implementing process management, inter-process communication, shared memory, synchronization, and concurrent booking operations.

---

## ✨ Features

- 🚌 Bus search and route selection
- 📅 Travel date selection
- 💺 Multiple seat selection
- 👥 Passenger details for each selected seat
- 💰 Automatic fare calculation
- 🎫 Automatic e-ticket generation
- 🆔 Unique HERMEX booking ID
- 📋 My Trips / booking history
- ❌ Ticket cancellation
- 🔒 Synchronized seat booking
- ⚡ C-based backend HTTP server

---

## 🖥️ Booking Experience

HERMEX is designed to feel like a real-world bus booking platform rather than a traditional Operating Systems demonstration.

### Booking Flow

Home → Search Bus → Select Bus → Choose Seats → Passenger Details → Review → Confirm → E-Ticket

---

## 🏗️ Architecture

                         HERMEX
                            │
              ┌─────────────┴─────────────┐
              │                           │
          FRONTEND                    BACKEND
              │                           │
       HTML / CSS / JS                    C
              │                           │
              └──────── HTTP API ─────────┘
                                          │
                         ┌────────────────┼────────────────┐
                         │                │                │
                    Shared Memory     Semaphore          IPC
                         │                │                │
                         └────────────────┼────────────────┘
                                          │
                                   ┌──────┴──────┐
                                   │             │
                                  pipe()       fork()
Technology Stack
Frontend
- HTML5
- CSS3
- JavaScript
- Fetch API
- Responsive Web Design
Backend
- C
- POSIX APIs
- TCP Sockets
- HTTP Server
- Shared Memory
- POSIX Semaphores
- Pipes
- Process Creation
Development Tools
- Clang / GCC
- Python HTTP Server
- Visual Studio Code
- Git
- GitHub
E-Ticket
After a successful booking, HERMEX generates an electronic ticket containing:
- Booking ID
- Bus name
- Route
- Travel date
- Departure time
- Selected seats
- Passenger details
- Fare
- Convenience fee
- Total amount
Generated tickets are stored in the tickets/ directory.
HERMEX/
│
├── backend/
│   └── main.c
│
├── frontend/
│   ├── index.html
│   ├── style.css
│   └── app.js
│
├── tickets/
│
└── README.md
TO RUN
1. Clone the Repository
git clone https://github.com/Bala-codes757/HERMEX.git
cd HERMEX

2. Start the Backend
cd backend
clang main.c -o hermex_backend -pthread
./hermex_backend

The backend will run on:
http://localhost:8080

3. Start the Frontend
Open another terminal:
cd frontend
python3 -m http.server 5500

Then open:
http://localhost:5500
OBJECTIVE
The objective of HERMEX is to bridge the gap between Operating Systems theory and practical software development.
Instead of demonstrating IPC, process management, shared memory, and synchronization only through isolated terminal programs, HERMEX integrates these concepts into a functional bus reservation system.
The project combines:
- Operating Systems
- C Programming
- Computer Networking
- Inter-Process Communication
- Process Management
- Synchronization
- Web Development
- User Interface Design

Author
Bala ES
CSE (AI/ML)
SRM Institute of Science and Technology

License
This project is developed for educational and academic purposes.
