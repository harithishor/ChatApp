#include<iostream>
#include<unistd.h>
#include<arpa/inet.h>
#include<thread>
#include<sys/socket.h>
#include<cstring>

using namespace std;

void receive_messages(int sock)
{
    while(true)
    {
        char buffer[1024] = {0};
        int bytes = read(sock, buffer, 1023);
        if(bytes <= 0)
        {
            cout << "Server disconnected";
            break;
        }
        buffer[bytes] = '\0';
        cout << "Message from server: " << buffer << endl;
    }
}


int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock < 0)
    {
        cout << "Socket creation failed";
    }

    sockaddr_in serv_addr;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9090);

    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if(connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        cout << "Connection failed";
    }

    cout << "Connected to server successfully\n";
    char username[100];

cout << "Enter username: ";

cin.getline(username, 100);

send(sock, username, strlen(username), 0);
    thread receiver(receive_messages, sock);

while(true)
{
    char msg[1024];

    cout << "Enter message: ";

    cin.getline(msg, 1024);
    if(strlen(msg) == 0) continue;


    send(sock, msg, strlen(msg), 0);
    if(strcmp(msg, "exit") == 0)
{
    break;
}
}
receiver.join();

    return 0;
}