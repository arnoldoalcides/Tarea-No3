#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/*threads library...*/
#include <pthread.h>
#include <assert.h>
#include <sys/time.h>


#define PORT        8081		/* port number as an integer */
#define IP_ADDRESS "127.0.0.1"	/* IP address as a string */

#define BUFSIZE 8196

pthread_mutex_t consola;

writeConsola(pthread_t idHilo, const char* msg)
{
	pthread_mutex_lock (&consola);

	printf("[%ld] %s\n",idHilo,msg);

	pthread_mutex_unlock (&consola);
}

//gettimeofday()

void* new_thread(void* nIteraciones)
{

	int ni = *((int*)nIteraciones); //castin the void* a int*, el otro * obtiene el valor de iteraciones.
	int i, sockfd;
	char buffer[BUFSIZE];
	struct sockaddr_in serv_addr;

	pthread_t idHilo = pthread_self();

	//fprintf(stderr,"[%ld] Starting with increment %d\n", idHilo, ni);

	while(ni--)
	{
		//writeConsola(idHilo, "client trying to connect");

		if((sockfd = socket(AF_INET, SOCK_STREAM,0)) <0) //parámetro es AF_INET indica si los clientes pueden estar en otras computadoras distintas del servidor o  en el mismo
			writeConsola(idHilo, "socket() failed");
		else
		{

			serv_addr.sin_family = AF_INET;
			serv_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
			serv_addr.sin_port = htons(PORT);

			if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) <0)
				writeConsola(idHilo, "connect() failed");
			else
			{

				//writeConsola(idHilo, "Getting index");

				// now the sockfd can be used to communicate to the server
				write(sockfd, "GET /index.html \r\n", 18);
				// note second space is a delimiter and important

				// this displays the raw HTML file as received by the browser
				while( (i=read(sockfd,buffer,BUFSIZE)) > 0)
				{
					buffer[i] = 0;
					writeConsola(idHilo,buffer);
				}

				//writeConsola(idHilo, "Closing socket!");
				close(sockfd);
			}
		}
	}
}


/* Que reciba dos parametros: numero de hilos (N) y numero de iteraciones (M) por hilo.*/
main(int argc, char **argv)
{
	int i,hilos = 1, iteraciones = 1;
	pthread_t* threadsArray; 	//Arreglo de hilos
	double 	start_time;
	struct timeval tempo;

	while ((i = getopt (argc, argv, "n:m:")) != -1)
		switch (i){
			case 'n': //Numero de Hilos
			    hilos = atoi(optarg);
				break;
			case 'm': //Numero de Iteraciones
				iteraciones = atoi(optarg);
				break;
			case '?':
				if (optopt == 'n' || optopt == 'm' )
					fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr,"Unknown option character `\\x%x'.\n", optopt);
				return 1;
			default:
				abort ();
		}

	pthread_mutex_init (&consola, NULL);

	//Creacion de hilos
	threadsArray = (pthread_t*) calloc (hilos, sizeof(pthread_t));
	assert(threadsArray != NULL);

	fprintf(stderr, "hilos %d  \n", hilos);
	fprintf(stderr, "iteraciones %d  \n", iteraciones);


	//obtener tiempo inicial
	gettimeofday(&tempo, NULL);
	start_time = (double)tempo.tv_usec;


	//poner a trabajar cada hilo que llama a la funcion new_thread
	for(i=0; i<hilos; i++)
	{
		if (pthread_create(&threadsArray[i], NULL, new_thread,(void*) &iteraciones))
		{
			   fprintf(stderr, "error creating a new thread \n");
			   exit(1);
		}
	}

	for(i=0; i<hilos; i++)
		pthread_join(threadsArray[i], NULL);

	fprintf(stderr, "Freeing resources\n");

	//liberar estructura de hilos.
	free(threadsArray);
	pthread_mutex_destroy(&consola);

	//obtener el tiempo final
	gettimeofday(&tempo, NULL);
	printf("Running for %.3f milliseconds\n", ((double)tempo.tv_usec - start_time)/1000);
}
