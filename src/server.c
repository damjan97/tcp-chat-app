/*
    ********************************************************************
    Odsek:          Elektrotehnika i racunarstvo
    Departman:      Racunarstvo i automatika
    Katedra:        Racunarska tehnika i racunarske komunikacije (RT-RK)
    Predmet:        Osnovi Racunarskih Mreza 1
    Godina studija: Treca (III)
    Skolska godina: 2017/2018
    Semestar:       Zimski (V)
	Autori:			Damjan Glamočić (RA65/2015) i Mihailo Marković (RA191/2015)
	
    Ime fajla:      server.c
    Opis:           TCP/IP server

    Platforma:      Raspberry Pi 2 - Model B
    OS:             Raspbian
    ********************************************************************
*/
/*
	********************************************************************
	Zadatak:
	Implementirati centralizovani sistem za razmenu tekstualnih poruka
	koji se sastoji iz klijentske i serverske aplikacije koje
	komuniciraju posredstvom TCP protokola. Po uspostavljanju veze
	klijent se prijavljuje na server slanjem korisnickog imena i
	lozinke. Posle uspesne autentifikacije, sve poruke klijenta
	prosledjuju se svim ostalim trenutno aktivnim klijentima. Prilikom
	prosledjivanja poruke potrebno je poslati i korisnicko ime klijenta
	koji je poslao poruku. Takodje je potrebno obavestiti sve aktivne
	klijente kada neko uspostavi ili raskine vezu sa serverom. Po
	raskidanju veze, klijent se izbacuje iz liste aktivnih klijenata.

	********************************************************************
*/

#include<stdio.h>		//for printf
#include<string.h>    	//strlen
#include<sys/socket.h>	//for socket
#include<arpa/inet.h> 	//inet_addr
#include<unistd.h>    	//write
#include<pthread.h>		//thread
#include <stdlib.h>     //exit

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT   27015
#define MAX_CLIENTS 32

typedef int bool;										//create bool type
enum { false, true };

int clientsActive = 0;									//number of active clients
int clientList[MAX_CLIENTS];							//list of active clients - value is socket
pthread_mutex_t listAccess;								//mutex for shared variables clientList and clientsActive

/*Function for checking if client logged out*/
bool ClientLoggedOut(char message[]);
/*Function which finds index of client*/
int FindIndexOfClient(int sock);
/*Function which deletes logged out client*/
void DeleteLoggedOutClient(int ind);
/*Thread function for establishing connection with multiple clients*/
void* Handler(void* pParam);

int main(int argc , char *argv[])
{
    int socketDesc , clientSock , c, j;
    struct sockaddr_in server , client;

    pthread_t threads[MAX_CLIENTS];							//threads for multiple clients

	/*Create socket*/
    socketDesc = socket(AF_INET , SOCK_STREAM , 0);
    if (socketDesc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
	
	/*Prepare the sockaddr_in structure*/
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(DEFAULT_PORT);
	
	/*Bind ip address to socket*/
    if( bind(socketDesc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("bind failed. Error");
        return 1;
    }
    puts("Bind done");
	
	/*Listen for socket connections*/
    listen(socketDesc , 3);

    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

	/*Initialize mutex*/
	pthread_mutex_init(&listAccess, NULL);
	
	/*main while*/
    while(1)
    {
		/*Accept connection from an incoming client*/
        clientSock = accept(socketDesc, (struct sockaddr *)&client, (socklen_t*)&c);

        if (clientSock < 0)
        {
            perror("Accept failed");
            return 1;
        }
        else
        {
			pthread_mutex_lock(&listAccess);			
            clientList[clientsActive]= clientSock;				//put client socket to a list of active clients
            clientsActive++;
            puts("Connection accepted");			
			pthread_mutex_unlock(&listAccess);
        }

		/*Create thread for each client*/
        if (pthread_create(threads + j++, NULL, Handler, (void*)&clientSock) != 0)
        {
            perror("Couldn't create thread, exiting\n");
            return 1;
        }
    }
	
	/*Destroy mutex*/
	pthread_mutex_destroy(&listAccess);
	
	/*Close sockets*/
    close(clientSock);
    close(socketDesc);
    return 0;
}

bool ClientLoggedOut(char message[])
{
	int i = 0;
	char justLoggedOut[15];
	/*To check if message is "username just logged out" */
	while(message[i] != 32)
	{
		i++;
	}
	i++;
	
	strncpy(justLoggedOut, &message[i], 15);

	if(strcmp(justLoggedOut, "just logged out") == 0)
	{
		return true;
	}else
	{
		return false;
	}
}

int FindIndexOfClient(int sock)
{
	int i;
	
	for(i=0; i<clientsActive; i++)
	{
		if(clientList[i] == sock)
		{
			return i;
		}
	}
	
	printf("There is no client with socket %d\n", sock);
	
	return -1;
}

void DeleteLoggedOutClient(int ind)
{
	int i;
	
	/*Delete client on position ind */		
	for(i=ind; i<clientsActive; i++)
	{
		clientList[i] = clientList[i+1];
	}
	clientsActive --;	
}

void* Handler(void* pParam)
{
    int i;
    char message[256];
    int socket = *(int*)pParam;
    int readSize;
	int index;

	/*Receive message "username just logged in" */
	if((readSize = recv(socket , message , DEFAULT_BUFLEN , 0)) > 0)
	{
		message[readSize] = '\0';
		puts(message);

		/*Send message "username just logged in" to all active clients except client who sent message*/		
		pthread_mutex_lock(&listAccess);
        for(i = 0; i < clientsActive; i++)
        {
			if(clientList[i] != socket)
			{
				if(send(clientList[i], message, strlen(message), 0) < 0)
				{
					puts("Send failed");
					exit(EXIT_FAILURE);
				}
			}
        }
		pthread_mutex_unlock(&listAccess);
	}else
    {
        perror("recv failed");
    }
	printf("Active clients: %d\n", clientsActive);
	
	/*Receive messages from clients*/
    while( (readSize = recv(socket , message , DEFAULT_BUFLEN , 0)) > 0 )
    {
		message[readSize] = '\0';

		/*If message is "username just logged out"*/
		if(ClientLoggedOut(message))
		{
			pthread_mutex_lock(&listAccess);
			index = FindIndexOfClient(socket);					//find index of client who logged out
			DeleteLoggedOutClient(index);						//delete client from list of active clients
			printf("Active clients: %d\n", clientsActive);
			pthread_mutex_unlock(&listAccess);
		}

		/*/*Send message to all active clients except client who sent message*/
		pthread_mutex_lock(&listAccess);
        for(i = 0; i < clientsActive; i++)
        {
			if(clientList[i] != socket)
			{
				if(send(clientList[i], message, strlen(message), 0) < 0)
				{
					puts("Send failed");
					exit(EXIT_FAILURE);
				}
			}
        }
		pthread_mutex_unlock(&listAccess);
    }

	if(readSize == -1)
    {
        perror("recv failed");
    }

    return 0;
}

