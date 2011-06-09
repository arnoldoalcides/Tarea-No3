#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT        8181		/* port number as an integer */
#define IP_ADDRESS "127.0.0.1"	/* IP address as a string */

#define BUFSIZE 8196

extern char *optarg;

pexit(char * msg)
{
    perror(msg);
	exit(1);
	}
	
	void xConnect(void* iteraciones){
	    int iter;
		int i;
		    int timestamp;
			iter = (int)iteraciones;
			    int sockfd;
				char buffer[BUFSIZE];
				    static struct sockaddr_in serv_addr;
				    
					printf("client trying to connect to %s and port %d\n",IP_ADDRESS,PORT);
					    if((sockfd = socket(AF_INET, SOCK_STREAM,0)) <0) 
						    printf("socket() failed");
						    
							serv_addr.sin_family = AF_INET;
							    serv_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
								serv_addr.sin_port = htons(PORT);
								
								    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) <0)
									    printf("connect() failed");
									    
										
										    /* now the sockfd can be used to communicate to the server */
											for(i=0; i<iter; i++){
												 // numero de llamados que el usuario solicita
													timestamp=gettimeofday();
														write(sockfd, "GET /index.html \r\n", 18);
															/* note second space is a delimiter and important */
															
																/* this displays the raw HTML file as received by the browser */
																	while( (i=read(sockfd,buffer,BUFSIZE)) > 0){
																		    write(1,buffer,i);
																			    }
																					
																						printf("Request %d: tomo %d ms", i, gettimeofday()-timestamp);
																						    }
																							    
																								
																								}
																								
																								void usage(){
																								    int i;
																									printf("usage: client -h [#hilos] -i [#iteraciones]\n");
																									}
																									
																									int main(int argc, char **argv)
																									{
																									    int c,i;
																										int hilos, iter;
																										    int flagh=0, flagi=0;
																											while ((c = getopt (argc, argv, "i:h:")) != -1){
																												switch(c){
																													    case 'h':
																														    	hilos = atoi(optarg);
																														    			flagh=1;
																														    					break;
																														    						    case 'i':
																														    							    	iter = atoi(optarg);
																														    							    			flagi=1;
																														    							    				    break;
																														    							    						case '?':
																														    							    							    default:
																														    							    									    usage();
																														    							    											    exit(0);
																														    							    												    }
																														    							    													}
																														    							    													
																														    							    													
																														    							    													    if(!flagh || !flagi ){
																														    							    														    usage();
																														    							    															    exit(1);
																														    							    																}
																														    							    																    
																														    							    																	pthread_t threads[hilos];
																														    							    																	    int rc;
																														    							    																		for (i=0; i<hilos; i++){
																														    							    																			rc = pthread_create(&threads[i], NULL, xConnect, (void*)iter);
																														    							    																				if (rc){
																														    							    																					    printf("ERROR; return code from pthread_create() is %d\n", rc);
																														    							    																					                exit(-1);
																														    							    																					                        }
																														    							    																					                    	}
																														    							    																					                    	    for (i=0; i<hilos; i++){
																														    							    																					                    		    pthread_join(&threads[i], NULL);
																														    							    																					                    			}
																														    							    																					                    				
																														    							    																					                    				}
																														    							    																					                    				