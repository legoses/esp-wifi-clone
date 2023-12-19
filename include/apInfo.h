#ifndef APINFO_H
#define APINFO_H
#include <stdint.h>
#include <Arduino.h>


class APInfo {
    static const int numAP = 10;
    static const int numClient = 20;
    uint8_t SSID[numAP][32];
    uint8_t BSSID[numAP][6];
    signed rssi[numAP];
    uint8_t channel[numAP];
    uint8_t clients[numAP][numClient][6];
    int clientCount[numAP];
    int curNum;

    void copyInfo(uint8_t storeArr[], uint8_t copyArr[], int len);
    void printBSSID(uint8_t bssid[]);

    public:
    APInfo();
    uint8_t *getSSID(int num);
    uint8_t *getBSSID(int num);
    uint8_t getRssi(int num);
    int checkExisting(uint8_t bssid[]);
    void addAP(uint8_t ssid[], uint8_t bssid[], signed rssi, uint8_t channel, int len);
    uint8_t getChannel(int num);
    void addClient(uint8_t mac[], int pos);

};

#endif