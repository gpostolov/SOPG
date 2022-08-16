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
#include <stdbool.h>
#include <signal.h>

#include "SerialManager.h"
#include "tcp.h"

void sig_handler(int sig);

static pthread_t tcp_thread;

int newfd = -1;
pthread_mutex_t mutex_newfd = PTHREAD_MUTEX_INITIALIZER;

char buffer[128];
int n;

bool cierre_forzado = false;

void bloquearSign(void)
{
    sigset_t set;
    int s;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
}

void desbloquearSign(void)
{
    sigset_t set;
    int s;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);
}

int main (void)
{
    printf("Bloqueo signal\n");
    bloquearSign();
	
    printf("Inicio Serial Service\r\n");

    /* SE LANZAN LOS THREADS */
    const char *message1 = "TCP Server";
    pthread_create (&tcp_thread, NULL, tcp_main, (void *) message1);
    /*                       */
	
    printf("Desbloqueo signal\n");
    desbloquearSign();

    /* INICIO INICIALIZACION SERIAL MANAGER */
    if (serial_open(4040,115200) == -1)
    {
        fprintf(stderr,"ERROR\r\n");
    }
    /* FIN INICIALIZACION SERIAL MANAGER    */
	
    /* CAPTURA DE SENALES */
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
    /*                     */

    /* INICIO LOOP DE SERIAL MANAGER */
    printf("(Inicio UART)\r\n");
    while(1)
    {
        if (cierre_forzado == true) break;
        if( (n = serial_receive(buffer,128)) > 0 )
	{
            buffer[n]=0x00;
            printf("Recibi %d bytes.:%s\n",n,buffer);
            //si lei algo, lo envio por socket
            pthread_mutex_lock (&mutex_newfd);
            {
                if (newfd != -1)
		{
                    if (write (newfd, buffer, n) == -1) 
		    {
                        perror("Error escribiendo mensaje en socket. Skipping.");
                    }
                }
            }            
            pthread_mutex_unlock (&mutex_newfd);
        }
        usleep(10000);
    }
    /* FIN LOOP DE SERIAL MANAGER    */

    /* CIERRE ORDENADO DE LOS RECURSOS */
    serial_close(); //SerialManager;
    tcp_close();    //tcp
    pthread_cancel(tcp_thread);
    pthread_join(tcp_thread, NULL);
    exit(1);	
    /*                                 */
}

void sig_handler(int sig)
{
    if ( (sig == SIGINT) ||(sig == SIGTERM) )
    {
        cierre_forzado = true;
    }
}
