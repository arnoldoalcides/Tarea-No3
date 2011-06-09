
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <assert.h>

#define BUFSIZE 8096
#define ERROR 42
#define SORRY 43
#define LOG   44
#define DEFAULT_NTHREADS 10

#define MAX_NTHREADS 1000
#define MAX_PORT 60000

#define THREAD_SLEEP 1

//Threads Array
pthread_t* threadsArray;
int *descriptorsArray;
int *paramArray;

pthread_cond_t *condWaitsArray;
pthread_mutex_t *mutexArray;

struct {
	char *ext;
	char *filetype;
} extensions [] = {
	{"gif", "image/gif" },
	{"jpg", "image/jpeg"},
	{"jpeg","image/jpeg"},
	{"png", "image/png" },
	{"zip", "image/zip" },
	{"gz",  "image/gz"  },
	{"tar", "image/tar" },
	{"htm", "text/html" },
	{"html","text/html" },
	{0,0} };

void nwebLog(int type, char *s1, char *s2, int num)
{
	int fd ;
	char logbuffer[BUFSIZE*2];

	switch (type) {
		case ERROR:
			(void)sprintf(logbuffer,"ERROR: %s:%s Errno=%d exiting pid=%d",s1, s2, errno,getpid());
			break;
		case SORRY:
			(void)sprintf(logbuffer, "<HTML><BODY><H1>nweb Web Server Sorry: %s %s</H1></BODY></HTML>\r\n", s1, s2);
			(void)write(num,logbuffer,strlen(logbuffer));
			(void)sprintf(logbuffer,"SORRY: %s:%s",s1, s2);
			break;
		case LOG:
			(void)sprintf(logbuffer," INFO: %s:%s:%d",s1, s2,num);
			break;
	}
	/* no checks here, nothing can be done a failure anyway */
	if((fd = open("nweb.log", O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0) {
		(void)write(fd,logbuffer,strlen(logbuffer));
		(void)write(fd,"\n",1);
		(void)close(fd);
	}
	if(type == ERROR || type == SORRY) exit(3);
}

void* attendClient(void* parametro)
{
	//int *fd = (int*)parametro;
	int index = *((int*)parametro);

	int j, file_fd, buflen, len;
	long i, ret;
	char * fstr;
	char ok = 1;
	char buffer[BUFSIZE+1]; // static so zero filled

	//nwebLog(LOG,"Thread created","",0);

	while (1)
	{
		if (-1 == descriptorsArray[index])
		{
			pthread_mutex_lock (&mutexArray[index]);

			nwebLog(LOG,"Thread blocked","",index);
			//sleep(THREAD_SLEEP);

			pthread_cond_wait(&condWaitsArray[index],&mutexArray[index]);


			pthread_mutex_unlock (&mutexArray[index]);

		}
		else
		{
			nwebLog(LOG,"Thread attending client","",index);

			ok = 1;

			//read Web request in one go
			ret = read(descriptorsArray[index],buffer,BUFSIZE);

			if(ret == 0 || ret == -1)
			{
				nwebLog(LOG,"failed to read browser request","",index); // read failure stop now
				ok = 0;
			}

			if (ok)
			{
				// Return code is valid chars
				if(ret > 0 && ret < BUFSIZE)
					buffer[ret]=0;		//terminate the buffer
				else
					buffer[0]=0;

				for(i=0;i<ret;i++)	/* remove CF and LF characters */
					if(buffer[i] == '\r' || buffer[i] == '\n')
						buffer[i]='*';

				//nwebLog(LOG,"request",buffer,*fd);

				if( strncmp(buffer,"GET ",4) && strncmp(buffer,"get ",4) )
				{
					nwebLog(LOG,"Only simple GET operation supported",buffer,index);
					ok = 0;
				}
			}

			if (ok)
			{
				// null terminate after the second space to ignore extra stuff
				for(i=4;i<BUFSIZE;i++) {
					if(buffer[i] == ' ') { /* string is "GET URL " +lots of other stuff */
						buffer[i] = 0;
						break;
					}
				}

				// check for illegal parent directory use ..
				for(j=0;j<i-1;j++)
					if(buffer[j] == '.' && buffer[j+1] == '.')
					{
						nwebLog(LOG,"Parent directory (..) path names not supported",buffer,index);
						ok = 0;
						break;
					}
			}

			if (ok)
			{
				// convert no filename to index file
				if( !strncmp(&buffer[0],"GET /\0",6) || !strncmp(&buffer[0],"get /\0",6) )
					(void)strcpy(buffer,"GET /index.html");

				// work out the file type and check we support it
				buflen=strlen(buffer);
				fstr = (char *)0;
				for(i=0;extensions[i].ext != 0;i++) {
					len = strlen(extensions[i].ext);
					if( !strncmp(&buffer[buflen-len], extensions[i].ext, len)) {
						fstr =extensions[i].filetype;
						break;
					}
				}

				if(fstr == 0)
				{
					nwebLog(LOG,"file extension type not supported",buffer,index);
					ok = 0;
				}
			}

			if (ok)
			{
				// open the file for reading
				if(( file_fd = open(&buffer[5],O_RDONLY)) == -1)
				{
					nwebLog(LOG, "failed to open file",&buffer[5],index);
					ok = 0;
				}
			}

			if (ok)
			{
				//nwebLog(LOG,"SEND",&buffer[5],index);

				(void)sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
				(void)write(descriptorsArray[index],buffer,strlen(buffer));

				// send file in 8KB block - last block may be smaller
				while (	(ret = read(file_fd, buffer, BUFSIZE)) > 0 ) {
					(void)write(descriptorsArray[index],buffer,ret);
				}

				//to allow socket to drain
				sleep(1);
			}

			//Make the thread available again
			(void)close(descriptorsArray[index]);
			descriptorsArray[index] = -1;
		}
	}

	//nwebLog(LOG,"Thread ends","",0);
}

int main(int argc, char **argv)
{
	int i, pid, listenfd, socketfd, hit;
	size_t length;
	static struct sockaddr_in cli_addr; /* static = initialised to zeros */
	static struct sockaddr_in serv_addr; /* static = initialised to zeros */
	int port = -1;
	char *sPort;
	char *directory = NULL;
	int nThreads = DEFAULT_NTHREADS;
	int threadAvailable;

	if( argc == 2 && !strcmp(argv[1], "-?") ) {
			(void)printf("hint: nweb -p Port-Number -d Top-Directory -t Number-Threads\n\n"
		"\tnweb is a small and very safe mini web server\n"
		"\tnweb only servers out file/web pages with extensions named below\n"
		"\t and only from the named directory or its sub-directories.\n"
		"\tThere is no fancy features = safe and secure.\n\n"
		"\tExample: nweb 8181 /home/nwebdir &\n\n"
		"\tOnly Supports:");
			for(i=0;extensions[i].ext != 0;i++)
				(void)printf(" %s",extensions[i].ext);

			(void)printf("\n\tNot Supported: URLs including \"..\", Java, Javascript, CGI\n"
		"\tNot Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin \n"
		"\tNo warranty given or implied\n\tNigel Griffiths nag@uk.ibm.com\n"
			    );
			exit(0);
	}

	while ((i = getopt (argc, argv, "p:d:t:")) != -1)
		switch (i)
		{
			case 'p': //Port in optarg

				port = atoi(optarg);

				if(port < 1 || port > MAX_PORT)
					nwebLog(ERROR,"Invalid port number",optarg,0);

				sPort = optarg; //Save the string representation of port

				break;

			case 'd': //Directory in optarg

				directory = optarg;

				if( !strncmp(directory,"/"   ,2 ) || !strncmp(directory,"/etc", 5 ) ||
					!strncmp(directory,"/bin",5 ) || !strncmp(directory,"/lib", 5 ) ||
					!strncmp(directory,"/tmp",5 ) || !strncmp(directory,"/usr", 5 ) ||
					!strncmp(directory,"/dev",5 ) || !strncmp(directory,"/sbin",6) ){
					(void)printf("ERROR: Bad top directory %s, see nweb -?\n",directory);
					exit(3);
				}

				if(chdir(directory) == -1){
					(void)printf("ERROR: Can't Change to directory %s\n",directory);
					exit(4);
				}

				break;
			case 't': //Number of threads to create
				nThreads = atoi(optarg);

				if(nThreads < 1 || nThreads> MAX_NTHREADS )
					nwebLog(ERROR,"Invalid number of threads",optarg,0);

				break;
			case '?':
				if (optopt == 'p' || optopt == 'd' || optopt == 't')
					fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr,"Unknown option character `\\x%x'.\n", optopt);
				return 1;
			default:
				abort ();
		}

	if(port < 1)
		nwebLog(ERROR,"Port not defined","",0);

	if(NULL == directory)
		nwebLog(ERROR,"Directory not defined","",0);

	/* Become deamon + unstopable and no zombies children (= no wait()) */
	if(fork() != 0)
		return 0; /* parent returns OK to shell */

	(void)signal(SIGCLD, SIG_IGN); /* ignore child death */
	(void)signal(SIGHUP, SIG_IGN); /* ignore terminal hangups */

	//close open files
	for(i=0;i<32;i++)
		(void)close(i);

	// break away from process group
	(void)setpgrp();

	nwebLog(LOG,"nweb starting",sPort,getpid());

	//Creation of threads array
	threadsArray = (pthread_t*) calloc (nThreads, sizeof(pthread_t));
	assert(threadsArray != NULL);

	//Creation of paramArray
	paramArray = (int*)calloc (nThreads, sizeof(int));
	assert(paramArray != NULL);

	//Creation of conditional waits
	condWaitsArray = (pthread_cond_t*) calloc (nThreads, sizeof(pthread_cond_t));
	assert(condWaitsArray != NULL);

	//Creation of mutexes
	mutexArray = (pthread_mutex_t*) calloc (nThreads, sizeof(pthread_mutex_t));
	assert(mutexArray != NULL);

	//Creation of descriptors array
	descriptorsArray = (int*)calloc (nThreads, sizeof(int));
	assert(descriptorsArray != NULL);

	//Initialization
	for(i = 0; i < nThreads; i++)
	{
		descriptorsArray[i] = -1;
		paramArray[i] = i; //index is passed on

		pthread_cond_init(&condWaitsArray[i],NULL);
		pthread_mutex_init(&mutexArray[i],NULL);

		if (pthread_create( &threadsArray[i], NULL, &attendClient, &paramArray[i] ) != 0)
			nwebLog(ERROR,"system call","pthread_create",0);
	}

	//setup the network socket
	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0)
		nwebLog(ERROR, "system call","socket",0);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0)
		nwebLog(ERROR,"system call","bind",0);

	if( listen(listenfd,64) <0)
		nwebLog(ERROR,"system call","listen",0);

	//
	for(hit=1; ;hit++) {

		length = sizeof(cli_addr);

		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
			nwebLog(ERROR,"system call","accept",0);

		threadAvailable = -1;
		//Find a thread available
		for(i = 0; i < nThreads; i++)
		{
			if (-1 == descriptorsArray[i])
			{
				threadAvailable = i;
				break;
			}
		}

		if (-1 == threadAvailable)
		{
			nwebLog(LOG,"NO thread AVAILABLE","",0);
			(void)close(socketfd);
		}
		else
		{
			nwebLog(LOG,"Thread available","",threadAvailable);
			descriptorsArray[threadAvailable] = socketfd;

			pthread_cond_signal(&condWaitsArray[threadAvailable]);
		}

		//sleep(1);

		/*if((pid = fork()) < 0) {
			nwebLog(ERROR,"system call","fork",0);
		}
		else {
			if(pid == 0) { 	// child
				(void)close(listenfd);
				web(socketfd,hit); // never returns
			} else { 	// parent
				(void)close(socketfd);
			}
		}*/
	}
}
