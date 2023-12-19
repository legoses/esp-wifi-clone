#include <beaconFrame.h>

void BeaconFrame::getSenderAddress(uint8_t mac[], int len) {
    for(int i = 0; i < len; i++) {
        mac[i] = this->senderAddress[i];
    }

}

void BeaconFrame::getDestinationAddress(uint8_t mac[], int len) {
    for(int i = 0; i < len; i++) {
        mac[i] = this->destinationAddress[i];
    }
}

uint8_t *BeaconFrame::getPayload() {
    return this->payload;
}

void BeaconFrame::getBSSID(uint8_t bssid[], int len) {
    for(int i = 0; i < len; i++) {
        bssid[i] = this->bssid[i];
    }
}

void BeaconFrame::getSSID(uint8_t ssid[], uint8_t ssidLen) {
    for(int i = 2; i < ssidLen+2; i++) {
        ssid[i-2] = this->payload[i];
    }
}

uint8_t BeaconFrame::getSSIDLen() {
    return this->payload[1];
}