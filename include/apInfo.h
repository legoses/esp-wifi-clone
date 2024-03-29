#ifndef APINFO_H
#define APINFO_H
#include <stdint.h>
#include <Arduino.h>
#include <NumConfig.h>


class APInfo : public ScanConstant {
    //static const int numAP = 20;
    //static const int numClient = 40;
    uint8_t **SSID;
    int ssidLen[numAP];
    uint8_t BSSID[numAP][6];
    signed rssi[numAP];
    uint8_t channel[numAP];
    uint8_t clients[numAP][numClient][6]; //mac address of clients
    int clientCount[numAP]; //stores amount of clients detected for an access point
    int curNum;

    void copyInfo(uint8_t storeArr[], uint8_t copyArr[], int len);
    void copyInfo(char storeArr[], char copyArr[], int len);
    void printBSSID(uint8_t bssid[]);

    public:
    const int SSID_LEN = 32;
    APInfo();
    uint8_t **getSSID();
    uint8_t *getBSSID(int num);
    uint8_t getRssi(int num);
    int checkExisting(uint8_t bssid[]);
    bool addAP(uint8_t ssid[], uint8_t bssid[], signed rssi, uint8_t channel, int len);
    uint8_t getChannel(int num);
    void addClient(uint8_t mac[], int pos);
    int getNumAP();
    void getClient(uint8_t store[], int ap, int client); //return selected client mac for selected ap
    int getClientCount(int ap); //get number of clients connected to an ap
    int checkClientExist(uint8_t client[], int ap);
    int getSSIDLen(int num);
};

#endif