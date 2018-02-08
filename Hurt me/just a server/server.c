#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <memory.h>
#include <signal.h>
#include <stdbool.h>
#include <ctype.h>

int sock;

const int BUFFER_SIZE = 8192;

struct simple_connection {
    size_t size;
    char *buffer;

    size_t res_size;
    char *response;
};

struct simple_connection connection[1024] = {0};

char *bad_rep = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\nContent-Length: 22%d\r\n\r\n<h1>404 Not Found</h1>";
char *http_response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n%s";

char *ascaddr(struct sockaddr_in *net) {
    uint32_t addr = net->sin_addr.s_addr;
    uint16_t port = net->sin_port;
    uint8_t *ip = (uint8_t *) (&addr);
    char container[10] = {0};
    sprintf(container, "%u.%u.%u.%u:%u", *ip, *(ip + 1), *(ip + 2), *(ip + 3), port);
    char *address = (char *) malloc(strlen(container));
    memcpy(address, container, strlen(container));
    return address;
}

void signal_handler(int sig_num) {
    if (sig_num == SIGINT) {
        close(sock);
        exit(7);
    } else {

    }
}

int valid(const char *buffer, int readret);

int http_parser(char *buffer, size_t buffer_size, char **response, size_t *res_size);

int main(int argc, char **argv) {

    signal(SIGINT, signal_handler);
    signal(SIGPIPE, signal_handler);

    char buffer[BUFFER_SIZE];
    int port = 7777;
    if (argc == 2) { port = (int) strtol(argv[1], NULL, 10); }

    sock = socket(PF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t) port);
    addr.sin_addr.s_addr = INADDR_ANY;
    int err = bind(sock, (struct sockaddr *) &addr, sizeof(addr));
    if (err == -1) {
        perror("bind");
        exit(1);
    }

    listen(sock, 7);

    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    /* select utility setup */
    fd_set master;
    fd_set read_fds;
    fd_set write_fds;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    int fd_max;

    FD_SET(sock, &master);
    fd_max = sock;

    printf("Liso serving on %d\n", port);
    while (true) {
        read_fds = master;
        write_fds = master;
        if (select(fd_max + 1, &read_fds, &write_fds, NULL, NULL) == -1) {
            perror("select");
            exit(8);
        }

        //  for fd in all current fds
        for (int i = 0; i <= fd_max; ++i) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == sock) { // sock is ready to accept
                    int client = accept(sock, (struct sockaddr *) &client_addr, &len);
                    if (client == -1) { perror("accept"); }
                    else {
                        FD_SET(client, &master);
                        if (client > fd_max) fd_max = client;
                        printf("%s\n", ascaddr(&client_addr));
                    }
                } else {         // client is ready to read
                    ssize_t readret = recv(i, buffer, BUFFER_SIZE, 0);
                    if (readret == 0) {
                        printf("Sock %d hang up\n", i);
                        close(i);
                        FD_CLR(i, &master);
                    } else if (readret == -1) {
                        perror("recv");
                        close(i);
                        FD_CLR(i, &master);
                    } else {

                        connection[i].buffer = (char *)malloc(readret);
                        memcpy(connection[i].buffer, buffer, readret);
                        connection[i].size = readret;

                        // printf("Request:\n%s\n", connection[i].buffer);

                        if (valid(buffer, (int) readret)
                            && http_parser(connection[i].buffer, connection[i].size, &connection[i].response,
                                           &connection[i].res_size)) {
                            connection[i].res_size = strlen(connection[i].response);
                        } else {
                            connection[i].res_size = strlen(bad_rep);
                            connection[i].response = (char *) malloc(strlen(bad_rep));
                            memcpy(connection[i].response, bad_rep, strlen(bad_rep));
                        }
                    }
                }
            }
            if (FD_ISSET(i, &write_fds) && connection[i].response) {
                if (send(i, connection[i].response, connection[i].res_size, 0) <= 0) {
                    perror("Send");
                }
                free(connection[i].buffer);
                free(connection[i].response);
                connection[i].buffer = NULL;
                connection[i].response = NULL;
                close(i);
                FD_CLR(i, &master);
            }
        } // END looping through file descriptors
    } // END infinate loop
}

int valid(const char *buffer, int readret) {
    for (int j = 0; j < readret; ++j) {
        if (buffer[j] == '\r' && buffer[j + 1] != '\n') {
            return 0;
        }
    }

    for (int j = 1; j <= readret; ++j) {
        if (buffer[j] == '\n' && buffer[j - 1] != '\r') {
            return 0;
        }
    }

    return 1;
}

int http_parser(char *buffer, size_t buffer_size, char **response, size_t *res_size) {
    // a parser to generate request header, a simple version is...

    if (buffer_size < 10) return 0;

    int space = 3;
    while (!isspace(buffer[++space])) {
        if (space > 14) return 0;
    }
    int offset;
    if (buffer[4] == '/' ) offset = 5; else offset = 4;
    // just get file name
    char filename[256] = {0};
    memcpy(filename, buffer + offset, space - offset);

    printf("file: %s", filename);
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        // 404 Not found
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *file = malloc(fsize + 1);
    fread(file, fsize, 1, f);
    fclose(f);

    *response = (char *) malloc(4094);
    bzero(*response, 4096);
    sprintf(*response, http_response, fsize, file);

    return 1;
    // *res_size = strlen(*response);
}
