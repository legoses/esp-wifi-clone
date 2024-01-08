#include <spoofAP.h>

const int SpoofAP::maxClients = 50;

void SpoofAP::setSpoofAP(char ssid[], uint8_t bssid[], int apLen, int channel) {
    memcpy(this->SSID, ssid, apLen);
    memcpy(this->BSSID, bssid, sizeof(uint8_t)*6);
    this->channel = channel;
}

int SpoofAP::getChannel() {
    return this->channel;
}

void SpoofAP::getSSID(char ssid[]) {
    memcpy(ssid, this->SSID, 32);
}

void SpoofAP::getBSSID(uint8_t bssid[]) {
    memcpy(bssid, this->BSSID, sizeof(uint8_t)*6);
}

void SpoofAP::setSSIDNum(int a) {
    this->ssidNum = a;
}

int SpoofAP::getSSIDNum() {
    return this->ssidNum;
}