#include <Arduino.h>
#include "lvgl.h"      /* https://github.com/lvgl/lvgl.git */
#include "rm67162.h"
#include "esp_wifi.h"
#include <beaconFrame.h>
#include <apInfo.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
//#include <WebServer.h>
#include <WebSerial.h>


#if ARDUINO_USB_CDC_ON_BOOT != 1
#warning "If you need to monitor printed data, be sure to set USB CDC On boot to ENABLE, otherwise you will not see any data in the serial monitor"
#endif

#ifndef BOARD_HAS_PSRAM
#error "Detected that PSRAM is not turned on. Please set PSRAM to OPI PSRAM in ArduinoIDE"
#endif

//configure AP
const char *CONFIG_SSID = "hello";
const char *CONFIG_PASS = "311202-nv";

const int cmdLen = 32;
//function comparing client command to this array will return int containing the command position in the array
//Int will be used in switch case to determing next step
uint8_t cmdArray[][cmdLen] = {
                                    "help", //0
                                    "set-stage", //1 
                                    "get-stage",  //2
                                    "start", //3
                                    "stop", //4
                                    "list-ap", //5
                                    "list-clients", //6
                                    "select-ap", //7
                                    "select-clients", //8
                                    "select-all-clients" //9
                                    };

AsyncWebServer Server(80);

static lv_style_t clock_obj_style;
static lv_style_t sep_style;
lv_color_t blue = lv_color_hex(0x6f8bc6);
lv_color_t darkBlue = lv_color_hex(0x2e3440);
lv_color_t shadow = lv_color_hex(0x2f333d);

const uint8_t BEACON = 128;

const long offsetSec = 3600;
int channel = 1;
unsigned long curTime = millis();
int scanStart;
const int scanTime = 60000; // 1 minute
//Stage. 0 is scanning for beacons. 1 is scannign for clients
int currentMode = 0;

APInfo apInfo;
wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
wifi_promiscuous_filter_t filter;

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
    //volatile byte beaconArr = 0 << 8;

    //Read through first byte to determine if recieved packet is a beacon 
    /*
    for(int i = 7; i >= 0; i--) {
        byte bit = bitRead(mgmtPacket->payload[0], i);

        beaconArr = beaconArr | (bit << i+1);
    }

        128 stands for 10000000 in binary 
        1000 - first four digits are the frame subtype. In this case it is the beacon subtype
        00 - Middle numbers are the frame type.
        00 - Last two zeroes are the version. In this case, it means we are using 802.11

    */
    Serial.println("Beacon Detected");
    Serial.println();

    //Cast to class to more easily parse information
    BeaconFrame *beacon = (BeaconFrame*)mgmtPacket->payload;
    uint8_t *payload = beacon->getPayload();

    //Lenth of ssid tag
    uint8_t ssidLen = beacon->getSSIDLen();
    
    if(sizeof(payload) > 0) {
        //Get necessary info from beacon
        uint8_t ssid[ssidLen];
        beacon->getSSID(ssid, ssidLen);
        uint8_t bssid[6];
        beacon->getBSSID(bssid, 6);
        
        Serial.println();
        //Store ssid, bssid, and rssi to spoof later
        apInfo.addAP(ssid, bssid, rssi, channel, ssidLen);
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
                //Serial.print("Channel: ");
                //Serial.println(channel);
                Serial.println("Beacon");
                parseBeacon(mgmtPacket, rssi, channel);
            }
            break;
        case WIFI_PKT_DATA:
            Serial.println("Data pkt");
            scanForClient(mgmtPacket);
            break;
    }
}

void configAP() {
    if(!WiFi.softAP(CONFIG_SSID, CONFIG_PASS)) {
        Serial.println("Config wifi failed");
        while(1) {
            Serial.println("Config fialed");
        }
    }
}


/*
    Make this function disable wifi if it is already enabled
    Have it take a parameter to set the packet filter
*/
//Config wifi, change packet filter type depending on which stage program is in
int configWifi(int stage) {
    //Deinit wifi if it is active in order to change parameters
    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
    esp_wifi_deinit();

    switch(stage) {
        case 0:
            Serial.println("Setting filter to MGMT");
           filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
           break;
        case 1:
            Serial.println("Setting filter to DATA");
            filter.filter_mask = WIFI_PROMIS_FILTER_MASK_DATA;
            break;
    }

    if(esp_wifi_init(&config) != ESP_OK) {
        return -1;
    }

    if(esp_wifi_set_mode(WIFI_MODE_AP) != ESP_OK) {
        return -1;
    }

    if(esp_wifi_set_promiscuous_filter(&filter) != ESP_OK) {
        return -1;
    }

    if(esp_wifi_set_promiscuous_rx_cb(wifiCallback) != ESP_OK) {
        return -1;
    }
    
    if(esp_wifi_set_promiscuous(true) != ESP_OK) {
        return -1;
    }

    scanStart = millis();

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


int getCommand(uint8_t data[], int len) {
    for(int i = 0; i < len; i++) {
        if(memcmp(cmdArray[i], data, len) == 0) {
            return i;
        }
    }
    return -1;
}


void recvMsg(uint8_t *data, size_t len) {
    if(len <= cmdLen) {
        //convert command to lowercase
        
        uint8_t cmd[len];
        for(int i = 0; i < len; i++) {
            cmd[i] = data[i];
        }
        

        int cmdInt = getCommand(cmd, len);
        Serial.println(cmdInt);
    }
    

}


void setup()
{
    static lv_disp_drv_t disp_drv;
    
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t *buf;
    Serial.begin(115200);
    //Serup wifi access point
    Serial.println("Setting up Config Access Point");
    configAP();

    //Config web server
    Serial.println("Configuring Wifi");
    if(configWifi(currentMode) == -1) {
        while(true) {
            Serial.println("Unable to initialize wifi");
            delay(1000);
        }
    }
    esp_wifi_start();

    //Setup serial webpage
    WebSerial.begin(&Server);
    WebSerial.msgCallback(recvMsg);
    Server.begin();
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    rm67162_init(); // amoled lcd initialization

    lcd_setRotation(1);

    lv_init();

    buf = (lv_color_t *)ps_malloc(sizeof(lv_color_t) * LVGL_LCD_BUF_SIZE);
    assert(buf);

    lv_disp_drv_init(&disp_drv);
    /*Change the following line to your display resolution*/
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

}


void changeChannel() {
    if(millis() - curTime > 1000) {
        if(channel <= 12) {
            channel++;
            esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
        }
        else {
            channel = 0;
            esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
        }
        curTime = millis();
    }
}


void loop()
{
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    switch(currentMode) {
        case 1:
            changeChannel();
            if((millis() - scanStart > scanTime)) {
                //currentMode = 2;
                configWifi(currentMode);
            }
            break;
        case 2:
            changeChannel();
            if((millis() - scanStart > scanTime)) {
                //currentMode = 3;
            }

    }

    lv_timer_handler();
    delay(500);
}