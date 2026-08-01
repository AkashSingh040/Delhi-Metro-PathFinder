/*
 * Delhi Metro PathFinder — CLI Application
 *
 * Compile:
 *   g++ -std=c++11 Delhi-Metro-PathFinder.cpp -o metro
 *
 * Run:
 *   ./metro.exe
 */

#include "metro.h"   // MetroSystem class + initializeDelhiMetro()

int main() {
    MetroSystem metro;

    cout << "\n";
    cout << "===========================================================\n";
    cout << "                                                           \n";
    cout << "             DELHI METRO ROUTE FINDER                     \n";
    cout << "                                                           \n";
    cout << "          Powered by Dijkstra's Algorithm                 \n";
    cout << "                                                           \n";
    cout << "===========================================================\n";

    cout << "\nLoading Delhi Metro Network...\n";
    initializeDelhiMetro(metro);
    cout << "Network loaded successfully!\n";

    metro.setLineChangePenalty(5);

    while (true) {
        cout << "\n" << string(60, '=') << "\n";
        cout << "MAIN MENU\n";
        cout << string(60, '=') << "\n";
        cout << "1. Find Route Between Stations\n";
        cout << "2. List All Stations\n";
        cout << "3. Exit\n";
        cout << string(60, '-') << "\n";
        cout << "Enter your choice: ";

        int choice;
        cin >> choice;
        cin.ignore();

        if (choice == 1) {
            string start, end;
            cout << "\nEnter starting station: ";
            getline(cin, start);
            cout << "Enter destination station: ";
            getline(cin, end);
            metro.displayRoute(start, end);
            cout << "\nPress Enter to continue...";
            cin.get();

        } else if (choice == 2) {
            metro.listAllStations();
            cout << "\nPress Enter to continue...";
            cin.get();

        } else if (choice == 3) {
            cout << "\nThank you for using Delhi Metro Route Finder!\n";
            cout << "Have a safe journey!\n\n";
            break;

        } else {
            cout << "\nInvalid choice! Please try again.\n";
        }
    }

    return 0;
}