/*
	consultar.c

	Se encarga de realizar la conexión mediante socket al servidor DNS, realiza la consulta
	ingresada y muestra por consola la respuesta del servidor DNS.

	Autores:	- Agra Federico (94186)
				- Loza Carlos 	(94399)

*/

//Librerias
#include <stdio.h> 			
#include <string.h>    		
#include <stdlib.h>    		
#include <sys/socket.h>    
#include <arpa/inet.h> 		
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>	

//Modo debug (Muestra printf)
#define DebugMode 0

//Tipo de consultas
#define T_A 1 			//Dirección Ipv4
#define T_MX 15 		//Servidor de Correo Electrónico
#define T_LOC 3			//Información de Localización Geográfica

//Codigo de Errores
#define Err_Env 1
#define Err_Rec 2
#define Sock_Creat 3

//Estructura del Encabezado
struct DNS_HEADER
{
    unsigned short id; 			// Identificador
    
    unsigned char rd :1; 		// Bit Recursion Desired
    unsigned char tc :1; 		// TrunCation: Mensaje truncado
    unsigned char aa :1; 		// Respuesta autoritativa
    unsigned char opcode :4; 	// Tipo de consulta en este mensaje
    unsigned char qr :1; 		// Campo Consulta(0)/Respuesta(1)

    unsigned char rcode :4; 	// Código de respuesta
    unsigned char cd :1;
    unsigned char ad :1;		// Datos Auntenticados
    unsigned char z :1; 		// Reservado para uso futuro
    unsigned char ra :1; 		// Recursión disponible
        
    unsigned short qdcount; 	// Número de preguntas
    unsigned short ancount; 	// Número de respuestas
    unsigned short nscount; 	// Número de registros de autoridad
    unsigned short arcount; 	// Número de registros adicionales
};

//Estructura de la Consulta
struct QUESTION
{
    unsigned short qtype;		//Tipo de Consulta
    unsigned short qclass;		//Clase de Consulta
};

//-------------------------------------------REVISAR-----------------------------------------------------------
#pragma pack(push, 1)
struct R_DATA
{
    unsigned short type;
    unsigned short _class;
    unsigned int ttl;
    unsigned short data_len;
};
#pragma pack(pop)

//Punteros a los contenidos del registro de recursos
struct RES_RECORD
{
    unsigned char * name;
    struct R_DATA * resource;
    unsigned char * rdata;
};

struct D_MAIL
{
    int pref;
    unsigned char * dir;
};
 
//Estructura de una Query
typedef struct
{
    unsigned char * name;
    struct QUESTION * ques;
} QUERY;

//Definición de Funciones
int consultar (unsigned char *, unsigned char *, int, int, int);
void formatoDNS (unsigned char *, unsigned char *);
unsigned char* leerHost (unsigned char *, char buf[], int *, int);
void enviar(int,unsigned char buf[], int , struct sockaddr_in);
void recibir(int,unsigned char buf[], struct sockaddr_in);
void leerEImprimirRespuestas(struct DNS_HEADER *,unsigned char buf[], struct sockaddr_in, unsigned char *);
void printMensaje(struct DNS_HEADER *, struct QUESTION *, unsigned char *);
void printDestino(struct sockaddr_in);

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
 * Método Consultar
 * */
