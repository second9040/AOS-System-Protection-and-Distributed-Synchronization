// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// 這裡要跟 server 一樣否則連不到
#define PORT 9777
#define SERVER_IP "127.0.0.1"
#define MAX_BUFFER_SIZE 1024

int get_user_number(char *user);

int main(int argc, char *argv[]) {
    int client_socket;
    struct sockaddr_in server_address;
    char client_command[MAX_BUFFER_SIZE] = " ";
    char enter_connand[MAX_BUFFER_SIZE] = " ";
    char *userlist[] = {
        "Apple:AOS", "Barbie:AOS", "Ken:AOS", 
        "Carla:CSE", "Hotpot:CSE", "Qoo:CSE"
    };
    char user[20];
    char *group = "AOS";
    int user_number;

    while (1) {
        printf("\n\033[1;32m###### Select a client number: ######\033[0m\n"); // ANSI 綠色: 32
        printf("-------------------------------------------------\n");
        printf("1. Apple:AOS 2. Barbie:AOS 3. Ken:AOS\n4. Carla:CSE 5. Hotpot:CSE 6. Qoo:CSE\n");
        printf("-------------------------------------------------\n");
        printf("\033[1;35mEnter 1 ~ 6: \033[0m"); // ANSI 紫色: 35
        scanf("%s", user);

        user_number = get_user_number(user);

        if (user_number >= 0 && user_number < 6) {
            printf("\033[1;34mClient %s. %s selected.\033[0m\n", user, userlist[user_number]); // ANSI 藍色: 34
            break;
        } else {
            printf(">>> Invalid input. Please enter a number between 1 and 6.\n");
            printf(">>> Note: Do NOT enter space.\n");
        }
    }

    int count = 1;
    while (1) {
        char buffer[MAX_BUFFER_SIZE] = "";

        memset(buffer, 0, sizeof(buffer));
        memset(client_command, 0, sizeof(client_command));

        if (count % 2 == 0) {
            printf("\n\033[1;32m###### Enter command [%d]: ######\033[0m\n", count++); // ANSI 綠色: 32
        } else {
            printf("\n\033[1;36m###### Enter command [%d]: ######\033[0m\n", count++); // ANSI 青色: 36
        }
        printf("1) create homework2.c rwr--- \n");
        printf("2) read homework2.c\n");
        printf("3) write homework2.c o/a\n");
        printf("4) changemode homework2.c rw----\n");
        printf("--------------------------------\n");
        printf("\033[1;35mCommand: \033[0m"); // ANSI 紫色: 35
        scanf(" %[^\n]", enter_connand); // 讀取包含空格的輸入, % 前密的空格不可省略

        // 建立 socket
        if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
            perror(">>> Socket creation failed");
            exit(EXIT_FAILURE);
        }

        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(PORT);

        // 把 IPv4 地址轉成二進制格式
        if (inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) <= 0) {
            perror(">>> Invalid address/ Address not supported");
            exit(EXIT_FAILURE);
        }

        // connect server
        if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
            perror(">>> Connection failed");
            exit(EXIT_FAILURE);
        }

        char user_info[20];
        strcpy(user_info, userlist[user_number]); // strtok 要字串陣列 = =
        char *temp = strtok(user_info, ":");
        strcat(client_command, temp);
        temp = strtok(NULL, ":");
        strcat(client_command, " ");
        strcat(client_command, temp);
        strcat(client_command, " ");
        strcat(client_command, enter_connand);
        send(client_socket, client_command, strlen(client_command), 0);

        if (strncmp(enter_connand, "read", 4) == 0) {
            memset(buffer, 0, sizeof(buffer));
            read(client_socket, buffer, sizeof(buffer));
            printf("\033[1;34m>>> %s\033[0m\n", buffer); // ANSI 藍色: 34
            if (strcmp(buffer, "Downloading...\n") == 0) { // 允許讀取 (下載) 檔案
                char *file_name;
                // strstr(str1, str2) 有找到會回傳指向 str1 的指標並且指向第一次符合的位置，沒找到會回傳 NULL
                file_name = strstr(enter_connand, " ");
                file_name = file_name + 1;

                memset(buffer, 0, sizeof(buffer));

                FILE *local_file = fopen(file_name, "wb");
                if (local_file == NULL) {
                    perror("Error creating local file");
                    exit(EXIT_FAILURE);
                }

                off_t file_size;
                ssize_t bytesRead;

                bytesRead = read(client_socket, &file_size, sizeof(file_size));
                if (bytesRead < 0) {
                    perror("Error reading file size from socket");
                    fclose(local_file);
                    exit(EXIT_FAILURE);
                } else {
                    // 接收 server 傳過來的檔案內容並寫入本地檔案中
                    while (file_size > 0 && (bytesRead = read(client_socket, buffer, sizeof(buffer))) > 0) {
                        fwrite(buffer, 1, bytesRead, local_file);
                        file_size -= bytesRead;
                    }

                    printf("\033[1;34m>>> %s is downloaded.\033[0m\n", file_name); // ANSI 藍色: 34
                }
                fclose(local_file);
            }
        } else {
            read(client_socket, buffer, sizeof(buffer));
            printf("\033[1;34m>>> %s\033[0m\n", buffer); // ANSI 藍色: 34
        }
    }

    close(client_socket);
    return 0;
}

int get_user_number(char *user) {
    return atoi(user) - 1;
}
