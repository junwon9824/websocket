
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/buffer.h> // BUF_MEM 포함
#include <openssl/evp.h> // EVP 관련 함수 사용을 위해 추가


#define SERVER_IP "127.0.0.1" // 서버 IP 주소
#define SERVER_PORT 9000
#define BUFFER_SIZE 1024

void create_websocket_key(char *key) {
    // 무작위 키 생성 (예시로 고정값 사용)
    strcpy(key, "dGhlIHNhbXBsZSBub25jZQ==");
}
/*
void send_message(int sock, const char *message) {
    size_t message_length = strlen(message);
    unsigned char frame[BUFFER_SIZE];

    frame[0] = 0x81; // FIN 비트가 1이고 텍스트 메시지임을 나타냄

    if (message_length <= 125) {
        frame[1] = message_length; // Payload 길이
        memcpy(frame + 2, message, message_length); // 메시지 복사
        send(sock, frame, 2 + message_length, 0); // 프레임 전송
    } else if (message_length <= 65535) {
        frame[1] = 126; // Payload 길이가 126 이상임을 나타냄
        frame[2] = (message_length >> 8) & 0xFF; // 길이 상위 바이트
        frame[3] = message_length & 0xFF; // 길이 하위 바이트
        memcpy(frame + 4, message, message_length); // 메시지 복사
        send(sock, frame, 4 + message_length, 0); // 프레임 전송
    }
}
*/

void send_message(int sock, const char *message) {
    size_t message_length = strlen(message);
    unsigned char frame[BUFFER_SIZE];
    unsigned char mask_key[4];
    int frame_length;
    int i;

    frame[0] = 0x81; // FIN 비트가 1이고 텍스트 메시지임을 나타냄

    // 마스크 키 생성 (임의의 값으로 생성)
    for (i = 0; i < 4; ++i) {
        mask_key[i] = rand() % 256;
    }
//	printf("maskkk %s\n",mask_key);
printf("mask_key: %02x %02x %02x %02x\n", mask_key[0], mask_key[1], mask_key[2], mask_key[3]);

    if (message_length <= 125) {
        frame[1] = 0x80 | message_length; // 마스크 비트 설정 + Payload 길이
        memcpy(frame + 2, mask_key, 4);     // 마스크 키 복사
        frame_length = 6 + message_length;
        for (i = 0; i < message_length; ++i) {
            frame[6 + i] = message[i] ^ mask_key[i % 4]; // XOR 마스크 적용
        }
        send(sock, frame, frame_length, 0); // 프레임 전송
    } else if (message_length <= 65535) {
        frame[1] = 0x80 | 126;             // 마스크 비트 설정 + Payload 길이가 126 이상
        frame[2] = (message_length >> 8) & 0xFF; // 길이 상위 바이트
        frame[3] = message_length & 0xFF;     // 길이 하위 바이트
        memcpy(frame + 4, mask_key, 4);         // 마스크 키 복사
        frame_length = 8 + message_length;
        for (i = 0; i < message_length; ++i) {
            frame[8 + i] = message[i] ^ mask_key[i % 4]; // XOR 마스크 적용
        }
        send(sock, frame, frame_length, 0); // 프레임 전송
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

        // 응답 코드 확인
        if (strncmp(buffer, "HTTP/1.1 101 Switching Protocols", 30) == 0) {
            // Sec-WebSocket-Accept 검증
            char accept_key[BUFFER_SIZE];
            if (sscanf(buffer, "HTTP/1.1 101 Switching Protocols\r\n"
                               "Upgrade: websocket\r\n"
                               "Connection: Upgrade\r\n"
                               "Sec-WebSocket-Accept: %s\r\n", accept_key) != 1) {
                fprintf(stderr, "Sec-WebSocket-Accept header missing or malformed.\n");
                close(sock);
                exit(EXIT_FAILURE);
            }

            // 클라이언트가 생성한 Sec-WebSocket-Key를 기반으로 Sec-WebSocket-Accept 값을 생성
            char expected_accept_key[BUFFER_SIZE];
            char combined_key[BUFFER_SIZE];
            snprintf(combined_key, sizeof(combined_key), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", key);

            // SHA1 해시 생성
            unsigned char hash[SHA_DIGEST_LENGTH];
            SHA1((unsigned char *)combined_key, strlen(combined_key), hash);

            // Base64 인코딩
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

            // Base64 인코딩된 문자열을 accept_key와 비교
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

    // 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // 서버에 연결
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Sec-WebSocket-Key 생성
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

       // 핸드셰이크 요청 전송
    send(sock, request, strlen(request), 0);

    // 서버로부터 응답 수신
    receive_messages(sock, key);

    // 메시지를 계속해서 전송
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

        // 서버로부터 메시지 수신
        if (FD_ISSET(sock, &readfds)) {
            receive_messages(sock, key);
        }

        // 표준 입력으로부터 입력 감지
        if (FD_ISSET(fileno(stdin), &readfds)) {
            // printf("Send a message (or type 'exit' to quit): ");
            fgets(message, sizeof(message), stdin);
            message[strcspn(message, "\n")] = 0; // 개행 문자 제거

            // "exit" 입력 시 종료
            if (strncmp(message, "exit", 4) == 0) {
                break;
            }

            // 메시지 전송
            send_message(sock, message);
        }
    }

    close(sock);
    return 0;
}

