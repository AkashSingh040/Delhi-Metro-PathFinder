<div align="center">

# 🚇 Delhi Metro PathFinder

**A Blazing-Fast, Intelligent Route Planner for the Delhi Metro Network**

[![C++11](https://img.shields.io/badge/C++-11-blue.svg?style=flat&logo=c%2B%2B)](https://isocpp.org/)
[![JavaScript](https://img.shields.io/badge/JavaScript-ES6+-yellow.svg?style=flat&logo=javascript)](https://developer.mozilla.org/en-US/docs/Web/JavaScript)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/licenses/MIT)

[Features](#-key-features) • [Tech Stack](#-tech-stack) • [Installation](#-getting-started) • [API Reference](#-api-reference) • [Contributing](#-contributing)

</div>

---

## 📌 Overview

Navigating the extensive Delhi Metro system—with its 11 overlapping lines, hundreds of stations, and numerous interchange hubs—can be overwhelming. Standard maps show you the network topology, but they don't dynamically calculate the absolute fastest route while accounting for real-world variables like line-transfer penalties.

**Delhi Metro PathFinder** solves this by modeling the entire metro network as a weighted mathematical graph. Powered by a high-performance C++ backend and a sleek, minimalist web interface, it uses **Dijkstra's Algorithm** to compute the most time-efficient path between any two stations in milliseconds.

## ✨ Key Features

- ⚡ **Lightning Fast Calculations**: Backend written purely in C++11 for zero-overhead graph traversal.
- 🧠 **Smart Transfer Penalties**: Automatically factors in a 5-minute penalty for inter-line transfers, ensuring the suggested route is actually the fastest in the real world.
- 🎨 **Premium Minimalist UI**: A beautiful, Vercel-inspired dark-mode frontend that is responsive and clean.
- 🔌 **RESTful API**: Decoupled architecture serving data via standard HTTP JSON endpoints.
- 🗺️ **Comprehensive Coverage**: Maps over 300+ stations across all 11 colored lines (Red, Yellow, Blue, Green, Violet, Pink, Magenta, Grey, Aqua, Orange/Airport Express, and Rapid Metro).

## 🛠 Tech Stack

### Backend
- **C++11**: Core logic and algorithm implementation.
- **cpp-httplib**: A lightweight, single-header C++ library for serving HTTP requests.
- **Graph Theory**: Nodes (stations) and Edges (travel time) processed via a Min-Heap Priority Queue.

### Frontend
- **HTML5 & CSS3**: Custom-built, zero-dependency minimalist styling with CSS variables.
- **Vanilla JavaScript (ES6+)**: Handles asynchronous state management and API communication.

---

## 🚀 Getting Started

### Prerequisites
To run this project locally, you need a C++ compiler and a web browser.
- **Windows**: [MinGW-w64](https://www.mingw-w64.org/) (GCC)
- **Linux**: `g++` (usually pre-installed or available via `build-essential`)
- **macOS**: Clang (via Xcode Command Line Tools)

### Installation

1. **Clone the repository**
   ```bash
   git clone https://github.com/yourusername/Delhi-Metro-PathFinder.git
   cd Delhi-Metro-PathFinder
   ```

2. **Compile the Server**
   The project uses `httplib.h` which requires specific flags depending on your operating system.

   **Windows:**
   ```bash
   g++ -std=c++11 server.cpp -o server -lws2_32
   ```

   **Linux / macOS:**
   ```bash
   g++ -std=c++11 server.cpp -o server -pthread
   ```

3. **Run the Server**
   **Windows:**
   ```bash
   ./server.exe
   ```
   **Linux / macOS:**
   ```bash
   ./server
   ```

4. **Access the Web App**
   Open your preferred web browser and navigate to:
   ```
   http://localhost:1158
   ```

---

## 📡 API Reference

The C++ backend operates as a standalone REST API that you can easily integrate into other clients (like a mobile app).

### `GET /api/stations`
Retrieves a list of all available stations and the lines they belong to.

**Response (200 OK):**
```json
[
  {
    "name": "Kashmere Gate",
    "lines": ["Red", "Yellow", "Violet"]
  },
  ...
]
```

### `POST /api/route`
Calculates the fastest route between a source and a destination.

**Request:**
```json
{
  "from": "Rajiv Chowk",
  "to": "Kalkaji Mandir"
}
```

**Response (200 OK):**
```json
{
  "total_time": 35,
  "total_stations": 14,
  "changes": 1,
  "from": "Rajiv Chowk",
  "to": "Kalkaji Mandir",
  "stations": ["Rajiv Chowk", "Patel Chowk", "..."],
  "segments": [
    {
      "line": "Yellow",
      "stations": ["Rajiv Chowk", "Central Secretariat"]
    },
    {
      "line": "Violet",
      "stations": ["Central Secretariat", "Kalkaji Mandir"]
    }
  ]
}
```

---

## 🤝 Contributing

Contributions make the open-source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

*Notice a missing station or a new metro line extension? Feel free to add the connections in `metro.h` and submit a PR!*

## 📜 License

Distributed under the MIT License. See `LICENSE` for more information.

---
<div align="center">
  <b>Built with ❤️ for Delhi Metro commuters.</b>
</div>
