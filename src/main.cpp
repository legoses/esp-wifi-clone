#include <Arduino.h>
#include "lvgl.h"      /* https://github.com/lvgl/lvgl.git */
#include "rm67162.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include <beaconFrame.h>
#include <apInfo.h>
#include <BLEControl.h>


#if ARDUINO_USB_CDC_ON_BOOT != 1
#warning "If you need to monitor printed data, be sure to set USB CDC On boot to ENABLE, otherwise you will not see any data in the serial monitor"
#endif

#ifndef BOARD_HAS_PSRAM
#error "Detected that PSRAM is not turned on. Please set PSRAM to OPI PSRAM in ArduinoIDE"
#endif

/*
    TODO:
    Store AP config so it can be configured and initialized seperatly
    Implement client select
    Change mac to chosen bssid
    Implement deauth functionality
*/

//configure AP
int switchChan = 0;

static lv_style_t clock_obj_style;
static lv_style_t sep_style;
lv_color_t blue = lv_color_hex(0x6f8bc6);
lv_color_t darkBlue = lv_color_hex(0x2e3440);
lv_color_t shadow = lv_color_hex(0x2f333d);

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

//Wifi config variables
wifi_config_t configAP;
wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
wifi_promiscuous_filter_t filter;
uint8_t deauthClients[50][6]; //Store client mac addresses
int numDeauthClients = 0;
int selectedApNum = -1; //If value is -1, an ap has not been selected

void configStyle() {
    lv_style_init(&clock_obj_style);
    lv_style_set_border_color(&clock_obj_style, blue);
    lv_style_set_border_width(&clock_obj_style, 2);
    lv_style_set_bg_color(&clock_obj_style, darkBlue);
    lv_style_set_shadow_width(&clock_obj_style, 10);
    lv_style_set_shadow_color(&clock_obj_style, shadow);
    lv_style_set_text_font(&clock_obj_style, &lv_font_montserrat_48);

    lv_style_remove_prop(&clock_obj_style, LV_STYLE_BORDER_COLOR);
}


void configSepStyle() {
    lv_style_init(&sep_style);
    lv_style_set_text_font(&sep_style, &lv_font_montserrat_48);
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
            //WebSerial.println("Access Point Found.");
        }
    }
    Serial.println();
}


