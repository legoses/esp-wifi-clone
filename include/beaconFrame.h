#ifndef BEACON_FRAME_H
#define BEACON_FRAME_H
#include <stdint.h>


class BeaconFrame {
    uint16_t frameControl;
    uint16_t duration;
    uint8_t destinationAddress[6];
    uint8_t senderAddress[6];
    uint8_t bssid[6];
    uint16_t seqCtrl;
    uint8_t timestamp[8];
    uint16_t beaconInterval;
    uint16_t capabilityInfo;
    uint8_t payload[];

    public:
    void getSenderAddress(uint8_t bssid[], int len);
    void getDestinationAddress(uint8_t bssid[], int len);
    uint8_t *getPayload();
    void getBSSID(uint8_t bssid[], int len);
    void getSSID(char ssid[], uint8_t ssidLen);
    uint8_t getSSIDLen();
}__attribute__((packed));

#endif