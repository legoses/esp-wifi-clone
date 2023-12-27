#include <BLEControl.h>


char BLETerm::cmdArray[9][cmdLen] = {
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
bool BLETerm::deviceConnected = false;
bool BLETerm::oldDeviceConnected = false;
int BLETerm::lastCommand = -1;

void BLETerm::setDeviceConnected(bool status) {
    this->deviceConnected = status;
}
bool BLETerm::getDeviceConnected() {
    return this->deviceConnected;
}

void BLETerm::setOldDeviceConnected(bool status) {
    this->oldDeviceConnected = status;
}
bool BLETerm::getOldDeviceConnected() {
    return this->oldDeviceConnected;
}

void BLETerm::setLastCommand(int cmd) {
    this->lastCommand = cmd;
}
int BLETerm::getLastCommad() {
    return this->lastCommand;
}

void MyServerCallbacks::onConnect(BLEServer *pServer) {
    setDeviceConnected(true);
}

void MyServerCallbacks::onDisconnect(BLEServer *pServer) {
    setDeviceConnected(false);
}

void MyCallbacks::onWrite(BLECharacteristic *pCharacteristic) {
    const char *rxValue = pCharacteristic->getValue().c_str();
    int rxValueLen = strlen(rxValue);

    if(rxValueLen > 0) {
        char cmd[2][rxValueLen];
        int flagsExist = 0;
        int divider;
        
        //convert command to lowercase 
        for(int i = 0; i < rxValueLen; i++) {
            //check if commanad has additional flags
            if(cmd[flagsExist][i] == 20 && flagsExist != 1) {
                flagsExist = 1;
                divider = i+1;
            }
            else if(flagsExist == 0) {
                cmd[flagsExist][i] = tolower(rxValue[i]);
            }
            else if(flagsExist == 1) {
                cmd[flagsExist][i-divider] = tolower(rxValue[i]);
            }
        }

        //record last command
        //DeviceStatus::setCommand(getCommand(cmd[0], rxValueLen));
    }
}

void handleBLEConnection(void *pvParameter) {
    BLETerm bleTerm = *((BLETerm*)pvParameter);
    while(true) {
        Serial.print("Device Connected: ");
        Serial.println(bleTerm.getDeviceConnected());
        delay(100);
    }
}

void setupBLE(BLETerm *bleTerm) { 
    BLEDevice::init("BLE Device");

    bleTerm->pServer = BLEDevice::createServer();
    bleTerm->pServer->setCallbacks(new MyServerCallbacks());
    //create ble service
    BLEService *pService = bleTerm->pServer->createService(SERVICE_UUID);
    //Create characteristic
    bleTerm->pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY
        );
    bleTerm->pTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE
    );

    pRxCharacteristic->setCallbacks(new MyCallbacks());
    //Start service
    pService->start();
    //Start advertising
    bleTerm->pServer->getAdvertising()->start();

    //start thread to handle ble
    xTaskCreate(
        handleBLEConnection,
        "Handle BLE",
        2048,
        &bleTerm,
        2,
        NULL
    );
}