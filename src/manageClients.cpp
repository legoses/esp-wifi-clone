#include <manageClients.h>

bool ConnectedClients::checkIfClientExists(uint8_t client[]) {
    for(int i = 0; i < this->numConnectedClients; i++) {
        if(memcmp(client, this->connectedClients[i], 6) == 0) {
            return true;
        }
    }
    return false;
}

void ConnectedClients::addClient(uint8_t client[]) {
    if(this->numConnectedClients < numClient) {
        memcpy(this->connectedClients[numConnectedClients], client, 6);
        this->numConnectedClients++;
    }
}

void ConnectedClients::removeClient(uint8_t client[]) {
    for(int i = 0; i < this->numConnectedClients; i++) {
        if(memcmp(client, this->connectedClients[i], 6) == 0) {
            for(int j = i; j < this->numConnectedClients-1; j++) {
               memcpy(this->connectedClients[j], this->connectedClients[j+1], 6); 
            }
            this->numConnectedClients--;
        }
    }
}

int ConnectedClients::getNumConnectedClients() {
    return this->numConnectedClients;
}