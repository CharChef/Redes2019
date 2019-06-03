/*
	Redes de Computadoras - 2019
	Proyecto: Mini-Cliente de Consultas de Servidores DNS

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
#include <math.h>

//Definimos los Tipos Soportados
#define T_A 1
#define T_NS 2
#define T_CNAME 5
#define T_MX 15
#define T_AAAA 28
#define T_LOC 29

//Constantes -> Codigo de Errores
const int ERR_ENV = 2;
const int ERR_REC = 3;
const int ERR_SOCK_CREAT = 4;

//Estructura Utilizada por el Procedimiento loc_ntoa
static unsigned int poweroften[10] = {1, 10, 100, 1000, 10000, 100000,1000000,10000000,100000000,1000000000};

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

//Etructura de Datos de las Respuestas
#pragma pack(push, 1)
struct R_DATA
{
    unsigned short type;
    unsigned short _class;
    unsigned int ttl;
    unsigned short data_len;
};
#pragma pack(pop)

//Estructura de Respuestas
struct RES_RECORD
{
    unsigned char * name;
    struct R_DATA * resource;
    unsigned char * rdata;
};

//Estructura de una Query
typedef struct
{
    unsigned char * name;
    struct QUESTION * ques;
} QUERY;

//Definición de Funciones
char * consultar (unsigned char *, unsigned char *, int, int, int, int);
void formatoDNS (unsigned char *, unsigned char *);
unsigned char* leerHost (unsigned char *, char buf[], int *);
int convertirAMetros(int n_hex[]);
void pasarAHexa(int, int n_hex[]);
unsigned int getLatLonAlt(unsigned int n_lla[]);


/* 	
	Procedimiento loc_ntoa

	Devuelve los datos obtenidos por una respuesta de tipo LOC de manera
	amigable al usuario. Este procedimiento fue sacado (y modificado) del Anexo del 
	"RFC 1876 - A Means for Expressing Location Information in the Domain Name System"
	(https://tools.ietf.org/html/rfc1876)
 */
char *
loc_ntoa(int i_latval, int i_longval, int i_altval)
{
        char *cp;
        const u_char *rcp;

        int latdeg, latmin, latsec, latsecfrac;
        int longdeg, longmin, longsec, longsecfrac;
        char northsouth, eastwest;
        int altmeters, altfrac, altsign;

        const int referencealt = 100000 * 100;

        int32_t latval, longval, altval;
       	
        latval = (i_latval - ((unsigned)1<<31));

        longval = (i_longval - ((unsigned)1<<31));

        if (i_altval < referencealt) { /* below WGS 84 spheroid */
                altval = referencealt - i_altval;
                altsign = -1;
        } else {
                altval = i_altval - referencealt;
                altsign = 1;
        }

        if (latval < 0) {
                northsouth = 'S';
                latval = -latval;
        }
        else
                northsouth = 'N';

        latsecfrac = latval % 1000;
        latval = latval / 1000;
        latsec = latval % 60;
        latval = latval / 60;
        latmin = latval % 60;
        latval = latval / 60;
        latdeg = latval;

        if (longval < 0) {
                eastwest = 'W';
                longval = -longval;
        }
        else
                eastwest = 'E';

        longsecfrac = longval % 1000;
        longval = longval / 1000;
        longsec = longval % 60;
        longval = longval / 60;
        longmin = longval % 60;
        longval = longval / 60;
        longdeg = longval;

        altfrac = altval % 100;
        altmeters = (altval / 100) * altsign;
        
        cp = malloc(2048);
        	sprintf(cp,
                "Latitud: %d dec %.2d min %.2d.%.3d sec | %c\n\t\tLongitud: %d dec %.2d min %.2d.%.3d sec | %c\n\t\tAltura: %d.%.2d m",
                latdeg, latmin, latsec, latsecfrac, northsouth,
                longdeg, longmin, longsec, longsecfrac, eastwest,
                altmeters, altfrac);
        
        
    	return (cp);
}


/*
 	Meétodo substring

 	Retorna una subcadena de caracteres de tamaño n, comenzando desde la posicion beg, de la string str.

 */
