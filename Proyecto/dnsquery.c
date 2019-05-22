#include <stdio.h>
#include <stdlib.h>

/*
 * Procedimiento ayuda
 * 
 * Se utiliza cuando se ingresan por parametro datos incorrectos o cuando 
 * se invoca al programa con el parametro -h.
 * Muestra por consola el modo de invocacion del programa, incluyendo las
 * distintas opciones del programa.
 * 
 */
void ayuda()
{
		printf("MODO DE USO: dnsquery consulta @servidor[:puerto] [-a | -mx | -loc] [-r | -t] [-h]\n");
		printf("OPCION\t\t\tDESCRIPCION\n");
		
		printf("consulta\t\t\tEs la consulta que se desea resolver.\n");
		printf(":puerto\t\t\tServidor DNS con el que se resolvera la consulta. Si no se determina se usa el servidor DNS por defecto.\n");
		printf("-a\t\tExplora los puertos contenidos dentro de rango lp:hp (0<lp<hp<1024).\n");
		printf("-mx\t\t\tMuestra esta ayuda.\n");
		printf("-loc\t\t\tMuestra esta ayuda.\n");
		printf("-r\t\t\tMuestra esta ayuda.\n");
		printf("-t\t\t\tMuestra esta ayuda.\n");
		printf("-h\t\t\tMuestra esta ayuda.\n");
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
	//
	if(argc==1)
	{
		//un solo parametro (apuertos)
		ayuda();
	}
	else
	{	
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
	}
}
