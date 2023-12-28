#include <BLEControl.h>


char BLETerm::cmdArray[9][cmdLen] = {
    "help", //0
    "scan-ap", //1 
    "scan-stop", //2
    "scan-client", //3
    "list-ap", //4
    "list-clients", //5
    "select-ap", //6
    "select-clients", //7
    "select-all-clients" //8
    };
bool BLETerm::deviceConnected = false;
bool BLETerm::oldDeviceConnected = false;
int BLETerm::curCommand = -1;

void BLETerm::setDeviceConnected(bool status) {
    this->deviceConnected = status;
}
bool BLETerm::getDeviceConnected() {
    return this->deviceConnected;
}

int BLETerm::getCommand() {
    return this->curCommand;
}

void BLETerm::resetCommand() {
    this->curCommand = -1;
}

void BLETerm::setOldDeviceConnected(bool status) {
    this->oldDeviceConnected = status;
}
bool BLETerm::getOldDeviceConnected() {
    return this->oldDeviceConnected;
}

void BLETerm::parseCommand(char cmd[], int len) {
    for(int i = 0; i < 9; i++) {
        if(memcmp(this->cmdArray[i], cmd, len-1) == 0) {
            Serial.println("Match Found");
            this->curCommand = i;
            break;
        }
        else {
            this->curCommand = -2; //value if command is not found
        }
    }    
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

    Serial.println(rxValue);
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
            //Get rid of control characters such as line breaks
            else if(flagsExist == 0 && rxValue[i] > 31) {
                cmd[flagsExist][i] = tolower(rxValue[i]);
            }
            else if(flagsExist == 1 && rxValue[i] > 31) {
                cmd[flagsExist][i-divider] = tolower(rxValue[i]);
            }
        }
        BLETerm::parseCommand(cmd[0], rxValueLen);
    }
}


void handleBLEConnection(void *pvParameter) {
    BLETerm bleTerm = *((BLETerm*)pvParameter);
    uint8_t txVal = 1;
    while(true) {
        bool device = bleTerm.getDeviceConnected();
        bool oldDevice = bleTerm.getOldDeviceConnected();

        if(!device && oldDevice) {
            bleTerm.pServer->startAdvertising();
            Serial.println("Start advertising");
            bleTerm.setOldDeviceConnected(device);
        }

        if(device && !oldDevice) {
            bleTerm.setOldDeviceConnected(device);
        }
        delay(1000);
    }
}

void setupBLE(BLETerm &bleTerm) { 
    Serial.println("Ble init");
    BLEDevice::init("BLE Device");

    Serial.println("Creating ble server");
    bleTerm.pServer = BLEDevice::createServer();
    Serial.println("Setting server callback");
    bleTerm.pServer->setCallbacks(new MyServerCallbacks());
    //create ble service
    Serial.println("Creating ble service");
    BLEService *pService = bleTerm.pServer->createService(SERVICE_UUID);
    //Create characteristic
    Serial.println("Creating ble characteristic");
    bleTerm.pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY
        );
    bleTerm.pTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE
    );

    Serial.println("Setting callvbacks");
    pRxCharacteristic->setCallbacks(new MyCallbacks());
    //Start service
    pService->start();
    //Start advertising
    bleTerm.pServer->getAdvertising()->start();

    Serial.println("Creating task");
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