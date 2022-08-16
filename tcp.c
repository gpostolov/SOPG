#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#include "SerialManager.h"

extern int newfd;
extern pthread_mutex_t mutex_newfd;

static int s;

void * tcp_main(void* message)
{
    socklen_t addr_len;
    struct sockaddr_in clientaddr;
    struct sockaddr_in serveraddr;
    char buffer[128];

    int n;

    /* INICIO INICIALIZACION TCP SERVER */
    // Creamos socket
    s = socket(AF_INET,SOCK_STREAM, 0);

    // Cargamos datos de IP:PORT del server
    bzero((char*) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(10000);
    //serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if(inet_pton(AF_INET, "127.0.0.1", &(serveraddr.sin_addr))<=0)
    {
    	fprintf(stderr,"ERROR invalid server IP\r\n");
    	return NULL;
    }

    // Abrimos puerto con bind()
    if (bind(s, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1)
    {
        close(s);
        perror("listener: bind");
        return NULL;
    }

    // Seteamos socket en modo Listening
    if (listen (s, 10) == -1) // backlog=10
    {
        perror("error en listen");
        exit(1);
    }
    /* FIN INICIALIZACION TCP SERVER */

    printf("(Inicio TCP Server)\r\n");
    /* INICIO LOOP DE TCP SERVER */
    while(1)
    {
        // Ejecutamos accept() para recibir conexiones entrantes
        pthread_mutex_lock (&mutex_newfd);
        {
            addr_len = sizeof(struct sockaddr_in);
            if ( (newfd = accept(s, (struct sockaddr *)&clientaddr,&addr_len)) == -1)
            {
                perror("error en accept");
                exit(1);
            }
        }
        pthread_mutex_unlock (&mutex_newfd);

        char ipClient[32];
        inet_ntop(AF_INET, &(clientaddr.sin_addr), ipClient, sizeof(ipClient));
        printf  ("server:  conexion desde:  %s\n",ipClient);

        // Leemos mensaje de cliente
        //if( (n = recv(newfd,buffer,128,0)) == -1 )
        while (1)
	{
            if( (n = read(newfd,buffer,128)) == -1 )
            {
                perror("Error leyendo mensaje en socket");
                exit(1);
            }
            buffer[n]=0x00;
            printf("Recibi %d bytes.:%s\n",n,buffer);
		    
            if (n == 0) break; //Salgo del bucle de recepcion
			
            // Enviamos mensaje por Serial
            if (n>0)
	    {
                serial_send(buffer,n);
            }
        }
        // Cerramos conexion con cliente
    	close(newfd);
    	newfd = -1;
    } // fin while
    /* FIN LOOP DE TCP SERVER */

    exit(EXIT_SUCCESS);
    return NULL;
}

void tcp_close(void)
{
    close(newfd);
    close(s);
}
