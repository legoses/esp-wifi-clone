#ifndef SPOOFAP_H
#define SPOOFAP_H
#include <stdint.h>

class SpoofAP {
    const int maxClients = 50;
    char SSID[32];
    uint8_t BSSID[6];

    public:
    void configEvilAP(char ssid[], uint8_t bssid[]);

};

#endif