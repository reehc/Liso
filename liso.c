#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <wait.h>
#include <sys/stat.h>
#include "clog.h"
#include "parse.h"

#define CHEER_PORT "7777"
#define BUF_SIZE 8192

int close_socket(int sock) {
    if (close(sock)) {
        fprintf(stderr, "Failed closing socket.\n");
        return 1;
    }
    return 0;
}

char *header = "HTTP/1.1 200 OK\r\n"
                 "Connection: close\r\n"
                 "Content-Type: text/html\r\n\r\n";

char *css_header = "HTTP/1.1 200 OK\r\n"
        "Connection: close\r\n"
        "Content-Type: text/css\r\n\r\n";

char *png_header = "HTTP/1.1 200 OK\r\n"
        "Connection: close\r\n"
        "Content-Type: image/png\r\n\r\n";


char *error = "HTTP/1.1 400 Bad Request\r\n";

char www[256] = "./www/";

int main(int argc, char *argv[]) {
    int sock, client_sock;  // server and client sock descriptor
    ssize_t readret;
    socklen_t cli_size;
    struct sockaddr_in addr, cli_addr;
    // or
    struct sockaddr_storage remoteaddr;
    char buf[BUF_SIZE];     // buffer for client data

    fd_set master;      // master file descriptor list
    fd_set read_fds;    // temp file descriptor list for select
    int fdmax;          // maximum file descriptor number

    char remoteIP[INET6_ADDRSTRLEN];

    int yes = 1;
    int rv;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    clog_initialize("log");

    fprintf(stdout, "----- Echo Server -----\n");

    // get us a socket and bind it
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, CHEER_PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "select-based server: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = ai; p != NULL; p = p->ai_next) {
        sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sock < 0) {
            continue;
        }

        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(sock, p->ai_addr, p->ai_addrlen) < 0) {
            close(sock);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "select-based server: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai);

    if (listen(sock, 10) == -1) {
        perror("listen");
        exit(2);
    }

    FD_SET(sock, &master);
    fdmax = sock;

    for (;;) {
        read_fds = master;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(8);
        }

        for (int i = 0; i <= fdmax; ++i) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == sock) {
                    // handle new connections
                    cli_size = sizeof(cli_addr);
                    client_sock = accept(sock, (struct sockaddr *) &cli_addr,
                                         &cli_size);
                    if (client_sock == -1) {
                        perror("accept");
                    } else {
                        FD_SET(client_sock, &master);
                        if (client_sock > fdmax) {
                            fdmax = client_sock;
                        }
                        unsigned char *ip = (unsigned char *) &cli_addr.sin_addr.s_addr;
                        printf("%u.%u.%u.%u:%d on socket %d",
                               *ip, *(ip + 1), *(ip + 2), *(ip + 3), cli_addr.sin_port, client_sock);
                    }
                } else {
                    // handle data from a client

                    // Only add this code, "connection reset by peer" won't show.
                    // Why?
                    // And a browser cannot load page properly.
                    // Why?
                    // Checkpoint2 !
                    memset(buf, 0, BUF_SIZE);
                    if ((readret = recv(i, buf, BUF_SIZE, 0)) <= 0) {
                        if (readret == 0) {
                            printf("select-based server: sock %d hang up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master);
                    } else {
                        // Request *request = parse(buf, readret, client_sock);

                        int valid = 1;
                        for (int j = 0; j < readret; ++j) {
                            if (buf[j] == '\r' && buf[j+1] != '\n') {
                                valid = 0;
                                break;
                            }
                        }

                        for (int j = 1; j <= readret; ++j) {
                            if (buf[j] == '\n' && buf[j-1] != '\r') {
                                valid = 0;
                                break;
                            }
                        }

                        for (int k = 0; k < readret ; ++k) {
                            if (buf[k] == '.' && buf[k+1] == '.') {
                                valid = 0;
                                break;
                            }
                        }

                        if (valid && buf[0] - 'G' == 0 && buf[1] - 'E' == 0 && buf[2] - 'T' == 0) {
                            // Content type
                            char *h = header;
                            int k = 4;
                            while (buf[k] != ' ') k++;
                            if (buf[k-1]=='s' && buf[k-2]=='s' && buf[k-3]=='c') h = css_header;
                            else if (buf[k-1]=='g' && buf[k-2]=='n' && buf[k-3]=='p') h = png_header;

                            // Data
                            memset(www+5, 0, sizeof(www)-6);
                            int j;
                            for (j = 4; buf[j] != ' '; ++j) {
                                www[j+1] = buf[j];
                            }
                            if (www[j] == '/' && www[j+1] == 0) {
                                char str[11] = "index.html";
                                strcpy(&www[j+1], str);
                            }

                            FILE *fp = fopen(www, "r");
                            struct stat st;
                            char *file;
                            if (fp != NULL) {
                                stat(www, &st);
                                file = malloc(sizeof(char) * st.st_size);
                                size_t newLen = fread(file, st.st_size, 1, fp);
                                if ( ferror( fp ) != 0 )
                                    fputs("Error reading file", stderr);
                                fclose(fp);
                                if (send(i, h, strlen(h), 0) == -1 || send(i, file, st.st_size, 0) == -1)
                                    perror("send");
                            } else {
                                if (send(i, error, strlen(error), 0) == -1)
                                    perror("send");
                            }
                        } else if (valid && buf[0] - 'P' == 0 && buf[1] - 'O' == 0 && buf[2] - 'S' == 0 && buf[3] - 'T' == 0) {
                            if (send(i, buf, readret, 0) == -1)
                                perror("send");
                        } else {
                            if (send(i, error, strlen(error), 0) == -1)
                                perror("send");
                        }
                        close(i);
                        FD_CLR(i, &master);
                    }
                }
            }
        }
    }

    close_socket(sock);

    return EXIT_SUCCESS;
}
