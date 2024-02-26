#include "..\..\Privy\header.cpp"
#include <unordered_map>
#include <algorithm>

#define CONNECTED "CONNECTED"
#define DECLINED "DECLINED"
#define UNRECOGNIZED "UNRECOGNIZED"
#define CHATLOG_LIMIT 756

struct room{
    char chat_log[CHATLOG_LIMIT];       // current chat log of room
    bool is_private;
    char description[256];     // description up to 255
    char password[21];         // password up to 20
    fd_set rf;                 // registered file descriptors

    room(){
        memset(description, '\0', 256);
        memset(password, '\0', 21);
        memset(chat_log, '\0', CHATLOG_LIMIT);
        FD_ZERO(&rf);
    }
};

std::unordered_map<int, room> rooms;
std::unordered_map<int, char*> clients; 

int ID = 10000;                 // Determine the ID of every room. NOTE this is not scalable

fd_set fp;                      // Clients who are asked to create a name
fd_set fc;                      // Clients already in the system
fd_set fr, fw, fe;              // Sockets ready to read, write and throwing an exception

char rcvbuff[BUFFERSIZE];       // Buffer where recieved data will go in (temporary)
char sndbuff[BUFFERSIZE];       // Buffer for sending data

struct timeval tv = {1, 0};     // 1 second delay
int nret;                       // return value of every select call

sockaddr c_info;                // where info of the connecting client will be stored

// type of commands
const char *CREATE_ROOM = "crm";
const char *JOIN_ROOM = "jrm";
const char *VIEW_ROOMS = "vrm";
const char *VIEW_ROOM_DETAIL = "vrd";
const char *PASSWORD = "pwd";
const char *REFRESH = "rfs";
const char *SEND = "snd";

void clear_buff(char buffer[], int size){
    if (strlen(buffer) > 0){
        memset(buffer, '\0', size);
    }
}
 
void remove_socket(int socket){
    // for now delete this socket to the server
    if (FD_ISSET(socket, &fp)){
        std::cout << "removing pending socket";
        std::cout << "\nsocket: " << socket << '\n';
        FD_CLR(socket, &fp);
        closesocket(socket);
    }
    else if (FD_ISSET(socket, &fc)){
        std::cout << "removing client";
        std::cout << "\nname: " << clients[socket];
        std::cout << "\nsocket: " << socket;
        // remove from all rooms he's at
        delete [] clients[socket];
        clients.erase(socket);
        FD_CLR(socket, &fc);
        closesocket(socket);
    }
}