void scanForClient(wifi_promiscuous_pkt_t *mgmtPacket) {
    BeaconFrame *beacon = (BeaconFrame*)mgmtPacket->payload;
    uint8_t destAddr[6];
    //Get destination mac from recived packet
    beacon->getDestinationAddress(destAddr, 6);
    //Get position mac is stored in
    int pos = apInfo.checkExisting(destAddr);
    if(pos != -1) {
        uint8_t sendAddr[6];
        beacon->getSenderAddress(sendAddr, 6);
        Serial.println("MAC exists");
        apInfo.addClient(sendAddr, pos);
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
        case WIFI_PKT_DATA:
            Serial.print("Data pkt first byte: ");
            if(mgmtPacket->payload[0] != 200) {
                Serial.println("Data pkt");
                scanForClient(mgmtPacket);
            }
            break;
        case WIFI_PKT_CTRL:
            Serial.println("ctrl packet");
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


void my_disp_flush(lv_disp_drv_t *disp,
                const lv_area_t *area,
                lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    lcd_PushColors(area->x1, area->y1, w, h, (uint16_t*)&color_p->full);
    lv_disp_flush_ready(disp);
}


void configurePromisc(int filterMask) {
    esp_wifi_set_promiscuous(false);

    switch(filterMask) {
        case 0:
            filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
            break;
        case 1:
            filter.filter_mask = WIFI_PROMIS_FILTER_MASK_DATA | WIFI_PROMIS_FILTER_MASK_CTRL;
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


void ssidToString(uint8_t ssid[], char newSSID[], int len) {
    //static char *newSSID = (char*)ssid;
    memcpy(newSSID, (char*)ssid, 32);
    newSSID[len] = '\0';
}


void printSSIDWeb() {
    int num = apInfo.getNumAP();
    uint8_t **ssidList = apInfo.getSSID();

    //WebSerial.println("---AP List---");
    //char msg[] = "---AP List---";
    sendMsg("---AP List---");
    int totalLen = apInfo.SSID_LEN;
    if(num > 0) {
        char apMsg[40];
        memset(apMsg, '\0', 40);
        for(int i = 0; i < num; i++) {
            //apMsg[0] = i+49; //Convert to ascii number so it prints properly
            //apMsg[1] = '.';
            //apMsg[2] = 20; //space character

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

    Serial.print("Full command");
    Serial.println(cmd);
    Serial.println(apSelect);
    //Subtract 1 so the value aligns with array value
    selectedApNum = apSelect-1;
}


void listClients() {
    int num = apInfo.getNumAP();
    uint8_t **ssidList = apInfo.getSSID();

    int selectedAP = 0;
    int clientCount = apInfo.getClientCount(selectedAP);
    uint8_t clientMac[6];

    //char *sendSSID = (char*)ssidList[selectedAP];
    //int ssidLen = apInfo.getSSIDLen(selectedAP);
    char sendSSID[32];
    ssidToString(ssidList[selectedAP], sendSSID, apInfo.getSSIDLen(selectedAP));
    //sendSSID[ssidLen] = '\0';
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


void configAPSpoof(uint8_t ssid[], int ssidLen, uint8_t bssid[], int channel) {
    Serial.println("Setting AP values");
    configAP.ap.channel = channel;
    configAP.ap.max_connection = 2;
    configAP.ap.authmode = WIFI_AUTH_OPEN;

    memcpy(configAP.ap.ssid, ssid, 32);
    configAP.ap.ssid_len = ssidLen;
    Serial.println("Initializing config");
    esp_wifi_set_config(WIFI_IF_AP, &configAP);
    esp_wifi_set_mac(WIFI_IF_AP, bssid);
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


void startAPSpoof() {
    esp_wifi_start();

    //esp_wifi_80211_tx();
}


void setup()
{
    //static lv_disp_drv_t disp_drv;
    
    //static lv_disp_draw_buf_t draw_buf;
    //static lv_color_t *buf;
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
    ////bleTerm.begin();

    //Setup serial webpage
    //WebSerial.begin(&Server);
    //WebSerial.msgCallback(recvMsg);
    //Server.begin();
    //Serial.print("IP Address: ");
    //Serial.println(WiFi.softAPIP());

    //rm67162_init(); // amoled lcd initialization

    //lcd_setRotation(1);

    //lv_init();
    /*

    buf = (lv_color_t *)ps_malloc(sizeof(lv_color_t) * LVGL_LCD_BUF_SIZE);
    assert(buf);

    lv_disp_drv_init(&disp_drv);
    //Sets resolution
    disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, LVGL_LCD_BUF_SIZE);
    //Implement function that can copy an image to the display
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    short int sepWidth = 30;
    short int numWidth = ((EXAMPLE_LCD_H_RES - sepWidth) / 2) - 30;
    short int contHeight = EXAMPLE_LCD_V_RES - 35;

    static lv_coord_t column_dsc[] = {numWidth, sepWidth, numWidth, LV_GRID_TEMPLATE_LAST}; //3 100 px wide columns
    static lv_coord_t row_dsc[] = {contHeight, LV_GRID_TEMPLATE_LAST}; // 1 100 pixel tall row

    //Create objects to be used
    lv_obj_t *grid = lv_obj_create(lv_scr_act());
    
    lv_obj_t *hourCont = lv_obj_create(grid);
    lv_obj_t *minuteCont = lv_obj_create(grid);

    lv_obj_t *hour = lv_label_create(hourCont);
    lv_obj_t *seperator = lv_label_create(grid);
    lv_obj_t *minute = lv_label_create(minuteCont);
    
    lv_obj_set_style_grid_column_dsc_array(grid, column_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(grid, row_dsc, 0);
    lv_obj_set_size(grid, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    lv_obj_center(grid);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    lv_obj_set_align(grid, LV_ALIGN_CENTER);

    lv_obj_set_grid_cell(hourCont, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_set_grid_cell(seperator, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_grid_cell(minuteCont, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    lv_label_set_text_fmt(hour, "test");
    lv_label_set_text_fmt(seperator, ":");
    lv_label_set_text_fmt(minute, "minute");

    lv_obj_center(minute);
    lv_obj_center(seperator);
    lv_obj_center(hour);
    configStyle();
    configSepStyle();
    lv_obj_add_style(hourCont, &clock_obj_style, LV_PART_MAIN);
    lv_obj_add_style(minuteCont, &clock_obj_style, LV_PART_MAIN);
    lv_obj_add_style(seperator, &sep_style, LV_PART_MAIN);
    Serial.println("Display initialized");
    */
}


void configState() {
    int cmd = bleTerm.getCommand();
    Serial.printf("Command: %i\n", cmd);
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
                int ssidLen = apInfo.getSSIDLen(selectedApNum);

                ssidToString(ssidList[selectedApNum], charSSID, selectedApNum);
                Serial.print("Cur num: ");
                Serial.println(selectedApNum);
                if(selectedApNum < apInfo.getNumAP()) {
                    uint8_t *bssid = apInfo.getBSSID(selectedApNum);
                    char msg[50];
                    snprintf(msg, 50, "AP '%s' Selected.", charSSID);
                    sendMsg(msg);
                    
                    //Store info to spoof ap
                    configAPSpoof(ssidList[selectedApNum], apInfo.getSSIDLen(selectedApNum), bssid, apInfo.getChannel(selectedApNum));
                }
                else {
                    sendMsg("Selected AP does not exist");
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
                if(selectedApNum != -1 && numDeauthClients > 0) {
                    sendMsg("Starting attack");
                    startAPSpoof();
                }
                else if(selectedApNum == -1) {
                    sendMsg("[Error] An an access point must be selected");
                }
                else if(numDeauthClients == 0) {
                    sendMsg("No clients selected. Defaulting to all.");
                    selectAllDeauthClients();
                    startAPSpoof();
                }
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

            if(millis() - curTime > 1000) {
                if(channel < apNum) {
                    esp_wifi_set_channel(apInfo.getChannel(channel), WIFI_SECOND_CHAN_NONE);
                    channel++;
                    Serial.printf("Current Channel: %i\n", channel);
                }
                else {
                    channel = 0;
                    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
                }
                curTime = millis();
            }
    }
    //lv_timer_handler();
    delay(500);
}