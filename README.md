# ESP Wifi Clone
# Configuration
Run `https://github.com/legoses/esp-wifi-clone.git` to clone the repository 
Run `cd esp-wifi-clone` to switch to the directory that was created 
Run `git submodule init && git submodule update` to download submodules used with this project 
Change the CONFIG_SSID and CONFIG_PASS variables to the ssid and password you wish to use

# Usage
- This program uses a BLE UART recieve commands. Download 'Bluefruit Connect' by adafruit 
or another app of your choice that supports uart
- Start scanning for access point using `scan-ap`.
- Start scanning for clients using `scan-client`.
- List found acces points or clients with `list-ap` or `list-clients`.
- Select access point to impersonate with `select-ap <num>` where <num> is the number associated with the access point.
- If you want to select individual clients to deauth, use `select-clients <num1>,<num2>`. If you wish to select multiple clients, append them as a list seperated by commas.
- If you want to select all clients associated with the access point, use `select-all-clients`.

## Commands
    - `help` - Display help message
    - `scan-ap` - Scan for access points
    - `scasn-client` - scan for clients and determine which access point they are connected to
    - `scan-stop` - Stop scanning
    - `list-ap` - List found access points
    - `list-clients` - List clients, as well as which access point they are connected to
    - `select-ap <num>` - Select the access point you wish to spoof.
    - `select-clients <1,2,...>` - Select individual clients to deauth
    - `select-all-clients` - Select all clients associated with your chosed access point