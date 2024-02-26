#include "..\..\Privy\header.cpp"
#include <string>
#include <algorithm>

#define CONNECTED "CONNECTED" // we can hash this if we really need security
#define DECLINED "DECLINED"

char rcvbuff[BUFFERSIZE];
char sndbuff[BUFFERSIZE];
char name[257];

int client_socket;

void clear_buff(char buffer[], int size){
    if (strlen(buffer) > 0){
        memset(buffer, '\0', size);
    }
}

void clear_stdin(){
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void input_verification(char destination[], int max, std::string flag){
    clear_buff(destination, max + 2);
    std::cout << flag;

    fgets(destination, max + 2, stdin);
    if (strlen(destination) == max + 1 && destination[max] != '\n'){
        std::cout << "max char is " << max << '\n';
        clear_stdin();
        input_verification(destination, max, flag);
    }
    else destination[strcspn(destination, "\n")] = '\0'; // remove new line
}

void view_rooms(){
    char choice[7];
    while(true){
        clear_buff(sndbuff, BUFFERSIZE);
        strcpy(sndbuff, "vrm");
        send(client_socket, sndbuff, strlen(sndbuff), 0);
        clear_buff(rcvbuff, BUFFERSIZE);
        recv(client_socket, rcvbuff, BUFFERSIZE, 0);

        std::cout << "\033[2J\033[1;1H";
        std::cout << "===================== ROOMS ====================\n";
        std::cout << "== (input room ID to see details | b to back) ==\n" << '\n';

        if (strncmp(rcvbuff, DECLINED, strlen(DECLINED)) == 0){
            std::cout << "something went wrong..." << '\n';
        }
        else if (strncmp(rcvbuff, CONNECTED, strlen(CONNECTED)) == 0){
            int len = strlen(rcvbuff);
            std::replace(rcvbuff, rcvbuff + strlen(rcvbuff), '|', '\0');
            int i = 1;
            for (int at = strlen(CONNECTED) + 1; at < len; at += 10){
                std::cout << i++ << "| ID: " <<  rcvbuff + at << " | ";
                std::cout << (strcmp(rcvbuff + at + 6, "PRV") == 0 ? "PRIVATE" :"PUBLIC") << '\n';
            }
        }
        else std::cout << "server sending gibberish..." << '\n';

        input_verification(choice, 5, "\nchoice: ");
        if (strlen(choice) == 1 && choice[0] == 'b') break;

        clear_buff(sndbuff, BUFFERSIZE);
        strcpy(sndbuff, "vrd|");
        strcat(sndbuff, choice);
        send(client_socket, sndbuff, strlen(sndbuff), 0);
        
        clear_buff(rcvbuff, BUFFERSIZE);
        recv(client_socket, rcvbuff, BUFFERSIZE, 0);

        std::replace(rcvbuff, rcvbuff + strlen(rcvbuff), '|', '\0');

        if (strcmp(rcvbuff, DECLINED) == 0){
            std::cout << "can't reach that ID..." << '\n';
            Sleep(1500);
        }
        else if (strcmp(rcvbuff, CONNECTED) == 0){
            char *room_type = (rcvbuff + strlen(CONNECTED) + 1);
            char *num_clients  = room_type + 4;

            std::cout << "\033[2J\033[1;1H";
            std::cout << "====== ROOM " << choice << " DETAILS ======\n";
            std::cout << "===== (press any key to go back) =====\n" << '\n';
            std::cout << "ROOM STATUS: " << (strcmp(room_type, "PRV") == 0 ? "PRIVATE" : "PUBLIC") << '\n';
            std::cout << "NUMBER OF CLIENTS: " <<  num_clients << '\n';
            std::cout << "DESCRIPTION: " <<  num_clients + strlen(num_clients) + 1 << '\n';
            getchar();
        }
        else{
            std::cout << "server sending gibberish" << '\n';
            std::cout << rcvbuff << '\n';
            Sleep(2000);
        }
    }
}

void send_message(char ID[]){
    char message[257];

    input_verification(message, 255, "input message: ");
    if (strlen(message) == 1 && message[0] == 'b') return;

    clear_buff(sndbuff, BUFFERSIZE);
    strcpy(sndbuff, "snd|");
    strcat(sndbuff, ID);
    strcat(sndbuff, "|");
    strcat(sndbuff, message);

    send(client_socket, sndbuff, strlen(sndbuff), 0);
    // check if received
}

void refresh(char ID[]){
    std::cout << "\033[2J\033[1;1H";

    char refresh_message[10];

    strcpy(refresh_message, "rfs|");
    strcat(refresh_message, ID);

    send(client_socket, refresh_message, 10, 0);
    clear_buff(rcvbuff, BUFFERSIZE);
    recv(client_socket, rcvbuff, BUFFERSIZE, 0);

    // handle whether it really is the proper message format
    if (strncmp(rcvbuff, CONNECTED, strlen(CONNECTED)) == 0){
        std::cout << "======= ROOM " << ID << " =======" << '\n' << '\n';
        std::cout << rcvbuff + strlen(CONNECTED) + 2 << '\n';
    }
    else if (strncmp(rcvbuff, DECLINED, strlen(DECLINED)) == 0){
        std::cout << "something went wrong, try again..." << '\n';
    }
    else{
        std::cout << "server is talking gibberish" << '\n';
        std::cout << "Server: " << rcvbuff << '\n';
    }
}

void chat(char ID[]){
    char choice[3];
    char message[257];
    
    refresh(ID);

    while (true){
        std::cout << "======= OPTIONS ======= " << '\n';
        std::cout << "[1]       send      " << '\n';
        std::cout << "[2]    refresh      " << '\n';
        std::cout << "[3]       back      " << '\n';

        input_verification(choice, 1, "your choice: ");
        
        switch(choice[0]){
            case '1': send_message(ID);
                    break;
            case '3': return;
        }
        refresh(ID);
    }
}

void join_room(){
    char room_id[7];

    std::cout << "\033[2J\033[1;1H";
    std::cout << "===== JOINING A ROOM (input b to back)" << " =====" << '\n';

    input_verification(room_id, 5, "input room id: ");
    if (strlen(room_id) == 1 && room_id[0] == 'b') return;

    // jrm|id
    clear_buff(sndbuff, BUFFERSIZE);
    strcpy(sndbuff, "jrm|");
    strcat(sndbuff, room_id);

    send(client_socket, sndbuff, strlen(sndbuff), 0);

    // recieve a connection approval or not
    clear_buff(rcvbuff, BUFFERSIZE);
    recv(client_socket, rcvbuff, BUFFERSIZE, 0);

    // requires a password
    if (strncmp(rcvbuff, DECLINED, strlen(rcvbuff)) == 0){
        std::cout << "there is not  such room..." << '\n';
        Sleep(3000);
        join_room();
        return;
    }
    else if (strncmp(rcvbuff, "PUB", strlen(rcvbuff)) == 0){
        chat(room_id);
    }
    else if (strncmp(rcvbuff, "PRV", strlen(rcvbuff)) == 0){
        char password[22];

        while (true){
            input_verification(password, 20, "please input room password: ");
            if (strlen(password) == 1 && password[0] == 'b'){
                join_room();
                return;
            }

            clear_buff(sndbuff, BUFFERSIZE);
            strcpy(sndbuff, "pwd|");
            strcat(sndbuff, room_id);
            strcat(sndbuff, "|");
            strcat(sndbuff, password);
            
            send(client_socket, sndbuff, strlen(sndbuff), 0);
            recv(client_socket, rcvbuff, BUFFERSIZE, 0);

            std::cout << rcvbuff << '\n';
            if(strncmp(rcvbuff, CONNECTED, strlen(CONNECTED)) == 0){
                chat(room_id);
                return;
            }
            else if (strncmp(rcvbuff, DECLINED, strlen(DECLINED)) == 0){
                std::cout << "incorrect password: " << '\n';
            }
            else {
                std::cout << "server is sending giberish" << '\n';
                std::cout << "server: " << rcvbuff << '\n';
            }
        }
    }
    else{
        std::cout << "server talking gibberish" << '\n';
        std::cout << "server: " << rcvbuff << '\n';
        Sleep(1000);
    }
}

void create_room(){
    boolean is_private;
    char description[257];
    char password[22];

    std::cout << "\033[2J\033[1;1H";
    std::cout << "===== CREATING A ROOM (input b to back)" << " =====" << '\n';

    input_verification(description, 255, "input description: ");
    if (strlen(description) == 1 && description[0] == 'b') return;
    
    char choice[3];
    while (true){
        std::cout << "Add a password? [y/n]: ";

        fgets(choice, sizeof(choice), stdin);
        if (strlen(choice) == 2 && choice[1] != '\n'){
            std::cout << "please input a single character" << '\n';
            clear_stdin();
            continue;
        }

        if (choice[0] == 'b'){
            create_room();
            return;
        }
        
        if (choice[0] != 'y' && choice[0] != 'n') continue;
        is_private = choice[0] == 'y';
        break;
    }
    
    clear_buff(sndbuff, BUFFERSIZE);     // clear buffer to prep for sending
    strcpy(sndbuff, "crm|");             // place the command.

    if (is_private){
        input_verification(password, 20, "input password: ");
        if (strlen(password) == 1 && password[0] == 'b'){
            create_room();
            return;
        }
        strcat(sndbuff, "1|");
    }
    else strcat(sndbuff, "0|");

        strcat(sndbuff, description);

    if (strlen(password) > 0){
        strcat(sndbuff, "|");
        strcat(sndbuff, password);
    }
    
    send(client_socket, sndbuff, strlen(sndbuff), 0);
    // verification of connection
    clear_buff(rcvbuff, BUFFERSIZE);
    recv(client_socket, rcvbuff, BUFFERSIZE, 0);

    char ID[6];
    memset(ID, '\0', 6);
    strncpy(ID, rcvbuff, 6);

    chat(ID);
}


void exit_privy(){
    WSACleanup();
    std::cout << "thank you for using PRIVY!" << std::endl;
    exit(0);
}

void communicate(){
    char choice[3];

    while (true){
        std::cout << "\033[2J\033[1;1H";
        std::cout << "===== HI " << name << " =====\n" << '\n';
        std::cout << "[1] join a room" << '\n';
        std::cout << "[2] view rooms" << '\n';
        std::cout << "[3] create a room" << '\n';
        std::cout << "[4] exit" << '\n';
        std::cout << "choice: ";

        fgets(choice, 3, stdin);
        if (strlen(choice) == 2 && choice[1] != '\n'){
            std::cout << "please input a single character" << '\n';
            clear_stdin();
            continue;
        }

        switch(choice[0]){
            case '1' : join_room();
                       break;
            case '2' : view_rooms();
                       break;
            case '3' : create_room();
                       break;
            case '4' : exit_privy();
            default: std::cout << "please input a valid choice" << '\n';
        }
    }
}

// Returns a socket for the server using the given environment
int connect_client(sockaddr_in *server_env){
    WSADATA wsd;
    test(WSAStartup(MAKEWORD(2, 2), &wsd), "Initializing windows sockets");

    int client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    test(client_socket, "Creating client socket");

    test(connect(client_socket, (sockaddr*) server_env, sizeof(*server_env)), "Connecting to server");

    return client_socket;
}

int main(int args, char **arsv){
    // Initialize environment for socket
    sockaddr_in client_env;
    client_env.sin_family = AF_INET;
    client_env.sin_addr.s_addr = inet_addr(ADDR);
    client_env.sin_port = htons(PORT);
    memset(&(client_env.sin_zero), 0, 8);
    
    // Start client socket
    client_socket = connect_client(&client_env);
    std::cout << "\033[2J\033[1;1H";
    recv(client_socket, rcvbuff, sizeof(rcvbuff), 0);
    std::cout << rcvbuff << std::endl;
    
    // Respond with a name
    input_verification(name, 255, "your name: ");
    send(client_socket, name, strlen(name), 0);
    recv(client_socket, rcvbuff, BUFFERSIZE, 0);
    
    std::cout << rcvbuff << std::endl;
    
    if (strncmp(sndbuff, CONNECTED, strlen(sndbuff)) == 0) communicate();
    else{
        std::cout << "failed to connect" << std::endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
}