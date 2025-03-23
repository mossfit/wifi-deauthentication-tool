# Wifi Deauther Tool

**Disclaimer:** ⚠️

This tool is provided for educational and authorized penetration testing purposes only. Use it **only** on networks for which you have explicit permission to test. The developer is not responsible for any misuse or damage resulting from its use.

## Overview

It crafts and sends 802.11 deauthentication frames using raw sockets on Linux. A novel feature in this version is an optional flag to randomize the source MAC address for each packet (to simulate dynamic testing scenarios).

## Requirements

- **Operating System:** Linux (raw sockets and monitor mode required)
- **Compiler:** g++ (or any C++11 compliant compiler)
- **Privileges:** Root access is required to run the tool.
- **Wireless Interface:** Must be set to monitor mode (e.g., using `airmon-ng`).

## Building

1. Open a terminal.
2. Compile the code using g++ (ensure you have C++11 support):

   ```bash
   g++ -o deauther main.cpp -std=c++11

## Usage

The executable accepts the following parameters:

`-i <interface>:` Name of the wireless interface in monitor mode (e.g., `wlan0mon`).

`-a <AP MAC>`: MAC address of the Access Point.

`-t <Target MAC>`: MAC address of the target client (use `ff:ff:ff:ff:ff:ff` for broadcast deauthentication).

`-r`: Optional flag to randomize the source MAC address on each packet.

```bash
sudo ./deauther -i wlan0mon -a 00:11:22:33:44:55 -t ff:ff:ff:ff:ff:ff -r
```

## Ethical Use Notice⚠️

This tool is meant for testing and educational purposes. Unauthorized deauthentication attacks can be illegal and unethical. Always obtain proper authorization before testing any network. The developer is not responsible for any misuse or damage resulting from its use.
 

**License**

This tool is licensed under [MIT License](LICENSE)