int consultar(unsigned char * host, unsigned char * ip_dns, int n_port, int q_type, int is_rec)
{
	unsigned char buf[65536],* qname,* lector;
    int i , j , parar, tipo, sock;
 
    struct sockaddr_in sai, dest;
 
    struct DNS_HEADER * dns_h = NULL;
    struct QUESTION * qinfo = NULL;
    
    if((sock = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP)) < 0)
	{
   		perror("Error al crear el socket");
		exit(Sock_Creat);
	}
	else
	{
		printf("Socket creado correctamente.\n");
	}

	//-----------------------------debugMode----------------------------
	if(DebugMode) printf("\nSocket %i (%i, %i, %i)\n", sock,AF_INET , SOCK_DGRAM , IPPROTO_UDP);

    dest.sin_family = AF_INET;
    dest.sin_port = htons(n_port);
    dest.sin_addr.s_addr = inet_addr(ip_dns);
 
    //Seteamos la estructura del encabezado DNS como una consulta estandar
    dns_h = (struct DNS_HEADER *) &buf;
 
    dns_h->id = (unsigned short) htons(getpid());		//Usamos el pid del proceso actual como id
    dns_h->qr = 0; 				
    dns_h->opcode = 0000; 			
    dns_h->aa = 0; 				
    dns_h->tc = 0;				
    dns_h->rd = is_rec; 		
    dns_h->ra = 0; 				
    dns_h->z = 0;
    dns_h->ad = 1;
    dns_h->cd = 0;
    dns_h->rcode = 0;
    dns_h->qdcount = htons(1);
    dns_h->ancount = 0;
    dns_h->nscount = 0;
    dns_h->arcount = 0;

    //-------------------------------------------REVISAR-----------------------------------------------------------
    qname = (unsigned char *) &buf[sizeof(struct DNS_HEADER)];
 	
    formatoDNS(qname, host);

    qinfo =(struct QUESTION*)&buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; 
 
 	qinfo->qtype = htons(q_type);
    qinfo->qclass = htons(1);			//Clase Internet = 1

    //-----------------------------debugMode----------------------------
   	if(DebugMode) printMensaje(dns_h, qinfo, qname);

   	//-----------------------------debugMode----------------------------
   	if(DebugMode) printDestino(dest);
 	
    //Envio la consulta
    //enviar(sock, buf, sizeof(struct DNS_HEADER) + (strlen((const char *)qname)+1) + sizeof(struct QUESTION), dest);
 	//----------------------------------------------------------enviar-----------------------------------------------------------
   	if( sendto(sock, (char*) buf, sizeof(struct DNS_HEADER) + (strlen((const char *)qname)+1) + sizeof(struct QUESTION), 0, (struct sockaddr *) &dest, sizeof(dest)) < 0)
    {
        perror("Error al enviar la consulta.");
        exit(Err_Env);
    }
    else
    {
    	printf("Consulta Enviada.\n");
    }
    //---------------------------------------------------------------------------------------------------------------------------

   	//recibir(sock, buf, dest);
    //----------------------------------------------------------recibir--------------------------------------------------------
    i = sizeof dest;
	int byte_count;
    if(byte_count = recvfrom (sock, (char *) buf, sizeof(buf)-1, 0, (struct sockaddr*) &dest, (socklen_t *)&i ) < 0)
    {
        perror("Error al recibir la respuesta");
        exit(Err_Rec);
    }
    else
    {
    	printf("Respuesta Recibida.\n");
    }
    //--------------------------------------------------------------------------------------------------------------------------
    for(i=0; i<sizeof(buf)-1;i++)
    {
    	//printf("%c", buf[i]);
    }
    
    dns_h = (struct DNS_HEADER*) buf;
 	
    lector = &buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION)];
       
    printf("\nContenido de la respuesta recibida : ");
    printf("\n %d Consultas.", ntohs(dns_h->qdcount));
    printf("\n %d Respuestas.", ntohs(dns_h->ancount));
    printf("\n %d Servidores Autoritativos.", ntohs(dns_h->nscount));
    printf("\n %d Registros Adicionales.\n\n", ntohs(dns_h->arcount));
 
   	//Leemos e imprimimos las respuestas
   	//leerEImprimirRespuestas(dns_h, buf, sai, lector);
   	//---------------------------------------------------------leerImprimir-----------------------------------------------------
    //int i, j, tipo, parar = 0;
	struct RES_RECORD resp[20], autor[20], adic[20];

    for(i=0; i<ntohs(dns_h->ancount); i++)
    {
    	resp[i].name = leerHost(lector, buf, &parar,0);
		lector = lector + parar;

		resp[i].resource = (struct R_DATA*)(lector);
		lector = lector + sizeof(struct R_DATA);
	
		tipo = ntohs(resp[i].resource->type);

        switch (tipo)
        {
        	case 1:	//Si es una consulta de conversión de nombre
        	{
        		resp[i].rdata = (unsigned char*)malloc(ntohs(resp[i].resource->data_len));

        		//printf("%i\n", resp[i].resource->data_len);

        		//for(j=0; j<ntohs(resp[i].resource->data_len);j++)
			    //{
			    //	printf("%d", lector[j]);
			    //}

	            for(j=0 ; j<ntohs(resp[i].resource->data_len) ; j++)
	            {
	                resp[i].rdata[j] = lector[j];
	            }
	 
	            resp[i].rdata[ntohs(resp[i].resource->data_len)] = '\0';
	 
	            lector = lector + ntohs(resp[i].resource->data_len);

	            printf("Leemos respuestas del tipo A.\n");

	            break;
        	}
        	case 15: //Si es una consulta de servidor de correo electrónico
        	{
        		resp[i].rdata = (unsigned char*)malloc(ntohs(resp[i].resource->data_len));
        		
        		for(j=0 ; j<ntohs(resp[i].resource->data_len) ; j++)
	            {
	               	resp[i].rdata[j] = lector[j];
	                           
	            }
	            resp[i].rdata[ntohs(resp[i].resource->data_len)] = '\0';
	 
	            lector = lector + ntohs(resp[i].resource->data_len);

	            printf("Leemos respuestas del tipo MX.\n");
				
	            break;

        	}
        	default:
        	{
            	resp[i].rdata = leerHost(lector, buf, &parar,0);
            	lector = lector + parar;
        	}
        }
        
    }
 
    //Leemos Respuestas Autoritativas
    for(i=0; i<ntohs(dns_h->nscount); i++)
    {
        autor[i].name = leerHost(lector, buf, &parar, 49152);
        lector += parar;
 
        autor[i].resource = (struct R_DATA*)(lector);
        lector += sizeof(struct R_DATA);
 
        autor[i].rdata = leerHost(lector, buf, &parar, 49152);
        lector += parar;
    }
 
    //Leemos Registros Adicionales (Método)
    for(i=0; i<ntohs(dns_h->arcount); i++)
    {
        adic[i].name = leerHost(lector, buf, &parar, 0);
        lector += parar;
 
        adic[i].resource = (struct R_DATA*)(lector);
        lector += sizeof(struct R_DATA);
 
        tipo = ntohs(adic[i].resource->type);

        switch (tipo)
        {
        	case 1:		//Si es una consulta de conversión de nombre
        	{
        		adic[i].rdata = (unsigned char*)malloc(ntohs(adic[i].resource->data_len));
 
	            for(j=0 ; j<ntohs(adic[i].resource->data_len) ; j++)
	            {
	                adic[i].rdata[j] = lector[j];
	            }
	 
	            adic[i].rdata[ntohs(adic[i].resource->data_len)] = '\0';
	 
	            lector = lector + ntohs(adic[i].resource->data_len);
	            printf("Leemos respuestas Adicionales del tipo A.\n");

	            break;
        	}
        	case 15:	//Si es una consulta de servidor de correo electrónico
        	{

        		adic[i].rdata = (unsigned char*)malloc(ntohs(adic[i].resource->data_len));
 
	            for(j=0 ; j<ntohs(adic[i].resource->data_len) ; j++)
	            {
	                adic[i].rdata[j] = lector[j];
	            }
	 
	            adic[i].rdata[ntohs(adic[i].resource->data_len)] = '\0';
	 
	            lector = lector + ntohs(adic[i].resource->data_len);
        		printf("Leemos respuestas Adicionales del tipo MX.\n");

	            break;

        	}
        	default:
        	{
            	adic[i].rdata = leerHost(lector, buf, &parar, 0);
            	lector = lector + parar;
        	}
    	}
    }

    //Imprimimos Respuestas
    printf("\nNúmero de Respuestas : %d \n" , ntohs(dns_h->ancount) );
    for(i=0 ; i < ntohs(dns_h->ancount) ; i++)
    {
        printf("Respuesta %i: El Host %s ", i+1,resp[i].name);
 		
 		tipo = ntohs(resp[i].resource->type);

        switch (tipo)
        {
        	case 1:		//Si es una consulta de conversión de nombre
        	{
        		long *p;
           		p = (long*)resp[i].rdata;
            	sai.sin_addr.s_addr = (* p);

            	printf("tiene como dirección IPv4 : %s\n", inet_ntoa(sai.sin_addr));
        		break;
        	}
        	case 15:	//Si es una consulta de servidor de correo electrónico
        	{
        		//struct D_MAIL * sv_mail = (struct D_MAIL *)malloc(ntohs(sizeof(int) + sizeof(unsigned char *)));
        		//sv_mail->pref = (int) resp[i].rdata[1];
        		lector = &resp[i].rdata[2];
        		char * d_mail = leerHost(lector,resp[i].rdata,&parar,0);
        		//sv_mail->dir = malloc(strlen(d_mail)+1);
        		//strcpy(sv_mail->dir,d_mail);
        		printf("tiene como servidor de correo electrónico a %s - Preferencia: %i\n", d_mail,resp[i].rdata[1]);
        		//free(sv_mail->dir);
        		//free(sv_mail);
        		break;
        	}
        	default:
        	{
        		printf("Respuesta: La Localización va aca\n");
        	}
        }
        
        printf("\n");
    }

    //Imprimimos Respuestas Autoritativas
    printf("\nNúmero de Respuestas Autoritarias : %d \n" , ntohs(dns_h->nscount) );
    for( i=0 ; i < ntohs(dns_h->nscount) ; i++)
    {
        //if(ntohs(autor[i].resource->type)==2)
        {
        	printf("Autoritativa: El Host %s tiene como alias : %s ",autor[i].name ,autor[i].rdata);
        	printf("\n");
        }
    }

    //print additional resource records
    printf("\nNúmero de Respuestas Adicionales : %d \n" , ntohs(dns_h->arcount) );
    for(i=0; i < ntohs(dns_h->arcount) ; i++)
    {
        printf("Adicionales: El Host %s ",adic[i].name);

        tipo = ntohs(adic[i].resource->type);

        switch (tipo)
        {
        	case 1:		//Si es una consulta de conversión de nombre
        	{
        		long *p;
           		p = (long*)adic[i].rdata;
            	sai.sin_addr.s_addr = (* p);
            	printf("tiene como dirección IPv4 : %s\n", inet_ntoa(sai.sin_addr));
        		break;
        	}
        	case 15:	//Si es una consulta de servidor de correo electrónico
        	{
        		lector = &adic[i].rdata[2];
        		char * d_mail = leerHost(lector,adic[i].rdata,&parar,0);
        		printf("tiene como servidor de correo electrónico a %s - Preferencia: %i\n", d_mail,adic[i].rdata[1]);

        		/*
        		struct D_MAIL * sv_mail = (struct D_MAIL *)malloc(ntohs(sizeof(int) + sizeof(unsigned char *)));
        		sv_mail->pref = (int) resp[i].rdata[1];
        		lector = &resp[i].rdata[2];
        		char * d_mail = leerHost(lector,resp[i].rdata,&parar);
        		sv_mail->dir = malloc(strlen(d_mail) + strlen(resp[i].name));
        		strcat(sv_mail->dir,d_mail);
        		strcat(sv_mail->dir,resp[i].name);
        		printf("tiene como servidor de correo electrónico a %s - Preferencia: %i\n", sv_mail->dir,sv_mail->pref);
        		free(sv_mail->dir);
        		free(sv_mail);*/
        		break;
        	}
        	default:
        	{
        		printf("Adicionales: La Localización va aca\n");
        	}
        }
    }
    //-----------------------------------------------------------------------------------------------------------------
}

