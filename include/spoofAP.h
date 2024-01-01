#ifndef SPOOFAP_H
#define SPOOFAP_H
#include <stdint.h>
#include <string.h>

class SpoofAP {
    static const int maxClients;
    char SSID[32];
    uint8_t BSSID[6];
    uint8_t client[50][6];
    int channel;

    public:
    void setSpoofAP(char ssid[], uint8_t bssid[], int len, int channel);
    int getChannel();
    void getSSID(char ssid[]);
    void getBSSID(uint8_t bssid[]);
};

#endif