#include <apInfo.h>

APInfo::APInfo() {
    //Init variable
    this->curNum = 0;

    //Init client count to all zeroes
    memset(this->clientCount, 0, 6);

    //Allocate memory for ssid array
    this->SSID = (uint8_t**)malloc(numAP * sizeof(uint8_t*));
    for(int i = 0; i < this->numAP; i++)
    {
        SSID[i] = (uint8_t*)malloc(this->SSID_LEN * sizeof(uint8_t));
    }
}

int APInfo::getClientCount(int ap) {
    if(ap < this->numAP) {
        return clientCount[ap];
    }

    return -1;
}

void APInfo::getClient(uint8_t store[], int ap, int client) {
    if(ap < this->curNum && this->clientCount[ap] < this->numClient) {
        store[0] = clients[ap][client][0];
        store[1] = clients[ap][client][1];
        store[2] = clients[ap][client][2];
        store[3] = clients[ap][client][3];
        store[4] = clients[ap][client][4];
        store[5] = clients[ap][client][5];
    }
}

void APInfo::addClient(uint8_t mac[], int pos) {
    //Grab the amount of clients currently stored for the selected AP
    int numClient = this->clientCount[pos];
    //Add client to array in same position as the AP, so it is easy to retrieve later
    if(numClient < this->numClient) {
        if(checkClientExist(mac, pos) == 0) {
            Serial.print("Assicoating AP ");
            printBSSID(this->BSSID[pos]);
            Serial.print(" and ");
            printBSSID(mac);
            Serial.println();

            copyInfo(clients[pos][numClient], mac, 6);
            //Serial.print(this->SSID[pos]);
            //Serial.print(" clients: ");
            //Serial.println(this->clientCount[pos]);
            this->clientCount[pos]++;

        }
    }
    else {
        Serial.println("Client list full");
    }
    
}

void APInfo::printBSSID(uint8_t bssid[]) {
    Serial.printf("%x:%x:%x:%x:%x:%x", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
}

void APInfo::copyInfo(uint8_t storeArr[], uint8_t copyArr[], int len) {
    for(int i = 0; i < len; i++) {
        storeArr[i] = copyArr[i];
    }
}

//Overload funciton to also accept chars
void APInfo::copyInfo(char storeArr[], char copyArr[], int len) {
    for(int i = 0; i < len; i++) {
        storeArr[i] = copyArr[i];
    }
    storeArr[len] = '\0'; //Add null terminator
}

int APInfo::getNumAP() {
    return this->curNum;
}

uint8_t **APInfo::getSSID() {
    return this->SSID;
}

uint8_t *APInfo::getBSSID(int num) {
    return this->BSSID[num];
}

//check if client has already been associated with an ap before adding
int APInfo::checkClientExist(uint8_t client[], int ap) {
    int clientCount = this->clientCount[ap]; //get number of clients currently associated with this ap
    for(int i = 0; i < clientCount; i++) {
        if(memcmp(this->clients[ap][i], client, 6) == 0) {
            Serial.println("Client already added");
            return -1;
        }
    }
    return 0;
}

//Check if bssid exists in array, and return position
int APInfo::checkExisting(uint8_t bssid[]) {
    //Serial.print("Comparing BSSID ");
    //printBSSID(bssid);
    Serial.println("and:");
    for(int i = 0; i < this->curNum; i++) {
        //Check if bssid is already stored
        //printBSSID(this->BSSID[i]);
        //Serial.println();
        if(memcmp(bssid, this->BSSID[i], 6) == 0) {
            //Serial.println("Memcpy worked");
            return i;
        }
    }
    //Serial.println("Memcpy did not work");
    return -1;
}

uint8_t APInfo::getChannel(int num) {
    return this->channel[num];
}

int APInfo::getSSIDLen(int num) {
    if(num < curNum) {
         return this->ssidLen[num];
    }
    return 0;
   
}

bool APInfo::addAP(uint8_t ssid[], uint8_t bssid[], signed rssi, uint8_t channel, int len) {
    //Make sure ssid length is 32 or less and number of stored AP is 10 or less
    if(checkExisting(bssid) == -1 && len < this->SSID_LEN && this->curNum < 10 && len > 0) {
        //Copy information to class variabes
        copyInfo(this->SSID[curNum], ssid, len);
        copyInfo(this->BSSID[curNum], bssid, 6);
        ssidLen[curNum] = len;
        this->rssi[this->curNum] = rssi;
        this->channel[this->curNum] = channel;
        
        for(int i = 0; i < len; i++) {
            Serial.printf("%c", this->SSID[curNum][i]);
        }
        Serial.println();
        //Keep track of AP stored
        this->curNum++;
        return true;
    }
    Serial.printf("Current AP: %i\n", curNum);
    return false;
}