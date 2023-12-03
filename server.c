#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <netinet/in.h>

#define PORT 9777
#define SERVER_IP "127.0.0.1"
#define MAX_BUFFER_SIZE 1024
#define CAPABILITY_FILE "CAPABILITY LISTS.txt"

#define MAX_CLIENTS 6
#define MAX_FILES 100
#define MAX_PERMISSIONS 9 // r, w, x, o/a ex. rwr------

#define MAX_GROUP_MEMBERS 3
#define MAX_OTHER_MEMBERS 3
#define MAX_NAME_LENGTH 50

typedef struct {
    const char *aos[3];
    const char *cse[3];
} MemberList;

MemberList members = {
    .aos = { "Apple", "Barbie", "Ken" },
    .cse = { "Carla", "Hotpot", "Qoo" }
};

// Structure to represent a capability list entry
typedef struct {
    char client[MAX_CLIENTS];
    char permissions[MAX_PERMISSIONS];
    char group[3];
} CapabilityEntry;

// Structure to represent a file on the server
typedef struct {
    char file_name[MAX_BUFFER_SIZE];
    char owner[MAX_NAME_LENGTH];
    char group[MAX_NAME_LENGTH];
    char permission[MAX_PERMISSIONS];
} ServerFile;

// Capability list for each file
CapabilityEntry capabilityList[MAX_FILES][MAX_CLIENTS];

void create_file ();
void handle_client_command ();
int check_read_permission();
void update_capability_list (int owner_p[], int group_p[], int other_p[]);
void check_file_name();
void changeFilePermissions(ServerFile *file, char *client, char *permissions, char *response);
int hasPermission(ServerFile *file, char *client, char operation, char *response);

void read_file();

void get_member_relation(char group_member[MAX_GROUP_MEMBERS][MAX_NAME_LENGTH], char other_member[MAX_OTHER_MEMBERS][MAX_NAME_LENGTH]);

ServerFile file_list[MAX_FILES];
int file_count = 0;

int server_fd, new_socket;
struct sockaddr_in address;
int addrlen = sizeof(address);
char client_command[MAX_BUFFER_SIZE];
char response[MAX_BUFFER_SIZE];
char usage_rule[MAX_BUFFER_SIZE];

char user_name[MAX_NAME_LENGTH];
char group_name[MAX_NAME_LENGTH];
char operation_name[MAX_NAME_LENGTH];
char file_name[200];
char permission[20];

int main() {
    strcpy(response, "Error.");
    strcpy(usage_rule, "###### Usage examples: ######\n");
    strcat(usage_rule, "1) create homework2.c rwr--- \n");
    strcat(usage_rule, "2) read homework2.c\n");
    strcat(usage_rule, "3) write homework2.c o/a\n");
    strcat(usage_rule, "4) changemode homework2.c rw----\n");
    strcat(usage_rule, "-----------------------------\n");

    // 建立 socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(SERVER_IP);
    address.sin_port = htons(PORT);

    // 將 socket 綁定到指定 IP 跟 port 號
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // 監聽連接請求
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    int count = 1;
    while(1) {
        memset(client_command, 0, sizeof(client_command));
        memset(response, 0, sizeof(response));
        memset(user_name, 0, sizeof(user_name));
        memset(group_name, 0, sizeof(group_name));
        memset(operation_name, 0, sizeof(operation_name));
        memset(file_name, 0, sizeof(file_name));
        memset(permission, 0, sizeof(permission));

        // 接受連接請求並讀取來自 client 的資料
        if (count % 2 == 0) {
            printf("\n\033[1;32m###### Server listening... [%d] ######\033[0m\n", count++); // ANSI 綠色: 32
        } else {
            printf("\n\033[1;36m###### Server listening... [%d] ######\033[0m\n", count++); // ANSI 青色: 36
        }
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        read(new_socket, client_command, sizeof(client_command));
        printf("\033[1;34mGet packet from client.\033[0m\n"); // ANSI 藍色: 34
        
        handle_client_command();
    }

    close(new_socket);
    close(server_fd);

    return 0;
}

void handle_client_command () {
    //  分割字串
    char *command_arr;
    command_arr = strtok(client_command, " ");
    int i = 0;
    while (command_arr != NULL) {
        if (i == 0) {
            strcpy(user_name, command_arr);
        } else if (i == 1) {
            strcpy(group_name, command_arr);
        } else if (i == 2) {
            strcpy(operation_name, command_arr);
        } else if (i == 3) {
            strcpy(file_name, command_arr);
        } else if (i == 4) {
            strcpy(permission, command_arr);
        }
        i++;
        command_arr = strtok(NULL, " "); // 從上次的位置繼續查找
    }
    printf("\nuser_name: %s\n", user_name);
    printf("group_name: %s\n", group_name);
    printf("operation_name: %s\n", operation_name);
    printf("file_name: %s\n", file_name);
    printf("permission: %s\n\n", permission);
    
    if (strcmp(operation_name, "create") == 0) {
        create_file();

        // 回覆 client
        // response[strlen(response)] = '\0';
        send(new_socket, response, strlen(response), 0);
        printf("\033[1;34m>>> %s\033[0m", response); // ANSI 藍色: 34
    } else if (strcmp(operation_name, "read") == 0) {
        read_file();
    } else if (strcmp(operation_name, "write") == 0) {
    } else if (strcmp(operation_name, "changemode") == 0) {
    } else {
        strcpy(response, "Invalid command.\n");
        strcat(response, usage_rule);
        send(new_socket, response, strlen(response), 0);
        printf("\033[1;34m>>> %s\033[0m", response); // ANSI 藍色: 34
    }
}