/*
	Método enviar
*/
void enviar(int sock,unsigned char buf[], int size, struct sockaddr_in dest)
{
	
}

/*
	Método recibir
*/
void recibir(int sock, unsigned char buf[], struct sockaddr_in dest)
{
	
}

void leerEImprimirRespuestas(struct DNS_HEADER * dns_h,unsigned char buf[], struct sockaddr_in sai, unsigned char * lector)
{
	
}

/*
	Método formatoDNS

	Transforma la dirección host a formato DNS.
	Convierte los '.' en la cantidad de caracteres hasta el próximo punto
	Ejemplo: cs.uns.edu.ar -> 2cs3uns3edu2ar0

 */
void formatoDNS(unsigned char* dns, unsigned char* host) 
{
    int lock = 0 , i;
    strcat((char*)host,".");
     
    for(i = 0 ; i < strlen((char*)host) ; i++) 
    {
        if(host[i]=='.') 
        {
            *dns++ = i-lock;
            for(0;lock<i;lock++) 
            {
                *dns++=host[lock];
            }
            lock++; //or lock=i+1;
        }
    }
    *dns++='\0';
}

/*
	Método LeerHost

	Lee los nombres de Host de formato DNS y los transforma a nombres de Host normales
	Ejemplo: 2cs3uns3edu2ar0 -> cs.uns.edu.ar
 */
