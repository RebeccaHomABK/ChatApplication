#include <enet/enet.h>

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

ENetAddress address;
ENetHost* server = nullptr;
ENetHost* client = nullptr;

struct Chat
{
    std::string m_name;
    std::string m_content;
    bool m_lock = false;
    
    Chat(std::string name, std::string content)
        : m_name(name), m_content(content)
    {};

    std::string Message() { return m_name + ": " + m_content; }
};

//  Creates the chat server
bool CreateServer()
{
    address.host = ENET_HOST_ANY;
    address.port = 1234;
    
    server = enet_host_create(&address, 32, 2, 0, 0);
    return server != nullptr;
}


//  Creates the chat client
bool CreateClient()
{
    client = enet_host_create(NULL, 1, 2, 0, 0);
    return client != nullptr;
}

//  Initializes the chat
int StartChat()
{
    //  Initializes enet
    if (enet_initialize() != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        std::cout << "An error occurred while initializing ENet." << std::endl;
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    //  Gets the user input
    int UserInput = 0;
    bool isCorrectInput = false;

    do {
        std::cout << "1) Create Server " << std::endl;
        std::cout << "2) Create Client " << std::endl;
        std::cin >> UserInput;

        isCorrectInput = ((UserInput > 0) && (UserInput < 3));

        if (!isCorrectInput)
        {
            std::cout << "Invalid input!" << std::endl;
        }
    } while (!isCorrectInput);

    return UserInput;
}

//  Sends a message
void MessageSend(Chat* chat)
{
    std::string content = "";
    while (1)
    {
        std::getline(std::cin, content);
        chat->m_content = content;
        chat->m_lock = true;
        content = "";
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

//  Sends a packet
void PacketSend(std::string message, ENetHost* host = client)
{
    ENetPacket* packet = enet_packet_create(message.c_str(), message.length() + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(host, 0, packet);
    enet_host_flush(host);
}

//  Chat server functionality
void ChatServer()
{
    while (1)
    {
        ENetEvent event;
        while (enet_host_service(server, &event, 1000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                std::cout << "A new client connected from "
                    << event.peer->address.host
                    << ":" << event.peer->address.port
                    << std::endl;

                break;
            case ENET_EVENT_TYPE_RECEIVE:
                std::cout << (char*)event.packet->data << std::endl;
                enet_packet_destroy(event.packet);

                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << (char*)event.peer->data << "disconnected." << std::endl;
                event.peer->data = NULL;
            }
        }
    }
}

void ChatClient()
{
    //  Gets the user's name.
    Chat* chat = new Chat("", "");
    std::cout << "Username: ";
    std::cin.ignore(999, '\n');
    std::getline(std::cin, chat->m_name);

    system("cls");
    std::cout << "Welcome, " << chat->m_name << "!" << std::endl;

    //  Connect to the client.
    ENetAddress address;
    ENetEvent event;
    ENetPeer* peer;
    char host[] = "127.0.0.1";

    enet_address_set_host(&address, host);
    address.port = 1234;

    peer = enet_host_connect(client, &address, 2, 0);
    if (peer == NULL)
    {
        fprintf(stderr,
            "No available peers for initiating an ENet connection.\n");
        exit(EXIT_FAILURE);
    }

    if ((enet_host_service(client, &event, 1000) > 0) &&
        event.type == ENET_EVENT_TYPE_CONNECT)
    {
        std::cout << "Connection to " << host << ":" << address.port << " succeeded." << std::endl;

        //  Sends a message to the entire chat room that the user has entered.
        std::string welcome = chat->m_name + " has entered the chat.\n";
        PacketSend(welcome);

        //  Give instructions on what to do
        std::cout << "Type in the chat below and hit enter to send. Type \\q to quit." << std::endl;
        std::thread ChatThread(MessageSend, chat);
        ChatThread.detach();
    }
    else
    {
        enet_peer_reset(peer);
        std::cout << "Connection to " << host << ":" << address.port << " failed." << std::endl;
    }

    while (1)
    {
        if (chat->m_lock)
        {
            if ((chat->m_content[0] == '\\') && (chat->m_content[1] == 'q'))
            {
                chat->m_content = chat->m_name + " has left the chat.\n";
                PacketSend(chat->m_content, client);
                delete chat;
                return;
            }

            PacketSend(chat->Message(), client);
            chat->m_lock = false;
        }

        ENetEvent event;

        while (enet_host_service(client, &event, 500) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_RECEIVE:
                std::cout << (char*)event.packet->data << std::endl;
                enet_packet_destroy(event.packet);
            }
        }
    }
}

int main(int argc, char** argv)
{
    int UserInput = StartChat();
    system("cls");
    
    if (UserInput == 1)
    {
        if (!CreateServer())
        {
            fprintf(stderr,
                "An error occurred while trying to create an ENet server host.\n");
            exit(EXIT_FAILURE);
        }
        
        ChatServer();
    }
    else if (UserInput == 2)
    {
        if (!CreateClient())
        {
            fprintf(stderr,
                "An error occurred while trying to create an ENet client host.\n");
            exit(EXIT_FAILURE);
        }

        ChatClient();
    }
    
    //  Cleans up the program
    if (server != nullptr)
    {
        enet_host_destroy(server);
    }

    if (client != nullptr)
    {
        enet_host_destroy(client);
    }
    
    return EXIT_SUCCESS;
}