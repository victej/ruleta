/*Practica manejo de archivos
 *Serie Fibonacci
 *Por Hernan Salgado Suarez
 */


#include <stdio.h>

int main(int argc, char **argv)
{
	FILE *salida;
	int indice;
	unsigned int arreglo[10];
	arreglo[0] = 0;
	arreglo[1] = 1;
	
	salida = fopen("salida.bin","w");
	
		fwrite(&arreglo[0],sizeof(unsigned int),1,salida);
		fwrite(&arreglo[1],sizeof(unsigned int),1,salida);
		
		for (indice = 2; indice < 10; indice++)
			{
			arreglo [indice]= arreglo[indice - 1] + arreglo[indice - 2];
			write(&arreglo[indice],sizeof(unsigned int),1,salida);
			}
		
	fclose(salida);
	
	return 0;
}

