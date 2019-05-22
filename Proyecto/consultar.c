#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <resolv.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const int SOCK_OPEN=4;
const int SOCK_CONN=5;
const int SOCK_SEND=6;
const int SOCK_RECV=7;



int debug = 1;

int conectar_sin_bloquear(int sockfd, struct sockaddr_in *host, socklen_t len , int sec);

/**
 * Funcion que retorna una subcadena de caracteres de tamaño n, comenzando desde la posicion beg, de 
 * la string str
 * Parametros:
 * char* str - Cadena de caracteres original
 * int beg - Comienzo de subcadena
 * int n - Longitud de subcadena
 * */
char* substring(const char* str, int beg, int n)
{
   char *sub = malloc(n+1); 
   strncpy(sub, (str + beg), n);
   *(sub+n) = 0;

   return sub;
}

/*
struct sockaddr_in 
{ 
   short int sin_family;        // Familia de la Dirección               
   unsigned short int sin_port; // Puerto                               
   struct in_addr sin_addr;     // Dirección de Internet                
   unsigned char sin_zero[8];   // Del mismo tamaño que struct sockaddr  
}; 
*/

/**
 * Funcion que se encarga de conectar mediante sockets no bloqueantes a un servidor dns.
 *
 * Parametros:
 * char* ip_dns
 * */
int consultar(char* ip_dns)
{
	printf("\n\n");	 
	int sockfd;    // Identificador del socket
	int connid;    // Identificador de la conexion
	int puerto = 53;  //-----------------------------------------------Puerto Estandar DNS

	struct sockaddr_in conect; // Estructura para guardar datos de conexion.
 	
	// Datos en la estructura:
	conect.sin_family = AF_INET;


	struct in_addr **addr_list;
	struct hostent *host;

	if(debug)printf("Llego hasta aca\n");

	host = gethostbyname("google-public-dns-a.google.com");		//-----------------------------------Servidor DNS por Defecto
	addr_list = (struct in_addr **)host->h_addr_list;			
	char * ip_Host=(char*)inet_ntoa(*addr_list[0]);

	if(debug)printf("Obtengo el IP\n");

	conect.sin_addr.s_addr = inet_addr(ip_Host);	
	if(debug) printf("pase 1.1\n");
	bzero(&(conect.sin_zero), 8);

	sockfd = socket(AF_INET,SOCK_STREAM,0); 
	conect.sin_port = htons(puerto);
	
	// Conexión al socket
	//onnid = connect (sockid, (struct sockaddr *)&conect, sizeof(struct sockaddr));
	// Me conecto
	if (connid = connect(sockfd, (struct sockaddr *)&conect, sizeof(struct sockaddr))<0){
		perror("SOCKET: Error al querer establecer conexion con el host");
		exit(SOCK_CONN);	
	}
	//connid = conectar_sin_bloquear(sockid, (struct sockaddr_in *)&conect, sizeof(struct sockaddr),1);

	if(debug)printf("Me conecte\n");
	
	//------------------------------------------------------------------------------------
	// Buffer para almacenar los datos que recibo
	// Buffer para almacenar los datos recibidos
	char buf[2056];
    int byte_count;
    
    // Mensaje a enviar
	char *mensaje = "A www.cs.uns.edu.ar OPT";
	
	// Envio mensaje: GET
	if (send(sockfd,mensaje,strlen(mensaje),0)<0){
		perror("SOCKET: Error al querer enviar datos");
		exit(SOCK_SEND);
	}
	// Recibo mensaje
	if ((byte_count = recv(sockfd,buf,sizeof(buf)-1,0))<0){
		perror("SOCKET: Error al querer recibir datos");
		exit(SOCK_RECV);
	}
	// Agrego el caracter nulo al final
	buf[byte_count] = 0;
	char * mens;
	if(debug)printf("Copio el buffer\n");
	strcpy(mens,(char *)buf);
	if(debug)printf("Me respondió: %s\n",mens);

	close(connid);
	close(sockfd);	
	return EXIT_SUCCESS;

}

/* 
	Descripción   : Archivo que contiene la función: conectar_sin_bloquear. La misma permite establecer conexiones vía sockets
					que no se bloqueen. Esto se logra trabajando sobre las propiedades del socket utilizando las funciones:
					FD_ISSET, FD_SET, F_SETFL.

	Actualización : 20140401 Leonardo de - Matteis
	Autor         : ?
	Materia       : Redes de Computadoras
	Institución   : DCIC / Universidad Nacional del Sur

	Modo de uso	  : result = conectar_sin_bloquear(sockfd, &remotehost, sizeof remotehost, segundos);
*/ 
int conectar_sin_bloquear(int sockfd, struct sockaddr_in *host, socklen_t len , int sec)    
{
	int flags, error, n;
	socklen_t elen;
	fd_set rset, wset;
	struct timeval time;

	if ((flags = fcntl (sockfd, F_GETFL, 0)) == -1) 
	{
		perror ("Fcntl error");
	}

	if (fcntl (sockfd, F_SETFL, flags | O_NONBLOCK) == -1) 
	{
		perror ("Fcntl error");
	}

	error = 0;

	if ((n = connect (sockfd, (struct sockaddr *) host, len)) < 0)  
	{
		if (errno != EINPROGRESS)       
		{
			perror("Connect error");
			return (-1);
		}
	}

	if (n == 0)     
	{
		goto done;
	}

	FD_ZERO (&rset);
	FD_SET (sockfd, &rset);
	wset = rset;
	time.tv_sec = sec;
	time.tv_usec = 0;

	if ((n = select (sockfd + 1, &rset, &wset, NULL, sec ? &time : NULL)) == 0)      
	{
		close (sockfd);
		errno = ETIMEDOUT;

		// Puerto filtrado.
		return (-2);
	}

	if (FD_ISSET (sockfd, &rset) || FD_ISSET(sockfd, &wset)) 
	{
		elen = sizeof(error);

		if (getsockopt (sockfd, SOL_SOCKET, SO_ERROR, &error, &elen) < 0)     
		{
			perror("Getsockopt error");

			// Error al inspeccionar el puerto.
			return (-1);
		}
	}

	done:
	if (fcntl (sockfd, F_SETFL, flags) == -1)       
	{
		// Error al cambiar atributos del socket.
		perror ("Fcntl error");
	}

	if (error)      
	{
		close (sockfd);
		errno = error;

		// Puerto cerrado.
		return (-3);
	}

	// Puerto abierto.
	return (0);
}

void main(int a, char** args){
	//
	//if (argc!=2) {
    //  printf("Uso: %s &lt;hostname&gt;\n",argv[0]);
    //  exit(-1);
   	//}
	/*
  	if ((he=gethostbyname(argv[1]))==NULL) {
    	printf("error de gethostbyname()\n");
    	exit(-1);
  	}*/
	if(debug) printf("pase 1\n");
	if (a==2){
		char* ip = args[1];
		consultar(ip);
	}
	if(debug) printf("pase 2\n");
}