char* substring(const char* str, int beg, int n)
{
   char *sub = malloc(n+1); 
   strncpy(sub, (str + beg), n);
   *(sub+n) = 0;

   return sub;
}

/*
 	Método Consultar

 	Se encarga de enviar consultas dns a un servidor, asi como también de recibir y mostrar por pantalla
 	las respuestas obtenidas.
 */
char * consultar(unsigned char * host, unsigned char * ip_dns, int n_port, int q_type, int is_rec, int modoPrint)
{
	char * salidaIP = NULL;
	char * salidaAddr = NULL;
	
	unsigned char buf[65536],* qname,* lector, *laux;
    int i , j , parar, tipo, sock;
 
    struct sockaddr_in sai, dest;
 
    struct DNS_HEADER * dns_h = NULL;
    struct QUESTION * qinfo = NULL;
    
    if((sock = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP)) < 0)
	{
   		perror("- Error al crear el socket -");
		exit(ERR_SOCK_CREAT);
	}
	else
	{
		if(modoPrint)  	printf("- Socket creado correctamente -\n");
	}

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
    
    qname = (unsigned char *) &buf[sizeof(struct DNS_HEADER)];
 	
 	if(strcmp(host,".")==0)
 	{}
 	else
 	{
 		formatoDNS(qname, host);
  	}
 	   
    qinfo =(struct QUESTION*)&buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; 
 	
 	if(is_rec)
 	{
 		qinfo->qtype = htons(q_type);
 	}
 	else
 	{
 		qinfo->qtype = htons(T_NS);
 	}

 	qinfo->qclass = htons(1);			//Clase Internet = 1
 
 	//Envio la consulta
 	//----------------------------------------------------------enviar-----------------------------------------------------------
   	if( sendto(sock, (char*) buf, sizeof(struct DNS_HEADER) + (strlen((const char *)qname)+1) + sizeof(struct QUESTION), 0, (struct sockaddr *) &dest, sizeof(dest)) < 0)
    {
        perror("- Error al enviar la consulta -");
        exit(ERR_ENV);
    }
    else
    {
		if(modoPrint)  	printf("- Consulta enviada -\n");
    }
    //---------------------------------------------------------------------------------------------------------------------------

   	//Recibo las respuestas
    //----------------------------------------------------------recibir--------------------------------------------------------
    i = sizeof dest;
	int byte_count;
    if(byte_count = recvfrom (sock, (char *) buf, sizeof(buf)-1, 0, (struct sockaddr*) &dest, (socklen_t *)&i ) < 0)
    {
        perror("- Error al recibir la respuesta -");
        exit(ERR_ENV);
    }
    else
    {
    	if(modoPrint)  	printf("- Respuesta recibida -\n");
    }
    //--------------------------------------------------------------------------------------------------------------------------
        
    dns_h = (struct DNS_HEADER*) buf;
 	
    lector = &buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION)];
    
    //Leemos e imprimimos las respuestas
   	//---------------------------------------------------------leerImprimir-----------------------------------------------------
	struct RES_RECORD resp[20], autor[20], adic[20];

	if(modoPrint)  	printf("\nNúmero de Respuestas [Rt]: %i\n", ntohs(dns_h->ancount));
    for(i=0; i<ntohs(dns_h->ancount); i++)
    {
    	resp[i].name = leerHost(lector, buf, &parar);
		lector = lector + parar;

		resp[i].resource = (struct R_DATA*)(lector);
		lector = lector + sizeof(struct R_DATA);
	
		tipo = ntohs(resp[i].resource->type);

        switch (tipo)
        {
        	case T_A:	//Si es una consulta de conversión de nombre
        	{
        		resp[i].rdata = (unsigned char*)malloc(ntohs(resp[i].resource->data_len));

	            for(j=0 ; j<ntohs(resp[i].resource->data_len) ; j++)
	            {
	                resp[i].rdata[j] = lector[j];
	            }
	 
	            resp[i].rdata[ntohs(resp[i].resource->data_len)] = '\0';
	            lector = lector + ntohs(resp[i].resource->data_len);

	            long *p;
           		p = (long*)resp[i].rdata;
            	sai.sin_addr.s_addr = (* p);

            	if(salidaIP == NULL)
        		{
        			salidaIP = malloc(ntohs(resp[i].resource->data_len));
        			strcpy(salidaIP,inet_ntoa(sai.sin_addr));
        		}

            	if(modoPrint)  	printf("[Rt%i]\tHost: %s\t Tipo de respuesta: A\t\tDirección IPv4: %s\n", i+1, resp[i].name, inet_ntoa(sai.sin_addr));
        		break;

        	}
        	case T_NS:	//Si se recibe una respuesta de tipo NS
        	{
        		resp[i].rdata = (unsigned char*)malloc(ntohs(resp[i].resource->data_len));

	            for(j=0 ; j<ntohs(resp[i].resource->data_len) ; j++)
	            {
	                resp[i].rdata[j] = lector[j];
	            }
	            
	            laux = &resp[i].rdata[0];
        		char * d_sv = leerHost(laux,buf,&parar);
        		
        		if(salidaAddr == NULL)
        		{
        			salidaAddr = malloc(strlen(d_sv));
        			strcpy(salidaAddr,d_sv);
        		}
	 
	            resp[i].rdata[ntohs(resp[i].resource->data_len)] = '\0';
	            lector = lector + ntohs(resp[i].resource->data_len);
        		if(modoPrint)  	printf("[Rt%i]\tHost: %s\t\t Tipo de respuesta: NS\t\tServidor: %s\n", i+1, host, d_sv);
        		break;
        	}
        	case T_CNAME:	//Si se recibe una respuesta de tipo CNAME
        	{
        		resp[i].rdata = (unsigned char*)malloc(ntohs(resp[i].resource->data_len));

	            for(j=0 ; j<ntohs(resp[i].resource->data_len) ; j++)
	            {
	                resp[i].rdata[j] = lector[j];
	            }
	 
	            resp[i].rdata[ntohs(resp[i].resource->data_len)] = '\0';
	            lector = lector + ntohs(resp[i].resource->data_len);
        		if(modoPrint)  	printf("[Rt%i]\tTipo CNAME\n", i+1);
        		break;
        	}
        	case T_MX:	//Si es una consulta de servidor de correo electrónico
        	{
        		resp[i].rdata = (unsigned char*)malloc(ntohs(resp[i].resource->data_len));

        		for(j=0 ; j<ntohs(resp[i].resource->data_len) ; j++)
	            {
	               	resp[i].rdata[j] = lector[j];
	                           
	            }
	            
        		laux = &resp[i].rdata[2];
        		char * d_mail = leerHost(laux,buf,&parar);
        			 
	            lector = lector + ntohs(resp[i].resource->data_len);

            	if(modoPrint)  	printf("[Rt%i]\tHost: %s\t Tipo de respuesta: MX\t\tServidor Mail: %s [%i]\n", i+1, resp[i].name, d_mail,resp[i].rdata[1]);            
	            break;

        	}
        	case T_AAAA:	//Si se recibe una respuesta de tipo AAAA
        	{
        		resp[i].rdata = (unsigned char*)malloc(ntohs(resp[i].resource->data_len));

	            for(j=0 ; j<ntohs(resp[i].resource->data_len) ; j++)
	            {
	                resp[i].rdata[j] = lector[j];
	            }
	 
	            resp[i].rdata[ntohs(resp[i].resource->data_len)] = '\0';
	            lector = lector + ntohs(resp[i].resource->data_len);
        		if(modoPrint)  	printf("[Rt%i]\tTipo AAAA\n", i+1);
        		break;
        	}
        	case T_LOC: //Si es una consulta de servidor de Localización Geográfica
        	{
        		resp[i].rdata = (unsigned char*)malloc(ntohs(resp[i].resource->data_len));

        		for(j=0 ; j<ntohs(resp[i].resource->data_len) ; j++)
	            {
	               	resp[i].rdata[j] = lector[j];                          
	            }

        		laux = &resp[i].rdata[1];
        		char * d_mail = leerHost(laux,buf,&parar);
        		
	            resp[i].rdata[ntohs(resp[i].resource->data_len)] = '\0';
	            lector = lector + ntohs(resp[i].resource->data_len);

	            int n_hex[2];
	            pasarAHexa(resp[i].rdata[1],n_hex);
	            int t = convertirAMetros(n_hex);
	            pasarAHexa(resp[i].rdata[2],n_hex);
	            int ph = convertirAMetros(n_hex);
	            pasarAHexa(resp[i].rdata[3],n_hex);
	            int pv = convertirAMetros(n_hex);

	            unsigned int n_lat[4] = {resp[i].rdata[4],resp[i].rdata[5],resp[i].rdata[6],resp[i].rdata[7]};
	            unsigned int lat = getLatLonAlt(n_lat);
	            
	            unsigned int n_lon[4] = {resp[i].rdata[8],resp[i].rdata[9],resp[i].rdata[10],resp[i].rdata[11]};
	            unsigned int lon = getLatLonAlt(n_lon);

	            unsigned int n_alt[4] = {resp[i].rdata[12],resp[i].rdata[13],resp[i].rdata[14],resp[i].rdata[15]};
	            unsigned int alt = getLatLonAlt(n_alt);
	           
	            char * respuesta = loc_ntoa(lat,lon,alt);
	            
	            if(modoPrint)  	printf("[Rt%i]\tHost: %s\t Tipo de respuesta: LOC\n", i+1, resp[i].name);
	            if(modoPrint)  	printf("\tDatos:\tVersión: %i\n\t\tTamaño: %i m\n\t\tPresición Horizontal: %i m\n\t\tPresición Vertical: %i m\n", resp[i].rdata[0], t,ph,pv);
	            if(modoPrint)  	printf("\t\t%s\n", respuesta);
	            break;

        	}
        	default:	//Si se recibe una respuesta de tipo Desconocido
        	{
        		if(modoPrint)  	printf("[Rt%i]\tNada que mostrar. Se recibió un tipo de respuesta no soportado.\n",i+1);
        	};
        }   
    }
    printf("\n");
    
    //Leemos Respuestas Autoritativas
    if(modoPrint)  	printf("Número de Respuestas Autoritativas [At]: %i\n", ntohs(dns_h->nscount));
    for(i=0; i<ntohs(dns_h->nscount); i++)
    {
        autor[i].name = leerHost(lector, buf, &parar);
        lector += parar;
 
        autor[i].resource = (struct R_DATA*)(lector);
        lector += sizeof(struct R_DATA);
 		
        autor[i].rdata = leerHost(lector, buf, &parar);
        lector += parar;

        if(salidaAddr == NULL)
        {
        	salidaAddr = malloc(ntohs(autor[i].resource->data_len));
        	strcpy(salidaAddr,autor[i].rdata);
        }

    	if(modoPrint)  	printf("[At%i]\tHost: %s\t\tTipo: NS\t\t Alias: %s\n",i,autor[i].name ,autor[i].rdata);
    	   
    }
    if(modoPrint)  	printf("\n");
 
    //Leemos Registros Adicionales (Método)
    if(modoPrint)  	printf("Número de Respuestas Adicionales [Ad]: %i\n", ntohs(dns_h->arcount));
    for(i=0; i<ntohs(dns_h->arcount); i++)
    {
        adic[i].name = leerHost(lector, buf, &parar);
        lector += parar;
 
        adic[i].resource = (struct R_DATA*)(lector);
        lector += sizeof(struct R_DATA);
 
        tipo = ntohs(adic[i].resource->type);

        switch (tipo)
        {
        	case T_A:		//Si se recibe una respuesta Adicional de tipo A
        	{
        		adic[i].rdata = (unsigned char*)malloc(ntohs(adic[i].resource->data_len));

	            for(j=0 ; j<ntohs(adic[i].resource->data_len) ; j++)
	            {
	                adic[i].rdata[j] = lector[j];
	            }
	 
	            adic[i].rdata[ntohs(adic[i].resource->data_len)] = '\0';
	            lector = lector + ntohs(adic[i].resource->data_len);

	            long *p;
           		p = (long*)adic[i].rdata;
            	sai.sin_addr.s_addr = (* p);

            	if(salidaIP == NULL)
        		{
        			salidaIP = malloc(ntohs(adic[i].resource->data_len));
        			strcpy(salidaIP,inet_ntoa(sai.sin_addr));
        		}

            	if(modoPrint)  	printf("[Ad%i]\tHost: %s\t Tipo de respuesta: A\t\tDirección IPv4: %s\n", i+1, adic[i].name, inet_ntoa(sai.sin_addr));
        		break;
        	}
        	case T_NS:	//Si se recibe una respuesta Adicional de tipo NS
        	{
        		adic[i].rdata = (unsigned char*)malloc(ntohs(adic[i].resource->data_len));

	            for(j=0 ; j<ntohs(adic[i].resource->data_len) ; j++)
	            {
	                adic[i].rdata[j] = lector[j];
	            }
	            
	            laux = &adic[i].rdata[0];
        		char * d_sv = leerHost(laux,buf,&parar);
        		
        		if(salidaAddr == NULL)
        		{
        			salidaAddr = malloc(strlen(d_sv));
        			strcpy(salidaAddr,d_sv);
        		}
	 
	            adic[i].rdata[ntohs(adic[i].resource->data_len)] = '\0';
	            lector = lector + ntohs(adic[i].resource->data_len);
        		if(modoPrint)  	printf("[Ad%i]\tHost: %s\t Tipo de respuesta: NS\t\tServidor: %s\n", i+1, adic[i].name, d_sv);
        		break;
        	}
        	case T_CNAME:	//Si se recibe una respuesta Adicional de tipo NS
        	{
        		adic[i].rdata = (unsigned char*)malloc(ntohs(adic[i].resource->data_len));

	            for(j=0 ; j<ntohs(adic[i].resource->data_len) ; j++)
	            {
	                adic[i].rdata[j] = lector[j];
	            }
	 
	            adic[i].rdata[ntohs(adic[i].resource->data_len)] = '\0';
	            lector = lector + ntohs(adic[i].resource->data_len);
        		if(modoPrint)  	printf("[Ad%i]\tTipo CNAME\n", i+1);
        		break;
        	}
			case T_MX:	//Si es una consulta de servidor de correo electrónico
        	{

        		adic[i].rdata = (unsigned char*)malloc(ntohs(adic[i].resource->data_len));

        		for(j=0 ; j<ntohs(adic[i].resource->data_len) ; j++)
	            {
	               	adic[i].rdata[j] = lector[j];
	                           
	            }
	            laux = lector;
        		laux = &adic[i].rdata[2];
        		char * d_mail = leerHost(laux,buf,&parar);
        			 
	            lector = lector + ntohs(adic[i].resource->data_len);

            	if(modoPrint)  	printf("[Ad%i]\tHost: %s\t Tipo de respuesta: MX\t\tServidor Mail: %s [%i]\n", i+1, resp[i].name, d_mail,resp[i].rdata[1]);          
	            break;

        	}
        	case T_AAAA:	//Si se recibe una respuesta Adicional de tipo AAAA
        	{
        		adic[i].rdata = (unsigned char*)malloc(ntohs(adic[i].resource->data_len));

	            for(j=0 ; j<ntohs(adic[i].resource->data_len) ; j++)
	            {
	                adic[i].rdata[j] = lector[j];
	            }
	 
	            adic[i].rdata[ntohs(adic[i].resource->data_len)] = '\0';
	            lector = lector + ntohs(adic[i].resource->data_len);
        		if(modoPrint)  	printf("[Ad%i]\tTipo AAAA\n", i+1);
        		break;
        	}
        	case T_LOC: //Si es una consulta de servidor de Localización Geográfica
        	{
        		adic[i].rdata = (unsigned char*)malloc(ntohs(adic[i].resource->data_len));

        		for(j=0 ; j<ntohs(adic[i].resource->data_len) ; j++)
	            {
	               	adic[i].rdata[j] = lector[j];                          
	            }

	            laux = lector;
        		laux = &adic[i].rdata[1];
        		char * d_mail = leerHost(laux,buf,&parar);
        		
	            adic[i].rdata[ntohs(adic[i].resource->data_len)] = '\0';
	            lector = lector + ntohs(adic[i].resource->data_len);

	            int n_hex[2];
	            pasarAHexa(adic[i].rdata[1],n_hex);
	            int t = convertirAMetros(n_hex);
	            pasarAHexa(adic[i].rdata[2],n_hex);
	            int ph = convertirAMetros(n_hex);
	            pasarAHexa(adic[i].rdata[3],n_hex);
	            int pv = convertirAMetros(n_hex);

	            unsigned int n_lat[4] = {adic[i].rdata[4],adic[i].rdata[5],adic[i].rdata[6],adic[i].rdata[7]};
	            unsigned int lat = getLatLonAlt(n_lat);
	            
	            unsigned int n_lon[4] = {adic[i].rdata[8],adic[i].rdata[9],adic[i].rdata[10],adic[i].rdata[11]};
	            unsigned int lon = getLatLonAlt(n_lon);

	            unsigned int n_alt[4] = {adic[i].rdata[12],adic[i].rdata[13],adic[i].rdata[14],adic[i].rdata[15]};
	            unsigned int alt = getLatLonAlt(n_alt);

	            char * respuesta = loc_ntoa(lat,lon,alt);
	            
	            if(modoPrint)  	printf("[Ad%i]\tHost: %s\t Tipo de respuesta: LOC\n", i+1, adic[i].name);
	            if(modoPrint)  	printf("\tDatos:\tVersión: %i\n\t\tTamaño: %i m\n\t\tPresición Horizontal: %i m\n\t\tPresición Vertical: %i m\n", adic[i].rdata[0], t,ph,pv);
	            if(modoPrint)  	printf("\t\t%s\n", respuesta);
                break;
        	}
        	default:	//Si se recibe una respuesta Adicional de tipo Desconocido
        	{
        		if(modoPrint)  	printf("[Ad%i]\tNada que mostrar. Se recibió un tipo de respuesta no soportado.\n",i+1);
        	}
    	}
    }
    if(modoPrint)  	printf("\n");

    if(salidaIP!=NULL)
    {
    	return salidaIP;	//Priorizo las direcciones IPv4 antes que las direcciones de nombre
    }
    else
    {
    	return salidaAddr;
    }
}

