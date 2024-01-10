//=========================== file = tcp2WayChat.c ============================
//=  2-way chat program with home port number, visiting port number,          =
//=  and IP address as command line inputs.                                   =
//=                                                                           =
//=  With these 3 inputs and a copy of this exact program, you should         =
//=  be able to establish a 2 way text communication via transmission         =
//=  control protocol on any computer                                         =
//=============================================================================
//=  Build:                                                                   =
//=    Windows (WIN):  Visual C: cl tcp2WayChat.c wsock32.lib                 =
//=---------------------------------------------------------------------------=
//=  Execute: tcp2WayChat <HOME_PORT> <VISITOR_PORT> <IP_ADDR>                =
//=---------------------------------------------------------------------------=
//=  Author: Ryan Stapf                                                       =
//=          University of South Florida                                      =
//=          Email: ryanstapf@usf.edu                                         =
//=============================================================================
#define  WIN                // WIN for Winsock and BSD for BSD sockets

//----- Include files ---------------------------------------------------------
#include <stdio.h>          // Needed for printf()
#include <string.h>         // Needed for memcpy() and strcpy()
#include <stdlib.h>         // Needed for exit()
#include <stdbool.h>        // Needed to define a variable as a boolean
#include <fcntl.h>
#include <windows.h>        // Needed for all Winsock stuff
#include <process.h>        // Needed for thread functions
#include <stddef.h>     
#include <sys\stat.h>   
#include <io.h>       

// ---- Defines and Global Variables ------------------------------------------
#define  MAX              80
#define  MAX_BUF          4096

int   VISITOR_PORT_NUM; // Port Number for other program defined in command line
int   HOME_PORT_NUM;    // Port Number for this program defined in command line
char  IP_ADDR[256];     // Shared IP address for both parties defined in command line
char  out_buf[MAX];     // Output buffer for outgoing messages
char  in_buf[MAX];      // Input buffer for incoming messages

// ---- Fucntions -------------------------------------------------------------

// Define threads so that the program may act as a server and client 
// simultaneously with the ability to both initiate and receive connections,
// and to both send and receive messages.


// Define the server thread
void* serverThread(void* args) {

    // ----- Local Variables --------------------------------------------------
    struct sockaddr_in client_addr;       // client IP address
    struct sockaddr_in server_addrServer; // server IP address (server thread variant)
    int connect_s;                        // Connection Socket
    int addr_len;                         // Address Length
    int retcode;                          // Return Code
    int server_s;                         // Server Socket
    bool running = TRUE; // Boolean variable to keep the message receiver running
    // ------------------------------------------------------------------------

    // Create the server socket
    server_s = socket(AF_INET, SOCK_STREAM, 0);
    if (server_s < 0)
    {
        printf("*** ERROR - server socket() failed  \n");
        exit(-1);
    }

    // Set the port number and IP address
    server_addrServer.sin_family = AF_INET;                 // Address family to use
    server_addrServer.sin_port = htons(HOME_PORT_NUM);      // Assign home port number
    server_addrServer.sin_addr.s_addr = htonl(INADDR_ANY);  // Server should listen on any IP address

    // Bind the socket
    retcode = bind(server_s, (struct sockaddr*)&server_addrServer, sizeof(server_addrServer));
    if (retcode < 0)
    {
        printf("*** ERROR - bind() failed \n");
        exit(-1);
    }

    // Listen for Connection
    printf("Waiting for accept() to complete...\n");
    listen(server_s, 1);

    // Connect to incoming client
    addr_len = sizeof(client_addr);
    connect_s = accept(server_s, (struct sockaddr*)&client_addr, &addr_len);
    if (connect_s < 0)
    {
        printf("*** ERROR - accept() failed \n");
        exit(-1);
    }
    printf("Connected and ready to recieve messages...\n");

    // Receive messages from the client while the connection is alive
    while (running) {
        retcode = -1;
        while (retcode < 0) {
            retcode = recv(connect_s, in_buf, sizeof(in_buf), 0);
        }
        if (strlen(in_buf) > 0) {
            printf(">>> %s\n", in_buf);
        }
    }
}


// Define client thread
void* clientThread(void* args) {

    // ----- Local Variables --------------------------------------------------
    struct sockaddr_in server_addrClient; // server IP address (client thread variant)
    int client_s;                         // Client Socket
    int retcode;                          // Return Code
    bool running = TRUE; // Boolean variable to keep the message prompt running until user decides to quit
    // ------------------------------------------------------------------------

    // Create the client socket
    client_s = socket(AF_INET, SOCK_STREAM, 0);
    if (client_s < 0) {
        printf("*** ERROR - socket() failed \n");
        exit(-1);
    }

    // Set the port number and IP address
    server_addrClient.sin_family = AF_INET;                  // Address family to use
    server_addrClient.sin_port = htons(VISITOR_PORT_NUM);    // Assign visitor port number
    server_addrClient.sin_addr.s_addr = inet_addr(IP_ADDR);  // Assign Client IP address

    // Keep attempting until a successful connection with the server
    while (running) {
        retcode = connect(client_s, (struct sockaddr*)&server_addrClient, sizeof(server_addrClient));
        if (retcode >= 0) {
            break;
        }
        Sleep(100);
    }
    printf("Connected and ready to send messages... \n");

    // Keep running the chat prompt until the user enters "Exit"
    while (running) {
        fflush(stdin);
        fgets(out_buf, sizeof(out_buf), stdin);
        if (!strcmp(out_buf, "Exit\n")) {
            running = FALSE;
            strcpy(out_buf, "***USER DISCONNECTED***\n");
            retcode = send(client_s, out_buf, (strlen(out_buf) + 1), 0);
            exit(-1);
        }
        retcode = send(client_s, out_buf, (strlen(out_buf) + 1), 0);
    }
}


//===== Main program ============================================================
int main(int argc, char* argv[])
{
    // Read the command line for the Home port number, Visitor port number, and IP Address
    if (argc != 4) {
        printf("Input: <file name> <Home Port Num> <Visitor Port Num> <IP ADDR>");
    }
    HOME_PORT_NUM = atoi(argv[1]);
    VISITOR_PORT_NUM = atoi(argv[2]);
    strcpy(IP_ADDR, argv[3]);

    // Needed for WSA functions
    WORD wVersionRequested = MAKEWORD(1, 1);
    WSADATA wsaData;


    // Initializes winsock
    WSAStartup(wVersionRequested, &wsaData);


    // Begin the server thread
    if (_beginthread(serverThread, MAX_BUF, (void*)HOME_PORT_NUM) < 0) {
        printf("Error - Unable to Create Server Thread");
        exit(1);
    }


    // Begin the client thread
    if (_beginthread(clientThread, MAX_BUF, (void*)VISITOR_PORT_NUM) < 0) {
        printf("Error - Unable to Create Client Thread");
        exit(1);
    }

    // Keeps the threads running until user quits
    while (TRUE) {}

    // Clean-up winsock
    WSACleanup();

    // Return zero and terminate
    return(0);
}