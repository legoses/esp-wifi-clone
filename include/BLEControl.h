#ifndef BLECONTROL_H
#define BLECONTROL_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>

#define SERVICE_UUID "9acec431-9e46-4e1d-a0be-3ad0da2be581"
#define CHARACTERISTIC_UUID_TX "1f85837c-0d9d-4792-84cc-509270622867"
#define CHARACTERISTIC_UUID_RX "b2b57a99-d93b-4f1e-9ed3-105db1a7a437"

class BLETerm {
    const static int cmdLen = 32;
    static char cmdArray[9][cmdLen];
    static bool deviceConnected; 
    static bool oldDeviceConnected; 
    static int lastCommand; 

    public:
    BLEServer *pServer = NULL;
    BLECharacteristic *pTxCharacteristic;
    void setDeviceConnected(bool status);
    bool getDeviceConnected();
    
    void setOldDeviceConnected(bool status);
    bool getOldDeviceConnected();

    void setLastCommand(int cmd);
    int getLastCommad();
};

class MyServerCallbacks : public BLEServerCallbacks, public BLETerm {
    void onConnect(BLEServer *pServer);
    void onDisconnect(BLEServer *pServer);
};

class MyCallbacks : public BLECharacteristicCallbacks, public BLETerm {
    void onWrite(BLECharacteristic *pCharacteristic);
};


void handleBLEConnection(void *pvParameter);
void setupBLE(BLETerm *bleTerm);

#endif