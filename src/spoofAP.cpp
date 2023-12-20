#include <spoofAP.h>

void SpoofAP::configEvilAP(char ssid[], uint8_t bssid[]) {
    int i = 0;
    while(ssid[i] != '\0') {
        this->SSID[i] = ssid[i];
        if(i < 6) {
            this->BSSID[i] = bssid[i];
        }
        i++;
    }
    //finish copying over bssid if it did not complete
    for(int j = i; j < 6; j++) {
        this->BSSID[j] = bssid[j];
    }
}