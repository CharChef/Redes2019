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
#include <arpa/inet.h>

//Incluyo la interface del módulo dns
#include "dns.h"

//Constantes -> Codigo de Salida de Errores
const int IP_INV = 2;


int cantParametros;
int _h = 0;
int _a = 0;
int _mx = 0;
int _loc = 0;
int _r = 0;
int _t = 0;
char* servidor=NULL;
int puerto=0;
char* consulta=NULL;

void obtenerHost (char* );
char* substring(const char*, int, int);
void get_dns_default();
void ayuda();
void ayudaConsulta();
void ayudaServidor();
void ayudaPuerto();
void ayudaA();
void ayudaMx();
void ayudaLoc();
void ayudaR();
void ayudaT();

//printfafa(char* cadena, int entreros );

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
 * Procedimiento ayuda
 * 
 * Se utiliza cuando se ingresan por parametro datos incorrectos o cuando 
 * se invoca al programa con el parametro -h.
 * Muestra por consola el modo de invocacion del programa, incluyendo las
 * distintas opciones del programa.
 * 
 */
void ayuda(){
		if(cantParametros <= 2){ //<-- lo q es igual a (_r+_a+_t+_loc+_mx==0) && (servidor==NULL && puerto==NULL && consulta==NULL)
			printf("MODO DE USO: dnsquery consulta @servidor[:puerto] [-a | -mx | -loc] [-r | -t] [-h]\n");
            printf("OPCION\t\t\tDESCRIPCION\n");

            printf("consulta\t\tEs la consulta (dirección IP o nombre de Host) que se desea resolver.\n");
            printf("@servidor\t\tServidor DNS con el que se resolvera la consulta.\n\t\t\tSi no se determina se usa el servidor DNS por defecto.\n");
            printf(":puerto\t\t\tPuerto DNS con el cual el servidor que resolvera la consulta esta ligado.\n\t\t\tSi no se determina se utiliza el puerto DNS estandar (53).\n");
            printf("-a\t\t\tConsulta de resolución de nombre.\n");
            printf("-mx\t\t\tSevidor a cargo de la recepción de correo electrónico para el dominio indicado en la consulta.\n");
            printf("-loc\t\t\tInformación relativa a la localización geográfica del dominio indicado en la consulta.\n");
            printf("-r\t\t\tConsulta Recursiva.\n");
            printf("-t\t\t\tConsulta Iterativa.\n");
            printf("-h\t\t\tMuestra esta ayuda.\n");
		}
		else{ //este else podria no estar
			if(consulta!=NULL)ayudaConsulta();
			if(servidor!=NULL)ayudaServidor();
			if(puerto!=0)ayudaPuerto();
			if(_a)ayudaA();
			if(_mx)ayudaMx();
			if(_loc)ayudaLoc();
			if(_r)ayudaR();
			if(_t)ayudaT();
		}
		
}

/*
 * Procedimiento ayudaConsulta
 * 
 */
void ayudaConsulta(){}
/*
 * Procedimiento ayudaServidor
 * 
 */
void ayudaServidor(){}
/*
 * Procedimiento ayudaPuerto
 * 
 */
void ayudaPuerto(){}
/*
 * Procedimiento ayudaA
 * 
 */
void ayudaA(){}
/*
 * Procedimiento ayudaMx
 * 
 */
void ayudaMx(){}
/*
 * Procedimiento ayudaLoc
 * 
 */
void ayudaLoc(){}
/*
 * Procedimiento ayudaR
 * 
 */
void ayudaR(){}
/*
 * Procedimiento ayudaT
 * 
 */
void ayudaT(){}


void masticarParametros(int argc, char *argv[]){
	int i=0;
	char* parametro;
	for(i=0; i<argc; i++){
		if(strcmp(argv[i], "-h")==0)
		{
			_h = 1;
			break;
		}
	}
	for(i=0; i<argc; i++){
		if(strcmp(argv[i], "-t")==0)
		{
			_t = 1;
			break;
		}
	}
	for(i=0; i<argc; i++){
		if(strchr(argv[i], '@')!=NULL)
		{
			servidor = substring(argv[i],1,strlen(argv[i]));
			break;
		}
	}
	//tiene que seguir
}

