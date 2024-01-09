#ifndef CONNECTED_CLIENTS_H
#define CONNECTED_CLIENTS_H

#include <stdint.h>
#include <string.h>

class ConnectedClients {
    uint8_t connectedClients[50][6];
    int numConnectedClients = 0;

    public:
    bool checkIfClientExists(uint8_t client[]);
    void addClient(uint8_t client[]);
    int getNumConnectedClients();
    void removeClient(uint8_t client[]);
};

#endif