void create_file () {
    printf("\033[1;34m>>> Create request from %s in group %s.\n\033[0m", user_name, group_name); // ANSI 藍色: 34
    
    check_file_name();
    if (strcmp(response, "[Error] Duplicate file name.\n") == 0) {
        return;
    }
    
    char group_member[MAX_GROUP_MEMBERS][MAX_NAME_LENGTH];
    char other_member[MAX_OTHER_MEMBERS][MAX_NAME_LENGTH];

    get_member_relation(group_member, other_member);
    printf("group_member of %s: %s, %s\n", user_name, group_member[0],group_member[1]);
    printf("other_member of %s: %s, %s, %s \n", user_name, other_member[0],other_member[1], other_member[2]);

    if (file_count >= MAX_FILES) {
        printf("Cannot create more than 100 files.\n");
    }

    // 獲取當前工作目錄
    char cwd[1024];
    // getcwd() 將當前工作目錄的絕對路徑複製到所指的記憶體空間
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("Error getting current working directory");
        exit(EXIT_FAILURE);
    }

    char filepath[1024];
    // snprintf() 用於格式化輸出字串，並將結果寫入到指定的緩衝區，
    // 與 sprintf() 不同的是 snprintf() 會限制輸出字元數，避免緩衝區溢出。
    snprintf(filepath, sizeof(filepath), "%s/%s", cwd, file_name);

    // w+ 開啟可讀寫檔案，若檔案存在則檔案長度清為零，即該檔案內容會消失。若檔案不存在則建立該檔案。
    FILE *file = fopen(filepath, "w");
    if (file == NULL) {
        perror("Error creating file");
        exit(EXIT_FAILURE);
    }
    fclose(file);

    int filePermission = 0;

    /*  https://zhuanlan.zhihu.com/p/530954889
        權限設定的格式是由九個字符組成
        前三個字符表示 擁有者 的權限 / 中間三個字符表示 群組 的權限 / 最後三個字符表示 其他人 的權限
        r 有讀取權限（Read）/ w 有寫入權限（Write / x 有執行權限 (execute) / - or 沒寫默認無權限

        但這個作業好像沒有寫到 x （先默認沒有）
    */

    // int owner_p[3] = {0, 0, 0}, group_p[3] = {0, 0, 0}, others_p[3] = {0, 0, 0};
    int owner_p[2] = {0, 0}, group_p[2] = {0, 0}, others_p[2] = {0, 0};
    for (int i = 0; i < strlen(permission); i++) {
        if (permission[i] == 'r' && owner_p[0] == 0) {
            filePermission |= S_IRUSR;
            owner_p[0] = 1;
            printf("owner_p[0] = 1\n");
        } else if (permission[i] == 'w' && owner_p[1] == 0) {
            filePermission |= S_IWUSR;
            owner_p[1] = 1;
            printf("owner_p[1] = 1\n");
        // } else if (permission[i] == 'x' && owner_p[2] == 0) {
        //     filePermission |= S_IXUSR;
        //     owner_p[2] = 1;
        } else if (permission[i] == 'r' && group_p[0] == 0) {
            filePermission |= S_IRGRP;
            group_p[0] = 1;
            printf("group_p[0] = 1\n");
        } else if (permission[i] == 'w' && group_p[1] == 0) {
            filePermission |= S_IWGRP;
            group_p[1] = 1;
            printf("group_p[1] = 1\n");
        // } else if (permission[i] == 'x' && group_p[2] == 0) {
        //     filePermission |= S_IXGRP;
        //     group_p[2] = 1;
        } else if (permission[i] == 'r') {
            filePermission |= S_IROTH;
            others_p[0] = 1;
            printf("others_p[0] = 1\n");
        } else if (permission[i] == 'w') {
            filePermission |= S_IWOTH;
            others_p[1] = 1;
            printf("others_p[1] = 1\n");
        // } else if (permission[i] == 'x') {
        //     filePermission |= S_IXOTH;
        //     others_p[2] = 1;
        }
    }

    if (chmod(file_name, filePermission) == -1) {
        perror("Error setting file permissions");
        exit(EXIT_FAILURE);
    }
    file_count++;
    update_capability_list(owner_p, group_p, others_p);
    

    char msg[MAX_BUFFER_SIZE] = "";
    strcat(msg, file_name);
    strcat(msg, " created by ");
    strcat(msg, user_name);
    strcat(msg, " in group ");
    strcat(msg, group_name);
    strcat(msg, ".\n");
    strcpy(response, msg);
    // unix 中可使用 ls -l 確認文件權限
}

