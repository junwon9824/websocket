

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
//#include <base64.h>
#include "base64.h"

#define PORT 9000 
#define BUFFER_SIZE 1024

const char *GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

void create_websocket_accept_key(const char *key, char *accept_key) {
    char combined[256];
    snprintf(combined, sizeof(combined), "%s%s", key, GUID);

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char *)combined, strlen(combined), hash);
    base64_encode(hash, SHA_DIGEST_LENGTH, accept_key);
}


/*
void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    
    // 클라이언트로부터 데이터 수신
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    buffer[bytes_received] = '\0';

    // Sec-WebSocket-Key 추출
    char *key = NULL;
    char *line = strtok(buffer, "\r\n");
    while (line != NULL) {
        if (strstr(line, "Sec-WebSocket-Key:") != NULL) {
            key = strchr(line, ':') + 2; // ':' 이후의 공백을 건너뜁니다.
            break;
        }
        line = strtok(NULL, "\r\n");
    }

    // 핸드셰이크 응답 전송
    if (key) {
        char accept_key[256];
        create_websocket_accept_key(key, accept_key);

        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response),
                 "HTTP/1.1 101 Switching Protocols\r\n"
                 "Upgrade: websocket\r\n"
                 "Connection: Upgrade\r\n"
                 "Sec-WebSocket-Accept: %s\r\n\r\n", accept_key);
        send(client_socket, response, strlen(response), 0);
    }


		// WebSocket 메시지 수신 및 응답 처리
	while (1) {
    	char buffer[BUFFER_SIZE];
   		memset(buffer, 0, BUFFER_SIZE);
    	int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    	if (bytes_received <= 0) {
        	printf("Client disconnected or error occurred\n");
        	break; // 연결 종료
    	}

    	// WebSocket 프레임 처리 (여기서는 간단한 예시로 처리)
    	printf("Received: %s\n", buffer);

    	// 사용자 입력을 받아서 클라이언트에게 응답 메시지 전송
    	char user_input[BUFFER_SIZE];
    	printf("Enter message to send to client: ");
   		fgets(user_input, sizeof(user_input), stdin);
    
    	// 개행 문자를 제거
    	user_input[strcspn(user_input, "\n")] = 0;

    	// 클라이언트에게 응답 메시지 전송
    	send(client_socket, user_input, strlen(user_input), 0);
	}



    close(client_socket);
}
*/

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];

    // 클라이언트로부터 데이터 수신 (핸드셰이크 요청)
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        perror("Error receiving handshake");
        close(client_socket);
        return;
    }
    buffer[bytes_received] = '\0';

    // Sec-WebSocket-Key 추출 (이전 코드와 동일)
    char *key = NULL;
    char *line = strtok(buffer, "\r\n");
    while (line != NULL) {
        if (strstr(line, "Sec-WebSocket-Key:") != NULL) {
            key = strchr(line, ':') + 2;
            break;
        }
        line = strtok(NULL, "\r\n");
    }

    // 핸드셰이크 응답 전송 
    if (key) {
        char accept_key[256];
        create_websocket_accept_key(key, accept_key);

        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response),
                 "HTTP/1.1 101 Switching Protocols\r\n"
                 "Upgrade: websocket\r\n"
                 "Connection: Upgrade\r\n"
                 "Sec-WebSocket-Accept: %s\r\n\r\n", accept_key);
        send(client_socket, response, strlen(response), 0);
        printf("WebSocket handshake successful for client %d\n", client_socket);

        // WebSocket 메시지 수신 및 응답 처리
        while (1) {

//			printf("in whilee\n");

            char buffer[BUFFER_SIZE];
            memset(buffer, 0, BUFFER_SIZE);
            int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
            if (bytes_received <= 0) {
                printf("Client %d disconnected or error occurred\n", client_socket);
                break; // 연결 종료
            }

            // WebSocket 프레임 헤더 파싱 및 역마스크 처리 (이전 코드와 동일)
            unsigned char opcode = buffer[0] & 0x0F;
            unsigned char mask_bit = (buffer[1] >> 7) & 0x01;
            unsigned char payload_len_byte = buffer[1] & 0x7F;
            uint64_t payload_length;
            unsigned char mask_key[4];
            unsigned char *payload_start;
            int header_length = 2;

            if (payload_len_byte <= 125) {
				printf("payload_len_byte %d\n", payload_len_byte );
                payload_length = payload_len_byte;
                if (mask_bit) {
                    memcpy(mask_key, buffer + header_length, 4);
                    header_length += 4;
					printf("buff%s\n", buffer );
	 				printf("mask_key: %02x %02x %02x %02x\n", mask_key[0], mask_key[1], mask_key[2], mask_key[3]);

				 }
                payload_start = (unsigned char *)buffer + header_length;
            	printf("payload_start\n");

			} else if (payload_len_byte == 126) {
                if (bytes_received < 4) break;
                payload_length = (uint16_t)((buffer[2] << 8) | buffer[3]);
                header_length += 2;
                if (mask_bit) {
                    memcpy(mask_key, buffer + header_length, 4);
                    header_length += 4;
                }
                payload_start = (unsigned char *)buffer + header_length;
            } else if (payload_len_byte == 127) {
                if (bytes_received < 10) break;
                payload_length = (uint64_t)(((uint64_t)buffer[2] << 56) |
                                          ((uint64_t)buffer[3] << 48) |
                                          ((uint64_t)buffer[4] << 40) |
                                          ((uint64_t)buffer[5] << 32) |
                                          ((uint64_t)buffer[6] << 24) |
                                          ((uint64_t)buffer[7] << 16) |
                                          ((uint64_t)buffer[8] << 8) |
                                          (uint64_t)buffer[9]);
                header_length += 8;
                if (mask_bit) {
                    memcpy(mask_key, buffer + header_length, 4);
                    header_length += 4;
                }
                payload_start = (unsigned char *)buffer + header_length;
            } else {
                fprintf(stderr, "Invalid payload length from client %d\n", client_socket);
                break;
            }

		//	printf("  buffer  %s\n", buffer );
            printf("mask_key: %02x %02x %02x %02x\n", mask_key[0], mask_key[1], mask_key[2], mask_key[3]);

			if (mask_bit) {
                for (uint64_t i = 0; i < payload_length; ++i) {
                    payload_start[i] = payload_start[i] ^ mask_key[i % 4];
                }
            }

            if (opcode == 0x01) {
                ((char *)payload_start)[payload_length] = '\0';
                printf("Received from client %d: %s\n", client_socket, (char *)payload_start);

                // 사용자 입력을 받아서 클라이언트에게 응답 메시지 전송 (마스크 처리 없이 직접 send 사용)
                char user_input[BUFFER_SIZE];
                printf("Enter message to send to client %d: ", client_socket);
                fgets(user_input, sizeof(user_input), stdin);
                user_input[strcspn(user_input, "\n")] = 0;

                // 클라이언트에게 응답 메시지 전송 (마스크 처리 없이 직접 send 사용)
                send(client_socket, user_input, strlen(user_input), 0);
            } else if (opcode == 0x08) {
                printf("Received close frame from client %d\n", client_socket);
                break;
            } else {
                printf("Received non-text frame (opcode: %x) from client %d\n", client_socket, opcode);
            }
        }
        close(client_socket);
        printf("Connection closed for client %d\n", client_socket);
    } else {
        fprintf(stderr, "Sec-WebSocket-Key not found in request from client %d\n", client_socket);
        char response[] = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        send(client_socket, response, strlen(response), 0);
        close(client_socket);
    }
}









int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Listening failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Listening on port %d\n", PORT);

   // while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
     //       continue;
        }
        printf("Connection accepted\n");

        handle_client(client_socket);
   // }

    close(server_socket);
    return 0;
}

