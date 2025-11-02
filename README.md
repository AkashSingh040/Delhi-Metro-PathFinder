# 🚇 Delhi Metro PathFinder

Find the optimal route between any two Delhi Metro stations using Dijkstra's shortest path algorithm.

## Features

- 🔍 Smart route finding with 300+ stations
- 🚉 Coverage of all 11 Delhi Metro lines
- ⏱️ Realistic time estimates with 5-min line change delay/penalty
- 🔄 Automatic interchange detection

## Installation

### Prerequisites
- C++ compiler with C++11 support

### Compile & Run

```bash
# Compile
g++ -std=c++11 main.cpp -o metro

# Run
./metro                # Linux/Mac
metro.exe             # Windows
```

## Usage

1. **Find Route**: Enter start and destination stations
2. **List Stations**: View all available stations
3. **Exit**: Close the application

## Example

```
Input:  Rajiv Chowk → Kashmere Gate
Output: 12 minutes via Yellow Line
Route:  Rajiv Chowk → New Delhi → Chawri Bazar → 
        Chandni Chowk → Kashmere Gate
```

## Metro Lines Covered

| Line | Route | Stations |
|------|-------|----------|
| Red | Rithala ↔ Shaheed Sthal | 21 |
| Yellow | Samaypur Badli ↔ HUDA City Centre | 37 |
| Blue | Dwarka ↔ Noida/Vaishali | 50+ |
| Green | Kirti Nagar ↔ Brigadier Hoshiar Singh | 22 |
| Violet | Kashmere Gate ↔ Raja Nahar Singh | 34 |
| Pink | Majlis Park ↔ Shiv Vihar | 38 |
| Magenta | Janakpuri West ↔ Botanical Garden | 25 |
| Grey | Dwarka ↔ Dhansa Bus Stand | 4 |
| Aqua | Noida Sector 51 ↔ Depot Station | 21 |
| Rapid | Sikanderpur ↔ Cyber City | 5 |
| Orange | New Delhi ↔ Dwarka Sector 21 | 6 |

## Algorithm

**Dijkstra's Shortest Path**
- Time Complexity: O((V + E) log V)
- Space Complexity: O(V + E)

## Tech Stack

- C++11
- STL (map, vector, queue, set)
- Graph data structure with adjacency list

