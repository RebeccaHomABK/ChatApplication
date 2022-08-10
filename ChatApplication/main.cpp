#include <enet/enet.h>

#include <iostream>
#include <thread>
#include <chrono>

ENetAddress address;
ENetHost* server = nullptr;
ENetHost* client = nullptr;

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
                event.peer->data = (void*)("Client information");

                {
                    ENetPacket* packet = enet_packet_create("hello",
                        strlen("hello") + 1,
                        ENET_PACKET_FLAG_RELIABLE);
                    enet_host_broadcast(server, 0, packet);
                    //enet_peer_send(event.peer, 0, packet);

                    //enet_host_service();
                    enet_host_flush(server);
                }
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                std::cout << "A packet of length "
                    << event.packet->dataLength << std::endl
                    << "containing " << (char*)event.packet->data
                    << std::endl;
                //<< "was received from " << (char*)event.peer->data
                //<< " on channel " << event.channelID << std::endl;
            /* Clean up the packet now that we're done using it. */
                enet_packet_destroy(event.packet);

                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << (char*)event.peer->data << "disconnected." << std::endl;
                /* Reset the peer's client information. */
                event.peer->data = NULL;
            }
        }
    }
}

void ChatClient()
{
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

    if (enet_host_service(client, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT)
    {
        std::cout << "Connection to " << host << ":" << address.port << " succeeded." << std::endl;
    }
    else
    {
        enet_peer_reset(peer);
        std::cout << "Connection to " << host << ":" << address.port << " failed." << std::endl;
    }

    while (1)
    {
        ENetEvent event;

        while (enet_host_service(client, &event, 1000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_RECEIVE:
                std::cout << "A packet of length "
                    << event.packet->dataLength << std::endl
                    << "containing " << (char*)event.packet->data
                    << std::endl;

                enet_packet_destroy(event.packet);

                {
                    ENetPacket* packet = enet_packet_create("hi",
                        strlen("hi") + 1,
                        ENET_PACKET_FLAG_RELIABLE);

                    enet_host_broadcast(client, 0, packet);
                    //enet_peer_send(event.peer, 0, packet);

                    enet_host_flush(client);
                }
            }
        }
    }
}

int main(int argc, char** argv)
{
    int UserInput = StartChat();
    
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