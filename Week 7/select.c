// select.c  --  Demo of using select() to handle multiple clients
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>

#define MAXCLIENTS 30
#define BUFFER_SIZE 1024
#define DEFAULT_PORT 8080

int main(int argc, char *argv[]) {
    int listening_socket, new_socket, client_sockets[MAXCLIENTS], sd, max_sd;
    int activity, addrlen, valread;
    struct sockaddr_in address;
    fd_set readfds;
    char buffer[BUFFER_SIZE];

    for (int i = 0; i < MAXCLIENTS; i++) {
        client_sockets[i] = 0;
    }

    listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listening_socket == 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(DEFAULT_PORT);

    if (bind(listening_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(listening_socket, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for clients on port %d...\n", DEFAULT_PORT);

    addrlen = sizeof(address);

    while (1) {
        // Create an "set" of FD to include all FDs of interest
        // Clear the set and include them one by one
        FD_ZERO(&readfds);

        // Add server listener socket to the set (for demonstration purpose)
        // There is no need to do it for project 2, as you can wait for all
        // connections to complete and receive data later
        FD_SET(listening_socket, &readfds);
        max_sd = listening_socket;

        // Include *connected* sockets for clients (those are not 0)
        for (int i = 0; i < MAXCLIENTS; i++) {
            sd = client_sockets[i];

            if (sd > 0) {
                FD_SET(sd, &readfds);
            }

            // Keep track of the maximum value of the included file descriptors
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // Call select function on the set, we are only interested in the read events
        // so we set all others as 0
        // It will return when any of the socket in the set has the event
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            perror("select");
        }

        // (For demo purpose) If there is event on the listening socket, it means that
        // another client is connecting
        if (FD_ISSET(listening_socket, &readfds)) {
            // Accept the connection and store the socket in the array to receive data
            // from the client
            new_socket = accept(listening_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen);

            if (new_socket < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            printf("Client connected, ip: %s, port: %d\n",
                   inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            for (int i = 0; i < MAXCLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }

        // Check the connection sockets to see if there is incoming data from
        // each client
        for (int i = 0; i < MAXCLIENTS; i++) {
            sd = client_sockets[i];

            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                // Receive data from client if there is data
                valread = read(sd, buffer, BUFFER_SIZE);
                buffer[valread] = '\0';

                if (valread == 0) {
                    close(sd);
                    client_sockets[i] = 0;
                } else {
                    printf("Client %d: %s\n", i, buffer);
                    send(sd, buffer, strlen(buffer), 0);
                }
            }
        }
    }

    return 0;
}
