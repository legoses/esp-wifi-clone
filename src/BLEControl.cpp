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

/*
void BLETerm::configState(int newCmd) {
    if(newCmd != this->lastCommand) {
        this->lastCommand = newCmd;
        switch(newCmd) {
            case 0: {
                char msg[] = "Help placeholder";
                size_t msgSize = sizeof(msg) / sizeof(msg[0]);
                //bleTerm.sendMsg(msg, msgSize);
                break;
            }
            case 1: {
                configurePromisc(0); //init scan for AP
                char msg[] = "Starting AP Scan";
                size_t msgSize = sizeof(msg) / sizeof(msg[0]);
                //bleTerm.sendMsg(msg, msgSize);
                break;
            }
            case 2: {
                char msg[] = "Stopping Scan";
                size_t msgSize = sizeof(msg) / sizeof(msg[0]);
                //bleTerm.sendMsg(msg, msgSize);
                esp_wifi_set_promiscuous(false); //stop ap scan
                switchChan = 0;
                break;
            }
            case 3: {
                char msg[] = "Staring Client Scan";
                size_t msgSize = sizeof(msg) / sizeof(msg[0]);
                //bleTerm.sendMsg(msg, msgSize);
                configurePromisc(1); //start client scan
                switchChan = 2;
                channel = 0;
                break;
            }
            case 4: {
                char msg[] = "Listing AP";
                size_t msgSize = sizeof(msg) / sizeof(msg[0]);
                //bleTerm.sendMsg(msg, msgSize);
                //printSSIDWeb(); //print access points
                break;
            }
            case 5: {
                char msg[] = "Listing Clients";
                size_t msgSize = sizeof(msg) / sizeof(msg[0]);
                //bleTerm.sendMsg(msg, msgSize);
                //listClients();
                break;
            }
            case 6: {
                char msg[] = "Select AP";
                size_t msgSize = sizeof(msg) / sizeof(msg[0]);
                //bleTerm.sendMsg(msg, msgSize);
                //selectAP(data, len);
                break;
            }
            default: {
                char msg[] = "[Error] Command Not Found";
                size_t msgSize = sizeof(msg) / sizeof(msg[0]);
                //bleTerm.sendMsg(msg, msgSize);
            }
        }
    }
}
*/
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
            //Get rid of control characters such as line breaks
            else if(flagsExist == 0 && rxValue[i] > 31) {
                cmd[flagsExist][i] = tolower(rxValue[i]);
            }
            else if(flagsExist == 1 && rxValue[i] > 31) {
                cmd[flagsExist][i-divider] = tolower(rxValue[i]);
            }
        }
        Serial.println(rxValue);
        Serial.println(cmd[0]);
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