#include<iostream>
#include<unistd.h>
#include<cstring>
#include<thread>
#include<vector>
#include<map>
#include<mutex>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sqlite3.h>
#include<algorithm>


using namespace std;
vector<int> clients;
map<int, string> usernames;
mutex clients_mutex;
sqlite3* db;

void broadcast_message(string message, int sender_socket)
{
 clients_mutex.lock();
    for(int client : clients)
    {
        if(client != sender_socket)
        {
            string msg_with_newline = message + "\n";
            send(client, msg_with_newline.c_str(), msg_with_newline.length(), 0);
        }
    }
    clients_mutex.unlock();
}
void save_message(string username, string message)
{
     string safe_msg = message;
    string safe_user = username;
    size_t pos = 0;
    while((pos = safe_msg.find("'", pos)) != string::npos) { safe_msg.replace(pos, 1, "''"); pos += 2; }
    pos = 0;
    while((pos = safe_user.find("'", pos)) != string::npos) { safe_user.replace(pos, 1, "''"); pos += 2; }

    string sql = "INSERT INTO messages(username, message) VALUES('" + safe_user + "','" + safe_msg + "');";
    char* err = nullptr;
    sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err);
    if(err) { cout << "DB insert failed\n"; sqlite3_free(err); }
}
vector<string> db_history(int limit = 20)
{
    vector<string> rows;

    sqlite3_stmt* st;

    if(sqlite3_prepare_v2(
        db,
        "SELECT username,message,timestamp FROM messages ORDER BY id DESC LIMIT ?;",
        -1,
        &st,
        nullptr
    ) == SQLITE_OK)
    {
        sqlite3_bind_int(st, 1, limit);

        while(sqlite3_step(st) == SQLITE_ROW)
        {
            string u = (const char*)sqlite3_column_text(st, 0);

            string m = (const char*)sqlite3_column_text(st, 1);

            string t = (const char*)sqlite3_column_text(st, 2);

            rows.push_back("[" + t + "] " + u + ": " + m);
        }

        sqlite3_finalize(st);
    }

    reverse(rows.begin(), rows.end());

    return rows;
}
void receive_messages(int client_socket)
{
    while(true)
    {
        char buffer[1024] = {0};
        int bytes = read(client_socket, buffer, 1024);
        if(bytes <= 0)
        {
            cout << usernames[client_socket] << " disconnected\n";
            clients_mutex.lock();
            clients.erase(remove(clients.begin(), clients.end(), client_socket), clients.end());
            string username = usernames[client_socket];
            usernames.erase(client_socket);
            clients_mutex.unlock();
            close(client_socket);
            break;
        }
        string message = usernames[client_socket] + ": " + string(buffer);
        save_message(usernames[client_socket], string(buffer));
        broadcast_message(message, client_socket);
    }
}
void db_init()
{
    if(sqlite3_open("chat.db", &db) != SQLITE_OK)
    {
        cout << "Database failed\n";
        exit(1);
    }

    const char* sql =
    "CREATE TABLE IF NOT EXISTS messages("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "username TEXT NOT NULL,"
    "message TEXT NOT NULL,"
    "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";

    char* err = nullptr;

    sqlite3_exec(db, sql, nullptr, nullptr, &err);

    if(err)
    {
        cout << err << endl;
        sqlite3_free(err);
    }
    else
    {
        cout << "Database ready\n";
    }
}

int main()
{
    db_init();
      int server_fd=socket(AF_INET,SOCK_STREAM,0);
      if(server_fd<0)
      {
        cout<<"Socket failed";
      }
      sockaddr_in address;
      address.sin_family = AF_INET;
      address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(9090);
    if(bind(server_fd, (sockaddr*)&address, sizeof(address))<0)
    {
        cout<<"Bind failed";
    }
    if(listen(server_fd,3)<0)
    {
        cout<<"Listen failed";
    }
    cout<<"Server waiting for clients...\n";
    
    while(true)
    {
        int client_socket = accept(server_fd, NULL, NULL);
        if(client_socket < 0)   
        {
            cout << "Accept failed";
            continue;
        }   
        cout << "Client connected successfully\n";
        clients_mutex.lock();
        clients.push_back(client_socket);
        clients_mutex.unlock();
        char username[100] = {0};

read(client_socket, username, 100);

usernames[client_socket] = username;

cout << username << " joined the chat\n";
// With this:
vector<string> history = db_history();
string history_block = "";
for(string msg : history)
{
    history_block += msg + "\n";
}
if(!history_block.empty())
{
    string marker = "HISTORY_START\n" + history_block + "HISTORY_END\n";
    send(client_socket, marker.c_str(), marker.length(), 0);
}
        thread receiver(receive_messages, client_socket);
        receiver.detach();
    }

    return 0;

}