void listen_to_requests(int server_socket){
    FD_ZERO(&fp);   // initializes file descriptors for pending
    FD_ZERO(&fc);   // initializes file descriptors for clients

    while (true){
        // clear sets
        FD_ZERO(&fr);
        FD_ZERO(&fw);
        FD_ZERO(&fe);

        // place main socket to accept requests
        FD_SET(server_socket, &fr);
        FD_SET(server_socket, &fe);

        // place every pending socket and online client sockets on fr and fe
        place(&fp, &fr);
        place(&fp, &fe);
        place(&fc, &fr);
        place(&fc, &fe);

        nret = select(server_socket + 1 + fc.fd_count + fp.fd_count, &fr, &fw, &fe, &tv);

        if (nret > 0){
            // Handle every socket ready to read
            for (int i = 0; i < fr.fd_count; i++){
                int socket_to_check = fr.fd_array[i];

                if (socket_to_check == server_socket){
                    // someones trying to connect 
                    std::cout << "someone is requesting access\n";
                    int nlen = sizeof(c_info);
                    int client_socket = accept(server_socket, &c_info, &nlen);

                    strcpy(sndbuff, "====== WELCOME TO PRIVY ======");
                    send(client_socket, sndbuff, strlen(sndbuff), 0);

                    FD_SET(client_socket, &fp);
                }
                else if (FD_ISSET(socket_to_check, &fp)){
                    // we expect that this is the name of the client
                    clear_buff(rcvbuff, BUFFERSIZE);
                    ssize_t bytes_read = recv(socket_to_check, rcvbuff, BUFFERSIZE, 0);
                    // checks whether client responds
                    if (bytes_read < 0){
                        remove_socket(socket_to_check);
                        continue;
                    }
                   
                    // Send confirmation
                    strcpy(sndbuff, CONNECTED);
                    send(socket_to_check, sndbuff, strlen(sndbuff), 0);
                    
                    // remove to fp add to fc
                    FD_CLR(socket_to_check, &fp);
                    FD_SET(socket_to_check, &fc);

                    // update clients
                    rcvbuff[strcspn(rcvbuff, "\n")] = '\0';
                    clients[socket_to_check] = new char[strlen(rcvbuff) + 1];
                    memset(clients[socket_to_check], '\0', strlen(rcvbuff) + 1);
                    strcpy(clients[socket_to_check], rcvbuff);


                    std::cout << "a client is created" << '\n' 
                              << "name: " << rcvbuff << "\nsocket: " << socket_to_check << '\n';
                    std::cout << "total clients: " << fc.fd_count << '\n';
                }
                else if (FD_ISSET(socket_to_check, &fc)){
                    clear_buff(rcvbuff, BUFFERSIZE);
                    ssize_t bytes_read = recv(socket_to_check, rcvbuff, BUFFERSIZE, 0);
                    // checks whether client responds
                    if (bytes_read < 0){
                        remove_socket(socket_to_check);
                        continue;
                    }
                     std::replace(rcvbuff, rcvbuff + strlen(rcvbuff), '|', '\0');

                    char cmd[4];
                    memset(cmd, '\0', 4);
                    strcpy(cmd, rcvbuff);  // get the type of cmd
                    
                    if (strcmp(cmd, CREATE_ROOM) == 0){
                        room new_room;
        
                        new_room.is_private = rcvbuff[4] == '1';
                        int desc_len = strlen(rcvbuff + 6);
                        strcpy(new_room.description, rcvbuff + 6);

                        if (new_room.is_private)
                            strcpy(new_room.password, rcvbuff +  desc_len + 7);
                        
                        FD_SET(socket_to_check, &(new_room.rf));

                        clear_buff(sndbuff, BUFFERSIZE);
                        snprintf(sndbuff, 6, "%d", ID);
                        send(socket_to_check, sndbuff, strlen(sndbuff), 0);

                        rooms[ID++] = new_room;
                    }
                    else if (strcmp(cmd, JOIN_ROOM) == 0){
                        int chosen_id = atoi(rcvbuff + 4);
                        if (rooms.count(chosen_id) == 0){
                            send(socket_to_check, DECLINED, strlen(DECLINED), 0);
                            continue;
                        }
                        
                        send(socket_to_check, (rooms[chosen_id].is_private ? "PRV" : "PUB"), 4, 0);
                    }
                    else if (strcmp(cmd, PASSWORD) == 0){
                        int chosen_id = atoi(rcvbuff + 4);
                        char password[22];
                        strcpy(password, rcvbuff + 10);
                        
                        if (rooms.count(chosen_id) > 0 && strncmp(rooms[chosen_id].password,
                         password, sizeof(rooms[chosen_id].password)) == 0){
                            FD_SET(socket_to_check, &(rooms[chosen_id].rf));
                            send(socket_to_check, CONNECTED, strlen(CONNECTED), 0);
                        }
                        else send(socket_to_check, DECLINED, strlen(DECLINED), 0);
                    }
                    else if (strcmp(cmd, REFRESH) == 0){
                        int chosen_id = atoi(rcvbuff + 4);
                        
                        if (rooms.count(chosen_id) == 0){
                            send(socket_to_check, DECLINED, strlen(DECLINED), 0);
                            continue;
                        }
                        if (!FD_ISSET(socket_to_check, &(rooms[chosen_id].rf))){
                             send(socket_to_check, DECLINED, strlen(DECLINED), 0);
                            continue;
                        }

                        clear_buff(sndbuff, BUFFERSIZE);
                        strcpy(sndbuff, "CONNECTED|");
                        if (strlen(rooms[chosen_id].chat_log)){
                            strcat(sndbuff, rooms[chosen_id].chat_log);
                        }
                        send(socket_to_check, sndbuff, strlen(sndbuff), 0);
                    }
                    else if (strcmp(cmd, SEND) == 0){
                        int chosen_id = atoi(rcvbuff + 4);

                        if (rooms.count(chosen_id) == 0){
                            send(socket_to_check, DECLINED, strlen(DECLINED), 0);
                            continue;
                        }

                        if (!FD_ISSET(socket_to_check, &(rooms[chosen_id].rf))){
                             send(socket_to_check, DECLINED, strlen(DECLINED), 0);
                            continue;
                        }

                        char *cl = rooms[chosen_id].chat_log;
                        strcat(cl, "\n");
                        strcat(cl, clients[socket_to_check]);
                        strcat(cl, ": \n");
                        strcat(cl, rcvbuff + 10); // message
                        strcat(cl, "\n");
                    }
                    else if (strcmp(cmd, VIEW_ROOMS) == 0){
                        if (!FD_ISSET(socket_to_check, &fc)){
                            send(socket_to_check, DECLINED, sizeof(DECLINED), 0);
                            continue;
                        }
                        clear_buff(sndbuff, BUFFERSIZE);
                        strcpy(sndbuff, CONNECTED);
                        
                        for (auto itr = rooms.begin(); itr != rooms.end(); itr++){
                            strcat(sndbuff, "|");
                            strcat(sndbuff, std::to_string(itr->first).c_str());
                            strcat(sndbuff, "|");
                            strcat(sndbuff, ((itr->second).is_private ? "PRV" : "PUB"));
                        }

                        std::cout << sndbuff << '\n';
                        send(socket_to_check, sndbuff, strlen(sndbuff), 0);
                    }
                    else if (strcmp(cmd, VIEW_ROOM_DETAIL) == 0){
                        int chosen_id = atoi(rcvbuff + 4);

                        if (rooms.count(chosen_id) == 0){
                            send(socket_to_check, DECLINED, strlen(DECLINED), 0);
                            continue;
                        }

                        room rm = rooms[chosen_id];

                        clear_buff(sndbuff, BUFFERSIZE);
                        strcpy(sndbuff, CONNECTED);
                        strcat(sndbuff, (rm.is_private ? "|PRV|" : "|PUB|"));
                        strcat(sndbuff, std::to_string(rm.rf.fd_count).c_str());
                        strcat(sndbuff, "|");
                        strcat(sndbuff, rm.description);

                        std::cout << sndbuff << '\n';
                        send(socket_to_check, sndbuff, strlen(sndbuff), 0);
                    }
                    else{
                        send(socket_to_check, UNRECOGNIZED, sizeof(UNRECOGNIZED), 0);
                        std::cout << "unrecognized command: " << cmd << '\n';
                    }
                }
            }

            // Handle sockets throwing an exception
            for (int i = 0; i < fe.fd_count; i++){
                int socket_to_check = fe.fd_array[i];
                remove_socket(socket_to_check);
            }
        }  
        else if (nret == 0){
            // Nothing on port
        }
        else{
            std::cout << "oops" << std::endl;
            WSACleanup();
            exit(EXIT_FAILURE);
            // something wrong happened
        }
    }
}

// Returns a socket for the server using the given environment
int start_server(sockaddr_in *server_env){
    WSADATA wsd;
    test(WSAStartup(MAKEWORD(2, 2), &wsd), "Initializing windows sockets");

    int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    test(server_socket, "Creating server socket");

    int enable_keep_alive;
    test(bind(server_socket, (sockaddr*) server_env, sizeof(*server_env)), "Binding server socket to environment");

    test(listen(server_socket, 5), "Listening to Port");

    return server_socket;
}

int main(int args, char **arsv){
    // Initialize environment for socket
    sockaddr_in server_env;
    server_env.sin_family = AF_INET;
    server_env.sin_addr.s_addr = inet_addr(ADDR);
    server_env.sin_port = htons(PORT);
    memset(&(server_env.sin_zero), 0, 8);
    // Start sever
    int server_socket = start_server(&server_env);
    std::cout << "\033[2J\033[1;1H";
    std::cout << "===== SERVER HAS STARTED! =====" << '\n';
    listen_to_requests(server_socket);
}