u_char* leerHost(unsigned char* _lec, char _buff[], int* _cont, int _desp)
{
    unsigned char *name;
    unsigned int p = 0, saltar = 0, offset;
    int i , j;
 
    *_cont = 1;
    name = (unsigned char*)malloc(256);
 
    name[0] = '\0';
 
    while(*_lec != 0)
    {
        if(*_lec >= 192)
        {
            offset = (*_lec)*256 + *(_lec+1) - _desp;
            _lec = _buff + offset - 1;
            saltar = 1; 								
        }
        else
        {
            name[p++] = *_lec;
        }
 
        _lec = _lec+1;
 
        if(saltar == 0)
        {
            *_cont = *_cont + 1; 
        }
    }
 
    name[p] = '\0';

    if(saltar == 1)
    {
        *_cont = *_cont + 1;
    }
 
    for(i=0; i<(int)strlen((const char*)name); i++) 
    {
        p = name[i];
        for(j=0;j<(int)p;j++) 
        {
            name[i] = name[i+1];
            i = i+1;
        }
        name[i] = '.';
    }

    name[i-1] = '\0';
    
    return name;
}

void printMensaje(struct DNS_HEADER * dns, struct QUESTION * qinfo, unsigned char * qname)
{
	printf("..........................................QUERY.....................................\n");
	printf("........................................dns_header..................................\n");
	printf("id = %d\n", dns->id);
	printf("rd = %d\n", dns->rd);
	printf("tc = %d\n", dns->tc);
	printf("aa = %d\n", dns->aa);
	printf("opcode = %d\n", dns->opcode);
	printf("qr = %d\n", dns->qr);
	printf("rcode = %d\n", dns->rcode);
	printf("cd = %d\n", dns->cd);
	printf("ad = %d\n", dns->ad);
	printf("z = %d\n", dns->z);
	printf("ra = %d\n", dns->ra);
	printf("qdcount = %d\n", dns->qdcount);
	printf("ancount = %d\n", dns->ancount);
	printf("nscount = %d\n", dns->nscount);
	printf("arcount = %d\n", dns->arcount);
	printf("..........................................qinfo...................................\n");
	printf("qtype = %d\n", qinfo->qtype);
	printf("qclass = %d\n", qinfo->qclass);
	printf("..........................................qname...................................\n");
	printf("qname = %hhn\n", qname);
}

void printDestino(struct sockaddr_in destino)
{
    printf(".........................................Destino....................................\n");
    printf("sin_family = %d\n", destino.sin_family);
    printf("sin_port = %d\n", destino.sin_port);
    printf("in_addr = %i\n", destino.sin_addr.s_addr);
    printf("sin_zero[8] = %hhn\n", destino.sin_zero);
}