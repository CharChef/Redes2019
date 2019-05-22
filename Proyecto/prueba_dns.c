#include <stdio.h>
#include <netdb.h>   /* gethostbyname() necesita esta cabecera */
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

  
int main(int argc, char *argv[])
{ 

   struct hostent *he;

   if (argc!=2) {
      printf("Uso: %s &lt;hostname&gt;\n",argv[0]);
      exit(-1);
   }

   if ((he=gethostbyname(argv[1]))==NULL) {
      printf("error de gethostbyname()\n");
      exit(-1);
   }

   printf("Nombre del host: %s\n",he->h_name);  
      /* muestra el nombre del nodo */
   printf("Dirección IP: %s\n",
          inet_ntoa(*((struct in_addr *)he->h_addr)));
      /* muestra la dirección IP */

}