#ifndef BLECONTROL_H
#define BLECONTROL_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>
#include <NumConfig.h>

//Nordic UART service uuid. Do no change
//https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/bluetooth_services/services/nus.html
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class BLETerm : public ScanConstant {
    //const static int INPUT_MAX_LEN = 100;
    //const static int cmdLen = 32;
    static char cmdArray[NUM_COMMANDS][cmdLen];
    static bool deviceConnected; 
    static bool oldDeviceConnected; 
    static int curCommand;
    static char command[INPUT_MAX_LEN];
    static int userCmdLen;
    static int spoofAP;

    public:
    BLEServer *pServer = NULL;
    BLECharacteristic *pTxCharacteristic;
    int getCommand();
    void resetCommand();
    char *getFullCommand();
    int getCommandLength();
    void setCommandLength(int cmdLen);
    
    void setDeviceConnected(bool status);
    bool getDeviceConnected();
    
    void setOldDeviceConnected(bool status);
    bool getOldDeviceConnected();

    void configState(int newCmd);

    void parseCommand(char cmd[], int len);
    void parseCommand(char cmd[], int len, int fullLen);
    void setSpoofAP(int ap);
    int getSpoofAP();
};

class MyServerCallbacks : public BLEServerCallbacks, public BLETerm {
    void onConnect(BLEServer *pServer);
    void onDisconnect(BLEServer *pServer);
};

class MyCallbacks : public BLECharacteristicCallbacks, public BLETerm {
    void onWrite(BLECharacteristic *pCharacteristic);
};

void handleBLEConnection(void *pvParameter);
void setupBLE(BLETerm &bleTerm);

#endif