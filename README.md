# ESP Wifi Clone
# Configuration
Run `https://github.com/legoses/esp-wifi-clone.git` to clone the repository 
Run `cd esp-wifi-clone` to switch to the directory that was created 
Run `git submodule init && git submodule update` to download submodules used with this project 
Change the CONFIG_SSID and CONFIG_PASS variables to the ssid and password you wish to use

# Usage
- Connect to the esp's network 
- Navigate to 192.168.4.1/webserial. This webpage contains the serial console you will use to issue commands 
- Start scanning for access point using `start-ap-scan`.
- Start scanning for clients using `start-client-scan`.
- List found acces points or clients with `list-ap` or `list-clients`.
- Select access point to impersonate with `select-ap <num>` where <num> is the number associated with the access point.
- If you want to select individual clients to deauth, use `select-clients <num1>,<num2>`. If you wish to select multiple clients, append them as a list seperated by commas.
- If you want to select all clients associated with the access point, use `select-all-clients`.

## Commands
    - `help` - Display help message
    - 