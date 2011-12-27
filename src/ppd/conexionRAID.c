#include <sys/socket.h>
#include <netdb.h>
#include "ppd.h"
#include "conexionRAID.h"

uint32_t sockfd, port;
struct sockaddr_in raid_addr;
struct hostent *server;

void conectarConRAID(config_t vecConfig)
{
  port = atoi((const char *)vecConfig.puertopraid);

  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) 
    error("ERROR opening socket");

  server = gethostbyname(vecConfig.ippraid);

  if (server == NULL) 
  {
    fprintf(stderr,"ERROR, no such host\n");
    exit(0);
  }

  bzero((char *) &raid_addr, sizeof(raid_addr));
  raid_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr,(char *)&raid_addr.sin_addr.s_addr,server->h_length);
  raid_addr.sin_port = htons(port);

  if (connect(sockfd,(struct sockaddr *) &raid_addr,sizeof(raid_addr)) < 0) 
    error("ERROR connecting");
       
  close(sockfd);
}