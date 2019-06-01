#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <resolv.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>

//Incluyo la interface del módulo dns
#include "consultar.h"

//Constantes -> Codigo de Salida de Errores
const int IP_INV = 2;
const int ERR_PARAM = 3;
#define debug 0

//colores y estilos para printf
#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */


int cantParametros;
int _h = 0;
int _a = 0;
int _mx = 0;
int _loc = 0;
int _r = 0;
int _t = 0;
int tipo;
char* servidor=NULL;
int puerto=0;
char* consulta=NULL;

void obtenerHost (char* );
void get_dns_default();
void ayuda(int);
void ayudaConsulta();
void ayudaServidor();
void ayudaPuerto();
void ayudaA();
void ayudaMx();
void ayudaLoc();
void ayudaR();
void ayudaT();

//printfafa(char* cadena, int entreros );

/*
 * Procedimiento ayuda
 * 
 * Se utiliza cuando se ingresan por parametro datos incorrectos o cuando 
 * se invoca al programa con el parametro -h.
 * Muestra por consola el modo de invocacion del programa, incluyendo las
 * distintas opciones del programa.
 * 
 */
void ayuda(int completa){
		if(completa){ //<-- lo q es igual a (_r+_a+_t+_loc+_mx==0) && (servidor==NULL && puerto==NULL && consulta==NULL)
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
			printf("\n=== SE MOSTRARA LA AYUDA SEGUN PARAMETROS PRESENTES ===\n\n");
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
void ayudaConsulta(){
	printf(BOLDGREEN "Consulta: \n" RESET);
	printf("Este argumento, el cual debe estar siempre prensente salvo que se este \n");
	printf("invocando a la ayuda, es la consulta que se desea resolver (en general,\n");
	printf("es la cadena de caracteres denotando el nombre simbolico que se desea\n");
	printf("mapear a un IP\n\n");
	printf("Ej:\n" "~$ dnsquery "BOLDYELLOW"www.wikipedia.org"RESET"\n\n\n");
}
/*
 * Procedimiento ayudaServidor
 * 
 */
void ayudaServidor(){
	printf(BOLDGREEN "Servidor: \n" RESET);
	printf("Este argumento es el servidor DNS con el que se resolverá la consulta\n");
	printf("suministrada, en caso de no estar presente este argumento se resolvera\n");
	printf("con un servidor DNS por defecto\n\n");
	printf("Ej:\n" "~$ dnsquery www.wikipedia.org "BOLDYELLOW"@8.8.8.8"RESET"\n\n\n");
}

/*
 * Procedimiento ayudaPuerto
 * 
 */
void ayudaPuerto(){
	printf(BOLDGREEN "Puerto: \n" RESET);
	printf("este parametro es opcional, el cual solo puede ser especificado si tambien\n");
	printf("se especifico un servidor, permite indicar que el servidor contra el cual\n");
	printf("se resolvera la consulta no está ligado al puerto DNS estandar. Caso que\n");
	printf("no se indique se asume que la consulta sera dirida al puerto estandar del\n");
	printf("servidor\n\n");
	printf("Ej:\n" "~$ dnsquery www.wikipedia.org @8.8.8.8"BOLDYELLOW":53"RESET"\n\n\n");
}

/*
 * Procedimiento ayudaA
 * 
 */
void ayudaA(){
	printf(BOLDGREEN "\"-a\": \n" RESET);
	printf("Si se espesifica este parametro, la consulta se trata de un nombre simbolico\n");
	printf("y se desea conocer su correspondiente IP numerico asociado\n");
	printf("Los opcionales \"-a\" \"-mx\" y \"-loc\" son excluyentes entre si, en caso de no\n");
	printf("espesificarse ninguno, se asume \"-a\"\n\n");
	printf("Ej:\n" "~$ dnsquery www.wikipedia.org "BOLDYELLOW"-a"RESET"\n\n\n");
}
/*
 * Procedimiento ayudaMx
 * 
 */
void ayudaMx(){
	printf(BOLDGREEN "\"-mx\": \n" RESET);
	printf("Si se espesifica este parametro, se desea determinar el servidor a cargo de\n");
	printf("la recepcion de correo electronico para el dominio indicado en la consulta\n");
	printf("Los opcionales \"-a\" \"-mx\" y \"-loc\" son excluyentes entre si, en caso de no\n");
	printf("espesificarse ninguno, se asume \"-a\"\n\n");
	printf("Ej:\n" "~$ dnsquery www.wikipedia.org "BOLDYELLOW"-mx"RESET"\n\n\n");
}
/*
 * Procedimiento ayudaLoc
 * 
 */

void ayudaLoc(){
	printf(BOLDGREEN "\"-loc\": \n" RESET);
	printf("Si se espesifica este parametro, se desea recuperar la informacion relativa\n");
	printf("a la ubicacion geográfica del dominio indicado en la consulta\n");
	printf("Los opcionales \"-a\" \"-mx\" y \"-loc\" son excluyentes entre si, en caso de no\n");
	printf("espesificarse ninguno, se asume \"-a\"\n\n");
	printf("Ej:\n" "~$ dnsquery www.wikipedia.org "BOLDYELLOW"-loc"RESET"\n\n\n");
}
/*
 * Procedimiento ayudaR
 * 
 */
void ayudaR(){
	printf(BOLDGREEN "\"-r\": \n" RESET);
	printf("en caso de espesificarse este parametro, se esta solicitando que la ");
	printf("consulta contega el bit de recursion desired activado es decir, puede");
	printf("pasar una de dos cosas, o bien el servidor acepta la consulta");
	printf("recursica y retorna la respuesta definitiva, o bien rechaza ese tipo");
	printf("de consulta, por lo que no se podra obtener la respuesta definitivva");
	printf("bajo esas condiciones");
	printf("Los opcionales \"-r\" y \"-t\" son excluyentes entre si, en caso de no\n");
	printf("espesificarse ninguno, se asume por defecto que se resuelve de mandera\n");
	printf("recursiva\n");
	printf("Ej:\n" "~$ dnsquery www.wikipedia.org "BOLDYELLOW"-r"RESET"\n\n\n");
}
/*
 * Procedimiento ayudaT
 * 
 */
void ayudaT(){
	printf(BOLDGREEN "\"-t\": \n" RESET);
	printf("En caso de espesificarse este parametro, se solicita que la consulta");
	printf("se resuelva iterativamente, mostrando na traza con la evolucion de la");
	printf("misma, esto es, mostrando las distintas respuesta parciales que se");
	printf("van obteniendo al efectuar una consulta no recursiva hasta dar con la");
	printf("respuesta final, similar a la funcionalida provista por el comando");
	printf("dig con la opcion --trace");
	printf("Los opcionales \"-r\" y \"-t\" son excluyentes entre si, en caso de no\n");
	printf("espesificarse ninguno, se asume por defecto que se resuelve de mandera\n");
	printf("recursiva\n");
	printf("Ej:\n" "~$ dnsquery www.wikipedia.org "BOLDYELLOW"-t"RESET"\n\n\n");
}


void masticarParametros(int argc, char *argv[]){
	int i=0;
	char* parametro;
	cantParametros = argc;
	if((cantParametros>1) && (cantParametros<7))
	{
		//char** restos = malloc(sizeof(char*)*argc); int cantResto = 0;
		for(i=0; i<argc; i++){
			if(strcmp(argv[i], "-h")==0){
				_h = 1;
			}
			else if(strcmp(argv[i], "-t")==0){
				_t = 1;
			}
			//else if(strcmp(argv[i], "@")==0){
			else if(strchr(argv[i], '@')!=NULL){
				int largo = strlen(argv[i]);
				int _puer = 1;
				for(_puer = 1; _puer < largo; _puer++){
					if(debug) printf("estoy en serv %s\n", argv[i]);
					if(argv[i][_puer]==':'){
						char * ret = substring(argv[i], _puer+1, largo);;
						puerto = (int) strtol(ret, NULL, 10);
						break;
					}
				}
				servidor = substring(argv[i], 1, _puer);
				if(debug)printf("servidor: %s | puerto: %i\n", servidor, puerto);			
			}
			if(strcmp(argv[i], "-a") == 0)
			{
				_a = 1;
				tipo = T_A;
			}
			if(strcmp(argv[i], "-mx") == 0)
			{
				_mx = 1;
				tipo = T_MX;
			}
			if(strcmp(argv[i], "-loc") == 0)
			{
				_loc = 1;
				tipo = T_LOC;
			}
			//else{	restos[cantResto] = argv[i];	}
		}
	}
	else
	{
		ayuda(1);
		exit(ERR_PARAM);
	}
	
	
	//for(i=0; i<argc; i++){ otras opciones }
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
	if((argc==1) || (_h!=0))
	{
		//un solo parametro o -h activado
		ayuda(0);
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
		if ((_loc + _mx + _a)>1)
		{
			ayuda(0);
			exit(ERR_PARAM);
		}
		if ((_loc + _mx + _a)==0)
		{
			tipo = T_A;	//Por defecto es una consulta tipo -a
		}
		//dnsquery <consulta>
		obtenerHost(argv[1]);
		
		printf("\nConsulta = %s (%li)\nServidor DNS = %s(%li)\nPuerto = %i\nRecursion = %i\n\n", 
			consulta, strlen(consulta), servidor, strlen(servidor),puerto, recursion );

		consultar(consulta,servidor,puerto,tipo,recursion);
			
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

