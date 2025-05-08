#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT_IN_1_DEFAULT  9000  // Gauges input port from BeamNG (default)
#define PORT_OUT_1        4444  // Gauges output port to DK software
#define PORT_IN_2_DEFAULT  9001  // Motion input port from BeamNG (default)
#define PORT_OUT_2        4445  // Motion output port to DK software
#define BUFFER_SIZE 1024
#define GAUGES_MIN_SIZE 20  // Minimum bytes for Gauges stream
#define MOTION_MIN_SIZE 60  // Minimum bytes for Motion stream
#define MS_TO_MPH 2.23694  // Conversion factor from m/s to mph
#define MS_TO_KMH 3.6      // Conversion factor from m/s to km/h

int debug = 0;         // Debug flag: 1 to show UDP data, 0 to hide (default off)
int direct_listen = 0; // Direct listen flag: 1 to listen on 4444/4445, 0 to forward from 9000/9001 (default off)

void error_exit(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// Function to print buffer contents in hex
void print_hex_dump(const char* buffer, int len) {
    printf("Hex dump: ");
    for (int i = 0; i < len; i++) {
        printf("%02X ", (unsigned char)buffer[i]);
    }
    printf("\n");
}

// Function to extract float from little-endian bytes
float get_little_endian_float(const char* buffer, int offset) {
    union {
        float f;
        unsigned char b[4];
    } u;
    u.b[0] = (unsigned char)buffer[offset];
    u.b[1] = (unsigned char)buffer[offset + 1];
    u.b[2] = (unsigned char)buffer[offset + 2];
    u.b[3] = (unsigned char)buffer[offset + 3];
    return u.f;
}

int main() {
    WSADATA wsa;
    SOCKET sock_recv1, sock_recv2, sock_send1, sock_send2;
    struct sockaddr_in server_addr1, server_addr2, client_addr, local_forward1, local_forward2;
    char buffer[BUFFER_SIZE];
    int recv_len, client_len = sizeof(client_addr);
    int port_in_1, port_in_2;

    // Set debug mode
    char input[2];
    printf("Enable debug output? (1 for yes, 0 for no): ");
    scanf_s("%1s", input, (unsigned)_countof(input));
    if (input[0] == '1') {
        debug = 1;
        printf("Debug mode enabled\n");
    }
    else {
        debug = 0;
        printf("Debug mode disabled\n");
    }

    // Set direct listen mode
    printf("Listen directly to ports 4444/4445? (1 for yes, 0 for no): ");
    scanf_s("%1s", input, (unsigned)_countof(input));
    if (input[0] == '1') {
        direct_listen = 1;
        port_in_1 = PORT_OUT_1;  // 4444
        port_in_2 = PORT_OUT_2;  // 4445
        printf("Direct listen mode enabled (ports 4444 and 4445)\n");
    }
    else {
        direct_listen = 0;
        port_in_1 = PORT_IN_1_DEFAULT;  // 9000
        port_in_2 = PORT_IN_2_DEFAULT;  // 9001
        printf("Forwarding mode enabled (9000->4444, 9001->4445)\n");
    }

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        error_exit("WSAStartup failed");
    }

    // Create UDP sockets for receiving
    sock_recv1 = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_recv1 == INVALID_SOCKET) {
        error_exit("Socket 1 creation failed");
    }
    sock_recv2 = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_recv2 == INVALID_SOCKET) {
        error_exit("Socket 2 creation failed");
    }

    // Configure receiving addresses
    server_addr1.sin_family = AF_INET;
    server_addr1.sin_addr.s_addr = INADDR_ANY;
    server_addr1.sin_port = htons(port_in_1);

    server_addr2.sin_family = AF_INET;
    server_addr2.sin_addr.s_addr = INADDR_ANY;
    server_addr2.sin_port = htons(port_in_2);

    // Bind sockets
    if (bind(sock_recv1, (struct sockaddr*)&server_addr1, sizeof(server_addr1)) == SOCKET_ERROR) {
        error_exit("Bind 1 failed");
    }
    if (bind(sock_recv2, (struct sockaddr*)&server_addr2, sizeof(server_addr2)) == SOCKET_ERROR) {
        error_exit("Bind 2 failed");
    }

    // Create sending sockets only if forwarding (not direct listening)
    if (!direct_listen) {
        sock_send1 = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_send1 == INVALID_SOCKET) {
            error_exit("Send socket 1 creation failed");
        }
        sock_send2 = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_send2 == INVALID_SOCKET) {
            error_exit("Send socket 2 creation failed");
        }

        // Configure local forwarding addresses
        local_forward1.sin_family = AF_INET;
        inet_pton(AF_INET, "127.0.0.1", &local_forward1.sin_addr);
        local_forward1.sin_port = htons(PORT_OUT_1);

        local_forward2.sin_family = AF_INET;
        inet_pton(AF_INET, "127.0.0.1", &local_forward2.sin_addr);
        local_forward2.sin_port = htons(PORT_OUT_2);
    }

    printf("Listening for UDP packets on ports %d (Gauges) and %d (Motion)...\n", port_in_1, port_in_2);

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock_recv1, &readfds);
        FD_SET(sock_recv2, &readfds);

        int max_sd = (sock_recv1 > sock_recv2) ? sock_recv1 : sock_recv2;

        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            if (debug) printf("select failed: %d\n", WSAGetLastError());
            continue;
        }

        // Check Gauges socket
        if (FD_ISSET(sock_recv1, &readfds)) {
            recv_len = recvfrom(sock_recv1, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_len);
            if (recv_len != SOCKET_ERROR) {
                if (recv_len < GAUGES_MIN_SIZE) {
                    if (debug) printf("Gauges packet too small: %d bytes (minimum %d)\n", recv_len, GAUGES_MIN_SIZE);
                    continue;
                }
                if (debug) {
                    float speed_ms = get_little_endian_float(buffer, 12);
                    float speed_mph = speed_ms * MS_TO_MPH;
                    float speed_kmh = speed_ms * MS_TO_KMH;
                    float rpm = get_little_endian_float(buffer, 16);
                    printf("\n=== Port %d (Gauges) Received ===\n", port_in_1);
                    printf("Length: %d bytes\n", recv_len);
                    printf("Vehicle Speed: %.2f mph (%.2f km/h) (offset 12-15)\n", speed_mph, speed_kmh);
                    printf("Engine RPM: %.2f (offset 16-19)\n", rpm);
                    print_hex_dump(buffer, recv_len);
                }

                if (!direct_listen) {
                    if (sendto(sock_send1, buffer, recv_len, 0, (struct sockaddr*)&local_forward1, sizeof(local_forward1)) != SOCKET_ERROR) {
                        if (debug) printf("Forwarded to local port %d\n", PORT_OUT_1);
                    }
                    else {
                        if (debug) printf("Failed to forward to port %d: %d\n", PORT_OUT_1, WSAGetLastError());
                    }
                }
            }
        }

        // Check Motion socket
        if (FD_ISSET(sock_recv2, &readfds)) {
            recv_len = recvfrom(sock_recv2, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_len);
            if (recv_len != SOCKET_ERROR) {
                if (recv_len < MOTION_MIN_SIZE) {
                    if (debug) printf("Motion packet too small: %d bytes (minimum %d)\n", recv_len, MOTION_MIN_SIZE);
                    continue;
                }
                if (debug) {
                    float sway = get_little_endian_float(buffer, 28);
                    float surge = get_little_endian_float(buffer, 32);
                    float heave = get_little_endian_float(buffer, 36);
                    float roll = get_little_endian_float(buffer, 52);
                    float pitch = get_little_endian_float(buffer, 56);
                    printf("\n=== Port %d (Motion) Received ===\n", port_in_2);
                    printf("Length: %d bytes\n", recv_len);
                    printf("Sway Accel: %.2f m/s² (offset 28-31)\n", sway);
                    printf("Surge Accel: %.2f m/s² (offset 32-35)\n", surge);
                    printf("Heave Accel: %.2f m/s² (offset 36-39)\n", heave);
                    printf("Roll Pos: %.2f rad (offset 52-55)\n", roll);
                    printf("Pitch Pos: %.2f rad (offset 56-59)\n", pitch);
                    print_hex_dump(buffer, recv_len);
                }

                if (!direct_listen) {
                    if (sendto(sock_send2, buffer, recv_len, 0, (struct sockaddr*)&local_forward2, sizeof(local_forward2)) != SOCKET_ERROR) {
                        if (debug) printf("Forwarded to local port %d\n", PORT_OUT_2);
                    }
                    else {
                        if (debug) printf("Failed to forward to port %d: %d\n", PORT_OUT_2, WSAGetLastError());
                    }
                }
            }
        }
    }

    // Cleanup
    closesocket(sock_recv1);
    closesocket(sock_recv2);
    if (!direct_listen) {
        closesocket(sock_send1);
        closesocket(sock_send2);
    }
    WSACleanup();

    return 0;
}