/*
 * Metodo principal (main)
 * 
 * Obtiene los parametros ingresados por consola, y realiza la
 * operacion correspondiente.
 * Retorna 	EXIT_SUCCES -> Terminacion exitosa
 * 			1 -> Error de Rango
 * 			2 -> Error de direccion IP
 * 			3 -> Error de Parametros
 * 
 */
void main(int argc, char *argv[]) {                   
	int help = 0;
	masticarParametros(argc,argv);
	if(argc==1 || _h==1)
	{
		//un solo parametro o -h activado
		ayuda();
	}
	else
	{
		int recursion;
		if(servidor==NULL) get_dns_default();
		if(puerto==0) puerto=53;
		if(_t==1)
		{
			recursion = 0;
		}
		else
		{
			recursion = 1;
		}
		//dnsquery <consulta>
		obtenerHost(argv[1]);
		
		printf("\nConsulta = %s (%li)\nServidor DNS = %s(%li)\nPuerto = %i\nRecursion = %i\n\n", consulta, strlen(consulta), servidor, strlen(servidor),puerto, recursion );

		consultar(consulta,servidor,puerto,recursion);
			
	}
}

/*
 * 
 * 
 */
void obtenerHost (char* host_argumento){
	
	struct hostent *host;
	
	//Recibo un nombre
    if((host = gethostbyname(host_argumento)) != 0)
    {
		consulta = host_argumento;
	}
	else
	{
		herror(host_argumento);
		exit(IP_INV);
	}
}
	

/*
 * Get the DNS server from /etc/resolv.conf file on Linux
 * */
void get_dns_default()
{
    FILE *fp;
    char line[200] , *p;
    if((fp = fopen("/etc/resolv.conf" , "r")) == NULL)
    {
        printf("Failed opening /etc/resolv.conf file \n");
    }
    while(fgets(line , 200 , fp))
    {
        if(line[0] == '#')
        {
            continue;
        }
        if(strncmp(line , "nameserver" , 10) == 0)
        {
            p = strtok(line , " ");
            p = strtok(NULL , " ");

            //Suena chancho, pero con el strcpy tiraba segmentation fault
            //y asi anda a la pelusha
            int Longitud = strlen(p);
            //servidor = substring(p,0,10);
            servidor = substring(p,0,strlen(p));            
        }
    }
    
}
		/*
		int i;
		for (i=0; i<argc; i++)
		{	
			if(strcmp(argv[i],"-h")==0)
			{
				//encontro un -h en algun lugar
				help = 1;
				break;
			}
		}
		if(help==1) 
		{ 
			// -h
			ayuda();
		}
		else
		{
			if(argc==2)
			{
				//apuertos <ip>
				obtenerIP(argv[1]);
				//analizar() <- debe verificar que el ip sea valido
				analizar(ip_Host,1,1024);
			}
			else
			{
				if(strcmp(argv[2],"-r")==0)
				{
					//hay mas de 2 argumentos
					if(argv[3] != NULL)
					{
						//apuertos <ip> -r <LP::HP>
						if(obtenerLH(argv[3])==0)
						{
							//obtengo el <ip>
							obtenerIP(argv[1]);
							//llama a analizar(ip,lp,hp)
							analizar(ip_Host,lp,hp);
						}
						else
						{
							//Error en el rango
							printf("RANGO_INV: Rango invalido");
							exit(RANGO_INV);
						}						
					}
					else
					{
						//apuertos <ip> -r
						ayuda();
						printf("PARAM_INV: Error de parametros");						
						exit(PARAM_INV);
					}
				}
				else
				{
					//apuertos <ip> <cualquier otra cosa>
					ayuda();
					printf("PARAM_INV: Error de parametros");
					exit(PARAM_INV);
				}
			}	
		}
		*/	

