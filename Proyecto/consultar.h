/*
	Redes de Computadoras - 2019
	Proyecto: Mini-Cliente de Consultas de Servidores DNS

	consultar.h

	Interface del módulo consultar.c.

	Autores:	- Agra Federico (94186)
				- Loza Carlos 	(94399)

*/

#ifndef CONSULTAR_H_INCLUDED
#define CONSULTAR_H_INCLUDED

char * consultar(unsigned char*,unsigned char*, int, int, int, int);
char* substring(const char*, int, int);

#endif // CONSULTAR_H_INCLUDED