#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #define CLOSE_SOCKET(s) closesocket(s)
    #define SOCKET_ERROR (-1)
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET (-1)
    #define SOCKET_ERROR (-1)
    #define CLOSE_SOCKET(s) close(s)
#endif

#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "ws2_32.lib") // 链接 Winsock 库

#define SERVER_PORT 9 // WOL 默认端口

void create_magic_packet(const unsigned char *mac, unsigned char *packet) {
    // 前 6 个字节填充 0xFF
    memset(packet, 0xFF, 6);

    // 后续 16 组 MAC 地址
    for (int i = 0; i < 16; i++) {
        memcpy(packet + 6 + i * 6, mac, 6);
    }
}

int parse_mac_address(const char *mac_str, unsigned char *mac) {
    int values[6];
    if (sscanf(mac_str, "%x:%x:%x:%x:%x:%x",
               &values[0], &values[1], &values[2],
               &values[3], &values[4], &values[5]) != 6) {
        return -1; // Invalid MAC address format
    }
    for (int i = 0; i < 6; i++) {
        mac[i] = (unsigned char)values[i];
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        printf("Usage: %s <MAC address> <Broadcast IP> [Local IP]\n", argv[0]);
        return 1;
    }

    const char *mac_str = argv[1];
    const char *broadcast_ip = argv[2];

    WSADATA wsaData;
    SOCKET sockfd;
    struct sockaddr_in server_addr;
    unsigned char magic_packet[102]; // WOL 魔术包
    unsigned char mac_address[6];

    // Get local IP from command line or use default
    const char* local_ip = (argc > 3) ? argv[3] : "192.168.1.251";

    // 解析 MAC 地址
    if (parse_mac_address(mac_str, mac_address) != 0) {
        printf("Invalid MAC address format. Use format: xx:xx:xx:xx:xx:xx\n");
        return 1;
    }

    // 生成魔术包
    create_magic_packet(mac_address, magic_packet);

    // 初始化 Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    // 创建 UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Bind to specific interface
    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = 0;
    if (inet_pton(AF_INET, local_ip, &bind_addr.sin_addr) != 1) {
        printf("Invalid local IP address: %s\n", local_ip);
        CLOSE_SOCKET(sockfd);
        WSACleanup();
        return 1;
    }

    // Bind to the interface
    if (bind(sockfd, (struct sockaddr*)&bind_addr, sizeof(bind_addr)) < 0) {
        printf("Failed to bind socket: %d\n", WSAGetLastError());
        CLOSE_SOCKET(sockfd);
        WSACleanup();
        return 1;
    }

    // Set up broadcast address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9);
    
    if (inet_pton(AF_INET, broadcast_ip, &addr.sin_addr) != 1) {
        printf("Invalid broadcast address: %s\n", broadcast_ip);
        CLOSE_SOCKET(sockfd);
        WSACleanup();
        return 1;
    }

    // Send packet multiple times
    for (int i = 0; i < 3; i++) {
        int sent = sendto(sockfd, (const char*)magic_packet, sizeof(magic_packet), 0,
                         (struct sockaddr*)&addr, sizeof(addr));
        if (sent == SOCKET_ERROR) {
            printf("Failed to send packet (attempt %d): %d\n", i + 1, WSAGetLastError());
            continue;
        }
        printf("Sent %d bytes to %s (attempt %d)\n", sent, broadcast_ip, i + 1);
        Sleep(100);
    }

    printf("Attempted to send Wake-on-LAN packet to MAC: %s via %s\n", mac_str, broadcast_ip);

    // 关闭 socket
    CLOSE_SOCKET(sockfd);
    WSACleanup();
    return 0;
}
