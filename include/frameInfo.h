#ifndef FRAMEINFO_H
#define FRAMEINFO_H

class FrameInfo {
    public:
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
};

#endif