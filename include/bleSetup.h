#ifndef BLESETUP_H
#define BLESETUP_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID "9acec431-9e46-4e1d-a0be-3ad0da2be581"
#define CHARACTERISTIC_UUID_TX "1f85837c-0d9d-4792-84cc-509270622867"
#define CHARACTERISTIC_UUID_RX "b2b57a99-d93b-4f1e-9ed3-105db1a7a437"

class DeviceStatus {
    int lastCmd = -1;
    static const int cmdLen = 32;

    const char cmdArray[9][cmdLen] = {
        "help", //0
        "scan-ap-start", //1 
        "scan-stop", //2
        "scan-client-start", //3
        "list-ap", //4
        "list-clients", //5
        "select-ap", //6
        "select-clients", //7
        "select-all-clients" //8
        };

    public:
    static bool deviceConnected;
    static bool oldDeviceConnected;
    int getCommand(char data[], int len);
    int getLastCommand();
    void setCommand(int cmd);
};

class MyServerCallbacks : public BLEServerCallbacks, public DeviceStatus {
    void onConnect(BLEServer *pServer);
    void onDisconnect(BLEServer *pServer);
};

class MyCallbacks : public BLECharacteristicCallbacks, public DeviceStatus {
    void onWrite(BLECharacteristic *pCharacteristic);
};

class BLETerm : public DeviceStatus {
    static uint8_t tx_value;
    int curCmd = -1;
    BLEServer *pServer;
    BLECharacteristic *pTxCharacteristic;

    public:
    void begin();
    int connectionStatus();
    void sendMsg(char data[], size_t len);
    void startAdvertising();
};

#endif