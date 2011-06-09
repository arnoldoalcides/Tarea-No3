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



pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;







#define BUFSIZE 8096

#define ERROR 42

#define SORRY 43

#define LOG   44



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
						
						
						
						extern char *optarg;
						
						
						
						/*global variables*/
						
						int gfd; 
						
						int ghit;
						
						
						
						
						
						void xLog(int type, char *s1, char *s2, int num)
						
						{
						
						#if 1
						
						    int fd ;
						    
							char logbuffer[BUFSIZE*2];
							
							
							
							    switch (type) {
							    
								case ERROR: (void)sprintf(logbuffer, "ERROR: %s:%s Errno=%d exiting pid=%d",s1, s2, errno,getpid()); break;
								
								    case SORRY: 
								    
									    (void)sprintf(logbuffer, "<HTML><BODY><H1>nweb Web Server Sorry: %s %s</H1></BODY></HTML>\r\n", s1, s2);
									    
										    (void)write(num,logbuffer,strlen(logbuffer));
										    
											    (void)sprintf(logbuffer, "SORRY: %s:%s",s1, s2); 
											    
												    break;
												    
													case LOG: (void)sprintf(logbuffer, " INFO: %s:%s:%d",s1, s2,num); break;
													
													    }	
													    
														/* no checks here, nothing can be done a failure anyway */
														
														    if((fd = open("nweb.log", O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0) {
														    
															    (void)write(fd,logbuffer,strlen(logbuffer)); 
															    
																    (void)write(fd,"\n",1);      
																    
																	    (void)close(fd);
																	    
																		}
																		
																		    if(type == ERROR || type == SORRY) exit(3);
																		    
																		    #endif
																		    
																		    }
																		    
																		    
																		    
																		    
																		    
																		    /* this is a child web server process, so we can exit on errors */
																		    
																		    void web(int fd, int hit )
																		    
																		    {
																		    
																			
																			
																			    int j, file_fd, buflen, len;
																			    
																				long i, ret;
																				
																				    char * fstr;
																				    
																					static char buffer[BUFSIZE+1]; /* static so zero filled */
																					
																					
																					
																					    ret =read(fd,buffer,BUFSIZE); 	/* read Web request in one go */
																					    
																						if(ret == 0 || ret == -1) {	/* read failure stop now */
																						
																							xLog(SORRY,"failed to read browser request","",fd);
																							
																							    }
																							    
																								if(ret > 0 && ret < BUFSIZE)	/* return code is valid chars */
																								
																									buffer[ret]=0;		/* terminate the buffer */
																									
																									    else buffer[0]=0;
																									    
																									    
																									    
																										for(i=0;i<ret;i++)	/* remove CF and LF characters */
																										
																											if(buffer[i] == '\r' || buffer[i] == '\n')
																											
																												    buffer[i]='*';
																												    
																													xLog(LOG,"request",buffer,hit);
																													
																													
																													
																													    if( strncmp(buffer,"GET ",4) && strncmp(buffer,"get ",4) )
																													    
																														    xLog(SORRY,"Only simple GET operation supported",buffer,fd);
																														    
																														    
																														    
																															for(i=4;i<BUFSIZE;i++) { /* null terminate after the second space to ignore extra stuff */
																															
																																if(buffer[i] == ' ') { /* string is "GET URL " +lots of other stuff */
																																
																																	    buffer[i] = 0;
																																	    
																																			break;
																																			
																																				}
																																				
																																				    }
																																				    
																																				    
																																				    
																																					for(j=0;j<i-1;j++) 	/* check for illegal parent directory use .. */
																																					
																																						if(buffer[j] == '.' && buffer[j+1] == '.')
																																						
																																							    xLog(SORRY,"Parent directory (..) path names not supported",buffer,fd);
																																							    
																																							    
																																							    
																																								if( !strncmp(&buffer[0],"GET /\0",6) || !strncmp(&buffer[0],"get /\0",6) ) /* convert no filename to index file */
																																								
																																									(void)strcpy(buffer,"GET /index.html");
																																									
																																									
																																									
																																									    /* work out the file type and check we support it */
																																									    
																																										buflen=strlen(buffer);
																																										
																																										    fstr = (char *)0;
																																										    
																																											for(i=0;extensions[i].ext != 0;i++) {
																																											
																																												len = strlen(extensions[i].ext);
																																												
																																													if( !strncmp(&buffer[buflen-len], extensions[i].ext, len)) {
																																													
																																														    fstr =extensions[i].filetype;
																																														    
																																																break;
																																																
																																																	}
																																																	
																																																	    }
																																																	    
																																																		if(fstr == 0) xLog(SORRY,"file extension type not supported",buffer,fd);
																																																		
																																																		
																																																		
																																																		    if(( file_fd = open(&buffer[5],O_RDONLY)) == -1) /* open the file for reading */
																																																		    
																																																			    xLog(SORRY, "failed to open file",&buffer[5],fd);
																																																			    
																																																			    
																																																			    
																																																				xLog(LOG,"SEND",&buffer[5],hit);
																																																				
																																																				
																																																				
																																																				    (void)sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
																																																				    
																																																					(void)write(fd,buffer,strlen(buffer));
																																																					
																																																					
																																																					
																																																					    /* send file in 8KB block - last block may be smaller */
																																																					    
																																																						while (	(ret = read(file_fd, buffer, BUFSIZE)) > 0 ) {
																																																						
																																																							(void)write(fd,buffer,ret);
																																																							
																																																							    }
																																																							    
																																																							    #ifdef LINUX
																																																							    
																																																								sleep(1);	/* to allow socket to drain */
																																																								
																																																								#endif
																																																								
																																																								    (void)close(fd);
																																																								    
																																																								    
																																																								    
																																																								    }
																																																								    
																																																								    
																																																								    
																																																								    void* xRun(){
																																																								    
																																																									int fd;
																																																									
																																																									    int hit;
																																																									    
																																																									    
																																																									    
																																																										while(1){
																																																										
																																																										    
																																																										    
																																																											    pthread_mutex_lock(&lock);
																																																											    
																																																												    pthread_cond_wait(&cond, &lock);
																																																												    
																																																													
																																																													
																																																														fd=gfd;
																																																														
																																																															hit=ghit;
																																																															
																																																															    
																																																															    
																																																																    pthread_mutex_unlock(&lock);
																																																																    
																																																																	
																																																																	
																																																																		web(fd, hit);
																																																																		
																																																																			//(void)close(fd);
																																																																			
																																																																			    }
																																																																			    
																																																																			    }
																																																																			    
																																																																			    
																																																																			    
																																																																			    void usage(){
																																																																			    
																																																																				int i;
																																																																				
																																																																				    (void)printf("hint: nweb Port-Number Top-Directory\n\n"
																																																																				    
																																																																					    "\tnweb is a small and very safe mini web server\n"
																																																																					    
																																																																						    "\tnweb only servers out file/web pages with extensions named below\n"
																																																																						    
																																																																							    "\t and only from the named directory or its sub-directories.\n"
																																																																							    
																																																																								    "\tThere is no fancy features = safe and secure.\n\n"
																																																																								    
																																																																									    "\tExample: nweb -p 8181 -d /home/nwebdir -t 5 &\n\n"
																																																																									    
																																																																										    "\tOnly Supports:");
																																																																										    
																																																																												for(i=0;extensions[i].ext != 0;i++)
																																																																												
																																																																														(void)printf(" %s",extensions[i].ext);
																																																																														
																																																																														
																																																																														
																																																																															    (void)printf("\n\tNot Supported: URLs including \"..\", Java, Javascript, CGI\n"
																																																																															    
																																																																																    "\tNot Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin \n"
																																																																																    
																																																																																	    "\tNo warranty given or implied\n\tNigel Griffiths nag@uk.ibm.com\n"
																																																																																	    
																																																																																			    );
																																																																																			    
																																																																																			    }
																																																																																			    
																																																																																			    
																																																																																			    
																																																																																			    
																																																																																			    
																																																																																			    int main(int argc, char **argv)
																																																																																			    
																																																																																			    {
																																																																																			    
																																																																																				int num_t;
																																																																																				
																																																																																				    int i, port, pid, listenfd, socketfd, hit, c;
																																																																																				    
																																																																																					char* dir;
																																																																																					
																																																																																					    char* xport;
																																																																																					    
																																																																																						int flagt=0, flagp=0, flagd=0;
																																																																																						
																																																																																						    size_t length;
																																																																																						    
																																																																																							static struct sockaddr_in cli_addr; /* static = initialised to zeros */
																																																																																							
																																																																																							    static struct sockaddr_in serv_addr; /* static = initialised to zeros */
																																																																																							    
																																																																																							    
																																																																																							    
																																																																																							    
																																																																																							    
																																																																																								while ((c = getopt (argc, argv, "d:p:t:")) != -1){
																																																																																								
																																																																																									switch(c){
																																																																																									
																																																																																										    case 't':
																																																																																										    
																																																																																											        	num_t = atoi(optarg);
																																																																																											        	
																																																																																											        			    flagt=1;
																																																																																											        			    
																																																																																											        				        	break;
																																																																																											        				        	
																																																																																											        				        			case 'p':
																																																																																											        				        			
																																																																																											        				        					    xport = optarg;
																																																																																											        				        					    
																																																																																											        				        						        	port = atoi(optarg);
																																																																																											        				        						        	
																																																																																											        				        						        			    flagp=1;
																																																																																											        				        						        			    
																																																																																											        				        						        					    break;
																																																																																											        				        						        					    
																																																																																											        				        						        						    	case 'd':
																																																																																											        				        						        						    	
																																																																																											        				        						        						    		    	dir = optarg;
																																																																																											        				        						        						    		    	
																																																																																											        				        						        						    		    			    flagd=1;
																																																																																											        				        						        						    		    			    
																																																																																											        				        						        						    		    						break;
																																																																																											        				        						        						    		    						
																																																																																											        				        						        						    		    							    case '?':
																																																																																											        				        						        						    		    							    
																																																																																											        				        						        						    		    									default:
																																																																																											        				        						        						    		    									
																																																																																											        				        						        						    		    											usage();
																																																																																											        				        						        						    		    											
																																																																																											        				        						        						    		    													exit(0);
																																																																																											        				        						        						    		    													
																																																																																											        				        						        						    		    														}
																																																																																											        				        						        						    		    														
																																																																																											        				        						        						    		    														    }
																																																																																											        				        						        						    		    														    
																																																																																											        				        						        						    		    														    
																																																																																											        				        						        						    		    														    
																																																																																											        				        						        						    		    														    
																																																																																											        				        						        						    		    														    
																																																																																											        				        						        						    		    															if(!flagt || !flagp || !flagd){
																																																																																											        				        						        						    		    															
																																																																																											        				        						        						    		    																usage();
																																																																																											        				        						        						    		    																
																																																																																											        				        						        						    		    																	exit(0);
																																																																																											        				        						        						    		    																	
																																																																																											        				        						        						    		    																	    }
																																																																																											        				        						        						    		    																	    
																																																																																											        				        						        						    		    																	    
																																																																																											        				        						        						    		    																	    
																																																																																											        				        						        						    		    																	    
																																																																																											        				        						        						    		    																	    
																																																																																											        				        						        						    		    																		if( !strncmp(dir,"/"   ,2 ) || !strncmp(dir,"/etc", 5 ) ||
																																																																																											        				        						        						    		    																		
																																																																																											        				        						        						    		    																		        !strncmp(dir,"/bin",5 ) || !strncmp(dir,"/lib", 5 ) ||
																																																																																											        				        						        						    		    																		        
																																																																																											        				        						        						    		    																		    	    !strncmp(dir,"/tmp",5 ) || !strncmp(dir,"/usr", 5 ) ||
																																																																																											        				        						        						    		    																		    	    
																																																																																											        				        						        						    		    																		    		    !strncmp(dir,"/dev",5 ) || !strncmp(dir,"/sbin",6) ){
																																																																																											        				        						        						    		    																		    		    
																																																																																											        				        						        						    		    																		    			    (void)printf("ERROR: Bad top directory %s, see nweb -?\n",dir);
																																																																																											        				        						        						    		    																		    			    
																																																																																											        				        						        						    		    																		    				    exit(3);
																																																																																											        				        						        						    		    																		    				    
																																																																																											        				        						        						    		    																		    					}
																																																																																											        				        						        						    		    																		    					
																																																																																											        				        						        						    		    																		    					    if(chdir(dir) == -1){ 
																																																																																											        				        						        						    		    																		    					    
																																																																																											        				        						        						    		    																		    						    (void)printf("ERROR: Can't Change to directory %s\n",dir);
																																																																																											        				        						        						    		    																		    						    
																																																																																											        				        						        						    		    																		    							    exit(4);
																																																																																											        				        						        						    		    																		    							    
																																																																																											        				        						        						    		    																		    								}
																																																																																											        				        						        						    		    																		    								
																																																																																											        				        						        						    		    																		    								    
																																																																																											        				        						        						    		    																		    								    
																																																																																											        				        						        						    		    																		    									//ipthreads begin
																																																																																											        				        						        						    		    																		    									
																																																																																											        				        						        						    		    																		    									    pthread_t threads[num_t];
																																																																																											        				        						        						    		    																		    									    
																																																																																											        				        						        						    		    																		    										int rc;
																																																																																											        				        						        						    		    																		    										
																																																																																											        				        						        						    		    																		    										    for (i=0; i<num_t; i++){
																																																																																											        				        						        						    		    																		    										    
																																																																																											        				        						        						    		    																		    											    rc = pthread_create(&threads[i], NULL, xRun, NULL);
																																																																																											        				        						        						    		    																		    											    
																																																																																											        				        						        						    		    																		    												    if (rc){
																																																																																											        				        						        						    		    																		    												    
																																																																																											        				        						        						    		    																		    														printf("ERROR; return code from pthread_create() is %d\n", rc);
																																																																																											        				        						        						    		    																		    														
																																																																																											        				        						        						    		    																		    														            exit(-1);
																																																																																											        				        						        						    		    																		    														            
																																																																																											        				        						        						    		    																		    														                    }
																																																																																											        				        						        						    		    																		    														                    
																																																																																											        				        						        						    		    																		    														                	}
																																																																																											        				        						        						    		    																		    														                	
																																																																																											        				        						        						    		    																		    														                	
																																																																																											        				        						        						    		    																		    														                	
																																																																																											        				        						        						    		    																		    														                	    /* Become deamon + unstopable and no zombies children (= no wait()) */
																																																																																											        				        						        						    		    																		    														                	    
																																																																																											        				        						        						    		    																		    														                		//if(fork() != 0)
																																																																																											        				        						        						    		    																		    														                		
																																																																																											        				        						        						    		    																		    														                			//return 0; // parent returns OK to shell
																																																																																											        				        						        						    		    																		    														                			
																																																																																											        				        						        						    		    																		    														                			    
																																																																																											        				        						        						    		    																		    														                			    
																																																																																											        				        						        						    		    																		    														                				(void)signal(SIGCLD, SIG_IGN); /* ignore child death */
																																																																																											        				        						        						    		    																		    														                				
																																																																																											        				        						        						    		    																		    														                				    (void)signal(SIGHUP, SIG_IGN); /* ignore terminal hangups */
																																																																																											        				        						        						    		    																		    														                				    
																																																																																											        				        						        						    		    																		    														                					
																																																																																											        				        						        						    		    																		    														                					
																																																																																											        				        						        						    		    																		    														                					    for(i=0;i<32;i++){
																																																																																											        				        						        						    		    																		    														                					    
																																																																																											        				        						        						    		    																		    														                						    (void)close(i); /* close open files */
																																																																																											        				        						        						    		    																		    														                						    
																																																																																											        				        						        						    		    																		    														                							}
																																																																																											        				        						        						    		    																		    														                							
																																																																																											        				        						        						    		    																		    														                							    
																																																																																											        				        						        						    		    																		    														                							    
																																																																																											        				        						        						    		    																		    														                								(void)setpgrp();		/* break away from process group */
																																																																																											        				        						        						    		    																		    														                								
																																																																																											        				        						        						    		    																		    														                								    xLog(LOG,"nweb starting",xport,getpid());
																																																																																											        				        						        						    		    																		    														                								    
																																																																																											        				        						        						    		    																		    														                									/* setup the network socket */
																																																																																											        				        						        						    		    																		    														                									
																																																																																											        				        						        						    		    																		    														                									    if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0)
																																																																																											        				        						        						    		    																		    														                									    
																																																																																											        				        						        						    		    																		    														                										    xLog(ERROR, "system call","socket",0);
																																																																																											        				        						        						    		    																		    														                										    
																																																																																											        				        						        						    		    																		    														                											if(port < 0 || port >60000)
																																																																																											        				        						        						    		    																		    														                											
																																																																																											        				        						        						    		    																		    														                												xLog(ERROR,"Invalid port number (try 1->60000)",xport,0);
																																																																																											        				        						        						    		    																		    														                												
																																																																																											        				        						        						    		    																		    														                												    serv_addr.sin_family = AF_INET;
																																																																																											        				        						        						    		    																		    														                												    
																																																																																											        				        						        						    		    																		    														                													serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
																																																																																											        				        						        						    		    																		    														                													
																																																																																											        				        						        						    		    																		    														                													    serv_addr.sin_port = htons(port);
																																																																																											        				        						        						    		    																		    														                													    
																																																																																											        				        						        						    		    																		    														                														if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0)
																																																																																											        				        						        						    		    																		    														                														
																																																																																											        				        						        						    		    																		    														                															xLog(ERROR,"system call","bind",listenfd);
																																																																																											        				        						        						    		    																		    														                															
																																																																																											        				        						        						    		    																		    														                															    if( listen(listenfd,64) <0)
																																																																																											        				        						        						    		    																		    														                															    
																																																																																											        				        						        						    		    																		    														                																    xLog(ERROR,"system call","listen",listenfd);
																																																																																											        				        						        						    		    																		    														                																    
																																																																																											        				        						        						    		    																		    														                																    
																																																																																											        				        						        						    		    																		    														                																    
																																																																																											        				        						        						    		    																		    														                																	for(hit=1; ;hit++) {
																																																																																											        				        						        						    		    																		    														                																	
																																																																																											        				        						        						    		    																		    														                																		length = sizeof(cli_addr);
																																																																																											        				        						        						    		    																		    														                																		
																																																																																											        				        						        						    		    																		    														                																	if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
																																																																																											        				        						        						    		    																		    														                																			
																																																																																											        				        						        						    		    																		    														                																				    xLog(ERROR,"system call","accept",0);
																																																																																											        				        						        						    		    																		    														                																				    
																																																																																											        				        						        						    		    																		    														                																					    
																																																																																											        				        						        						    		    																		    														                																					    
																																																																																											        				        						        						    		    																		    														                																						    pthread_mutex_lock(&lock);
																																																																																											        				        						        						    		    																		    														                																						    
																																																																																											        				        						        						    		    																		    														                																							    
																																																																																											        				        						        						    		    																		    														                																							    
																																																																																											        				        						        						    		    																		    														                																								    gfd=socketfd;
																																																																																											        				        						        						    		    																		    														                																								    
																																																																																											        				        						        						    		    																		    														                																									    ghit=hit;
																																																																																											        				        						        						    		    																		    														                																									    
																																																																																											        				        						        						    		    																		    														                																										    
																																																																																											        				        						        						    		    																		    														                																										    
																																																																																											        				        						        						    		    																		    														                																											    pthread_cond_signal(&cond);
																																																																																											        				        						        						    		    																		    														                																											    
																																																																																											        				        						        						    		    																		    														                																												    pthread_mutex_unlock(&lock);
																																																																																											        				        						        						    		    																		    														                																												    
																																																																																											        				        						        						    		    																		    														                																												    
																																																																																											        				        						        						    		    																		    														                																												    
																																																																																											        				        						        						    		    																		    														                																													}
																																																																																											        				        						        						    		    																		    														                																													
																																																																																											        				        						        						    		    																		    														                																													    
																																																																																											        				        						        						    		    																		    														                																													    
																																																																																											        				        						        						    		    																		    														                																														for (i=0; i<num_t; i++){
																																																																																											        				        						        						    		    																		    														                																														
																																																																																											        				        						        						    		    																		    														                																															pthread_join(&threads[i], NULL);
																																																																																											        				        						        						    		    																		    														                																															
																																																																																											        				        						        						    		    																		    														                																															    }
																																																																																											        				        						        						    		    																		    														                																															    
																																																																																											        				        						        						    		    																		    														                																																
																																																																																											        				        						        						    		    																		    														                																																
																																																																																											        				        						        						    		    																		    														                																																}
																																																																																											        				        						        						    		    																		    														                																																
																																																																																											        				        						        						    		    																		    														                																																
																																																																																											        				        						        						    		    																		    														                																																
																																																																																											        				        						        						    		    																		    														                																																