/*
	Método getLatLonAlt

	Convierte un cuarteto de enteros (4 números Hexa en representación decimal)
	a repesentación decimal.
*/
unsigned int getLatLonAlt(unsigned int n_lla[])
{
	unsigned int aux = 0;
    for(int j=0;j<4;j++)
    {
    	aux = aux *256 + n_lla[j];
    	
    }
	return aux;
}  

/*
	Método convertirAMetros

	Transforma la dirección host a formato DNS.
	Convierte los '.' en la cantidad de caracteres hasta el próximo punto
	Ej:	IN 	>	cs.uns.edu.ar
		OUT >	2cs3uns3edu2ar0

*/
int convertirAMetros(int n_hex[]) 
{
    int salida = (n_hex[0]*pow(10,n_hex[1]))/100;
    
    return salida;
}


/*
	Método pasarAHexa

	Recibe un entero de dos cifras y devuelve una dupla
	de dichas cifras en Hexa.

*/
void pasarAHexa(int entero, int n_hex[]) 
{
	n_hex[0] = entero / 16;
	n_hex[1] = entero % 16; 
}

/*
	Método formatoDNS

	Transforma la dirección host a formato DNS.
	Convierte los '.' en la cantidad de caracteres hasta el próximo punto
	Ej: IN 	>	cs.uns.edu.ar
		OUT	>	2cs3uns3edu2ar0

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
            lock++;
        }
    }
    *dns++='\0';
}

/*
	Método LeerHost

	Lee los nombres de Host de formato DNS y los transforma a nombres de Host normales
	Ej: IN 	>	2cs3uns3edu2ar0
		OUT	>	cs.uns.edu.ar
 */
u_char* leerHost(unsigned char* _lec, char _buff[], int* _cont)
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
            offset = (*_lec)*256 + *(_lec+1) - 49152;
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


