#include <bleSetup.h>

bool DeviceStatus::deviceConnected = false;
bool DeviceStatus::oldDeviceConnected = false;
uint8_t BLETerm::tx_value = 0;

void MyServerCallbacks::onConnect(BLEServer *pServer) {
    this->deviceConnected = true;
}

void MyServerCallbacks::onDisconnect(BLEServer *pServer) {
    this->deviceConnected = false;
}


int DeviceStatus::getCommand(char data[], int len) {
    for(int i = 0; i < len; i++) {
        if(memcmp(cmdArray[i], data, len) == 0) {
            return i;
        }
    }
    return -1;
}

void DeviceStatus::setCommand(int cmd) {
    this->lastCmd = cmd;
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
        DeviceStatus::setCommand(getCommand(cmd[0], rxValueLen));
    }
}

int DeviceStatus::getLastCommand() {
    return this->lastCmd;
}

//Transmit data to client
void BLETerm::sendMsg(char data[], size_t len) {
    uint8_t *message = (uint8_t*)(data[0]);
    pTxCharacteristic->setValue(message, len);
    pTxCharacteristic->notify();
}

void BLETerm::begin() {
    //Create ble device
    BLEDevice::init("BLE Device");

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    //create ble service
    BLEService *pService = pServer->createService(SERVICE_UUID);
    //Create characteristic
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY
        );
    pTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE
    );

    pRxCharacteristic->setCallbacks(new MyCallbacks());
    //Start service
    pService->start();
    //Start advertising
    pServer->getAdvertising()->start();
}

void BLETerm::startAdvertising() {
        pServer->startAdvertising();
        oldDeviceConnected = deviceConnected;
}

int BLETerm::connectionStatus() {
    //Check for disconnected
    if(!deviceConnected && oldDeviceConnected) {
        return 0;
    }
    //Check for new connection
    if(deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
        return 1;
    }

    return -1;
}