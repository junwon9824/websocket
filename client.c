
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/buffer.h> // BUF_MEM ����
#include <openssl/evp.h> // EVP ���� �Լ� ����� ���� �߰�


#define SERVER_IP "127.0.0.1" // ���� IP �ּ�
#define SERVER_PORT 9000
#define BUFFER_SIZE 1024

void create_websocket_key(char *key) {
    // ������ Ű ���� (���÷� ������ ���)
    strcpy(key, "dGhlIHNhbXBsZSBub25jZQ==");
}
/*
void send_message(int sock, const char *message) {
    size_t message_length = strlen(message);
    unsigned char frame[BUFFER_SIZE];

    frame[0] = 0x81; // FIN ��Ʈ�� 1�̰� �ؽ�Ʈ �޽������� ��Ÿ��

    if (message_length <= 125) {
        frame[1] = message_length; // Payload ����
        memcpy(frame + 2, message, message_length); // �޽��� ����
        send(sock, frame, 2 + message_length, 0); // ������ ����
    } else if (message_length <= 65535) {
        frame[1] = 126; // Payload ���̰� 126 �̻����� ��Ÿ��
        frame[2] = (message_length >> 8) & 0xFF; // ���� ���� ����Ʈ
        frame[3] = message_length & 0xFF; // ���� ���� ����Ʈ
        memcpy(frame + 4, message, message_length); // �޽��� ����
        send(sock, frame, 4 + message_length, 0); // ������ ����
    }
}
*/

void send_message(int sock, const char *message) {
    size_t message_length = strlen(message);
    unsigned char frame[BUFFER_SIZE];
    unsigned char mask_key[4];
    int frame_length;
    int i;

    frame[0] = 0x81; // FIN ��Ʈ�� 1�̰� �ؽ�Ʈ �޽������� ��Ÿ��

    // ����ũ Ű ���� (������ ������ ����)
    for (i = 0; i < 4; ++i) {
        mask_key[i] = rand() % 256;
    }
//	printf("maskkk %s\n",mask_key);
printf("mask_key: %02x %02x %02x %02x\n", mask_key[0], mask_key[1], mask_key[2], mask_key[3]);

    if (message_length <= 125) {
        frame[1] = 0x80 | message_length; // ����ũ ��Ʈ ���� + Payload ����
        memcpy(frame + 2, mask_key, 4);     // ����ũ Ű ����
        frame_length = 6 + message_length;
        for (i = 0; i < message_length; ++i) {
            frame[6 + i] = message[i] ^ mask_key[i % 4]; // XOR ����ũ ����
        }
        send(sock, frame, frame_length, 0); // ������ ����
    } else if (message_length <= 65535) {
        frame[1] = 0x80 | 126;             // ����ũ ��Ʈ ���� + Payload ���̰� 126 �̻�
        frame[2] = (message_length >> 8) & 0xFF; // ���� ���� ����Ʈ
        frame[3] = message_length & 0xFF;     // ���� ���� ����Ʈ
        memcpy(frame + 4, mask_key, 4);         // ����ũ Ű ����
        frame_length = 8 + message_length;
        for (i = 0; i < message_length; ++i) {
            frame[8 + i] = message[i] ^ mask_key[i % 4]; // XOR ����ũ ����
        }
        send(sock, frame, frame_length, 0); // ������ ����
    }
}

 



