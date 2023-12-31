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
char BLETerm::command[INPUT_MAX_LEN];

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

//Match the command to the array
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

//Used so if there are flags, the full length of the command can still be passed
void BLETerm::parseCommand(char cmd[], int len, int fullLen) {
    Serial.print("Pass cmd: ");
    Serial.println(cmd);
    for(int i = 0; i < 9; i++) {
        if(memcmp(this->cmdArray[i], cmd, len-1) == 0) {
            Serial.println("Match Found");
            this->curCommand = i;
            if(len+1 < this->INPUT_MAX_LEN) {
                //copy into the command array so it can be read by the main functions
                Serial.print("Stored info: ");
                Serial.println(cmd);

                //memcpy(this->command, &cmd, fullLen);
                for(int j = 0; j < fullLen; j++) {
                    this->command[j] = cmd[j];
                }
                this->command[fullLen+1] = '\0';
            }
            break;
        }
        else {
            this->curCommand = -2; //value if command is not found
        }
    }    
}

void BLETerm::setSpoofAP(int ap) {
    this->spoofAP = ap;
}

int BLETerm::getSpoofAP() {
    return this->spoofAP;
}

//Return full command entered
char *BLETerm::getFullCommand() {
    
    return this->command;
}

void MyServerCallbacks::onConnect(BLEServer *pServer) {
    setDeviceConnected(true);
}

void MyServerCallbacks::onDisconnect(BLEServer *pServer) {
    setDeviceConnected(false);
}

//I fucked up the command handling, but don't feel like redoing it, so its going to be clunky
void MyCallbacks::onWrite(BLECharacteristic *pCharacteristic) {
    const char *rxValue = pCharacteristic->getValue().c_str();
    int rxValueLen = strlen(rxValue);

    Serial.println(rxValue);
    if(rxValueLen > 0) {
        char cmd[rxValueLen+1];
        int flagsExist = 0;
        int divider;
        
        //convert command to lowercase 
        for(int i = 0; i < rxValueLen; i++) {
            //look for space in command, indicating flags exist and no spaces exist at beginning of command
            if(rxValue[i] == ' ' && flagsExist != 1 && i > 0) {
                flagsExist = 1;
                divider = i;
                cmd[i] = rxValue[i];
            }
            else if(rxValue[i] == ' ') { //Get rid of any extra spaces
                delay(1);
            }
            //Get rid of control characters such as line breaks, and make sure tolower is not applied to an integer
            else if(flagsExist == 0 && rxValue[i] > 31 && isdigit(rxValue[i]) == 0) {
                cmd[i] = tolower(rxValue[i]);
            }
            else {
                cmd[i] = rxValue[i];
            }
            if(i == rxValueLen-1) {
                cmd[i+1] = '\0'; //Add null to end of string
            }
        }
        //print out full recieved command
        if(flagsExist == 0) {
            parseCommand(cmd, rxValueLen);
        }
        else {
            //send space location so function only compares the actual command, not flags
            parseCommand(cmd, divider, rxValueLen);
        }
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