void get_member_relation(char group_member[MAX_GROUP_MEMBERS][MAX_NAME_LENGTH], char other_member[MAX_OTHER_MEMBERS][MAX_NAME_LENGTH]) {
    const char **group, **others;

    if (strcmp(group_name, "AOS") == 0) {
        group = members.aos;
        others = members.cse;
    } else if (strcmp(group_name, "CSE") == 0) {
        group = members.cse;
        others = members.aos;
    } else {
        printf(">>> error: Invalid group name.\n");
        return;
    }

    int j = 0;
    for (int i = 0; i < MAX_GROUP_MEMBERS; i++) {
        if (strcmp(group[i], user_name) != 0) {
            strcpy(group_member[j++], group[i]);
        }
    }

    for (int i = 0; i < MAX_GROUP_MEMBERS; i++) {
        strcpy(other_member[i], others[i]);
    }
}

void check_file_name () {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(file_list[i].file_name, file_name) == 0) {
            strcpy(response, "[Error] Duplicate file name.\n");
            break;
        }
    }
}

void update_capability_list (int owner_p[], int group_p[], int other_p[]) {
    // format permission
    char formatted_p[10];
    formatted_p[0] = '\0'; // 要初始化 不然會怪怪的QQ
    int file_index = file_count - 1;
    strcat(formatted_p, owner_p[0] == 1 ? "r" : "-");
    strcat(formatted_p, owner_p[1] == 1 ? "w" : "-");
    strcat(formatted_p, group_p[0] == 1 ? "r" : "-");
    strcat(formatted_p, group_p[1] == 1 ? "w" : "-");
    strcat(formatted_p, other_p[0] == 1 ? "r" : "-");
    strcat(formatted_p, other_p[1] == 1 ? "w" : "-");

    strcpy(file_list[file_index].file_name, file_name);
    strcpy(file_list[file_index].owner, user_name);
    strcpy(file_list[file_index].group, group_name);
    strcpy(file_list[file_index].permission, formatted_p);

    printf("\n\033[1;35m------------ CAPABILITY LIST: ------------\033[0m\n"); // ANSI 紫色: 35
    for (int i = 0; i < file_count; i++) {
        printf("%s \t", file_list[i].file_name);
        printf("%s \t", file_list[i].owner);
        printf("%s \t", file_list[i].group);
        printf("%s\n", file_list[i].permission);
    }
    printf("------------------------------------------\n");
}

void read_file() {
    strcpy(response, "Permission denied: You don't have read permission for ");
    strcat(response, file_name);
    strcat(response, ".\n");

    printf("\033[1;34m>>> Read request from %s in group %s\n\033[0m", user_name, group_name); // ANSI 藍色: 34

    if (check_read_permission() == 1) {
        strcpy(response, "Downloading...\n");
        send(new_socket, response, strlen(response), 0);
        printf("\033[1;34m>>> %s\n\033[0m", response); // ANSI 藍色: 34

        // Open the file for reading
        FILE *file = fopen(file_name, "r");
        if (file == NULL) {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }
        
        fseek(file, 0, SEEK_END); // 把檔案指標移到最後面計算大小
        long size = ftell(file);

        // 告訴 client 檔案大小
        send(new_socket, &size, sizeof(size), 0);

        fseek(file, 0, SEEK_SET);  // 記得把指標移回去最前面啊QAQ
        
        char *file_contents = malloc(size + 1);
        if (file_contents == NULL) {
            perror("Error allocating memory");
            exit(EXIT_FAILURE);
        }


        ssize_t bytesRead;
        // server 讀取檔案內容並送給 client
        while ((bytesRead = fread(file_contents, 1, sizeof(file_contents), file)) > 0) {
            send(new_socket, file_contents, bytesRead, 0);  
        }

        printf("\033[1;34m%s is downloaded by %s in group %s.\n\033[0m", file_name, user_name, group_name); // ANSI 藍色: 34

        free(file_contents);
        fclose(file);
    } else {
        printf("\033[1;34m>>> %s\n\033[0m", response); // ANSI 藍色: 34
        send(new_socket, response, strlen(response), 0);
    }
}

int check_read_permission() {
    // permission format 為 rwrwrw: [owner 的 rw 權限][group 的 rw 權限][other 的 rw  權限]
    for (int i = 0; i < file_count; i++) {
        if (strcmp(file_name, file_list[i].file_name) == 0) { // 找到要讀取的檔案
            if (strcmp(group_name, file_list[i].group) == 0) { // 判斷要求讀取的 client 跟 owner group 是否相同
                if (strcmp(user_name, file_list[i].owner) == 0) { // 判斷client 是不是 owner
                    return file_list[i].permission[0] == 'r' ? 1 : 0;
                } else {
                    return file_list[i].permission[2] == 'r' ? 1 : 0;
                }
            } else {
                return file_list[i].permission[4] == 'r' ? 1 : 0;
            }
        }
    }
    strcpy(response, "File is not found.\n");
    printf("\033[1;34m>>> %s\033[0m", response); // ANSI 藍色: 34
    return 0;
}