void receive_messages(int sock, const char *key) {
    char buffer[BUFFER_SIZE];
    struct timeval timeout;




        int bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("Server disconnected or error occurred\n");
            close(sock);
            exit(EXIT_FAILURE);
        }
        buffer[bytes_received] = '\0';

        // ���� �ڵ� Ȯ��
        if (strncmp(buffer, "HTTP/1.1 101 Switching Protocols", 30) == 0) {
            // Sec-WebSocket-Accept ����
            char accept_key[BUFFER_SIZE];
            if (sscanf(buffer, "HTTP/1.1 101 Switching Protocols\r\n"
                               "Upgrade: websocket\r\n"
                               "Connection: Upgrade\r\n"
                               "Sec-WebSocket-Accept: %s\r\n", accept_key) != 1) {
                fprintf(stderr, "Sec-WebSocket-Accept header missing or malformed.\n");
                close(sock);
                exit(EXIT_FAILURE);
            }

            // Ŭ���̾�Ʈ�� ������ Sec-WebSocket-Key�� ������� Sec-WebSocket-Accept ���� ����
            char expected_accept_key[BUFFER_SIZE];
            char combined_key[BUFFER_SIZE];
            snprintf(combined_key, sizeof(combined_key), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", key);

            // SHA1 �ؽ� ����
            unsigned char hash[SHA_DIGEST_LENGTH];
            SHA1((unsigned char *)combined_key, strlen(combined_key), hash);

            // Base64 ���ڵ�
            BIO *bio, *b64;
            BUF_MEM *bufferPtr;

            b64 = BIO_new(BIO_f_base64());
            bio = BIO_new(BIO_s_mem());
            bio = BIO_push(b64, bio);
            BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
            BIO_write(bio, hash, SHA_DIGEST_LENGTH);
            BIO_flush(bio);
            BIO_get_mem_ptr(bio, &bufferPtr);
            BIO_set_close(bio, BIO_NOCLOSE);
            BIO_free_all(bio);

            // Base64 ���ڵ��� ���ڿ��� accept_key�� ��
            char *base64_encoded = malloc(bufferPtr->length + 1);
            memcpy(base64_encoded, bufferPtr->data, bufferPtr->length);
            base64_encoded[bufferPtr->length] = '\0';

            if (strcmp(accept_key, base64_encoded) != 0) {
                fprintf(stderr, "Sec-WebSocket-Accept value mismatch.\n");
                free(base64_encoded);
                close(sock);
                exit(EXIT_FAILURE);
            }

            free(base64_encoded);
            printf("Handshake successful!\n");
        } else {
            printf("Received from server: %s\n", buffer);
        }
    // }
}







int main() {
    int sock;
    struct sockaddr_in server_addr;
    char key[256];

    // ���� ����
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // ������ ����
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Sec-WebSocket-Key ����
    create_websocket_key(key);
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request),
             "GET / HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Key: %s\r\n"
             "Sec-WebSocket-Version: 13\r\n\r\n",
             SERVER_IP, SERVER_PORT, key);

       // �ڵ����ũ ��û ����
    send(sock, request, strlen(request), 0);

    // �����κ��� ���� ����
    receive_messages(sock, key);

    // �޽����� ����ؼ� ����
    char message[BUFFER_SIZE];
    fd_set readfds;
    int maxfd = sock > fileno(stdin) ? sock : fileno(stdin);
    struct timeval timeout;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        FD_SET(fileno(stdin), &readfds);

        timeout.tv_sec = 60;
        timeout.tv_usec = 0;

        int activity = select(maxfd + 1, &readfds, NULL, NULL, &timeout);

        if (activity < 0) {
            perror("Select error");
            break;
        } else if (activity == 0) {
            printf("Timeout occurred\n");
           	
			close(sock);

        }

        // �����κ��� �޽��� ����
        if (FD_ISSET(sock, &readfds)) {
            receive_messages(sock, key);
        }

        // ǥ�� �Է����κ��� �Է� ����
        if (FD_ISSET(fileno(stdin), &readfds)) {
            // printf("Send a message (or type 'exit' to quit): ");
            fgets(message, sizeof(message), stdin);
            message[strcspn(message, "\n")] = 0; // ���� ���� ����

            // "exit" �Է� �� ����
            if (strncmp(message, "exit", 4) == 0) {
                break;
            }

            // �޽��� ����
            send_message(sock, message);
        }
    }

    close(sock);
    return 0;
}

