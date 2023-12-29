#include <spoofAP.h>

void SpoofAP::configEvilAP(char ssid[], uint8_t bssid[]) {
    memcpy(this->SSID, ssid, 32);
    memcpy(this->BSSID, bssid, sizeof(uint8_t)*6);
}