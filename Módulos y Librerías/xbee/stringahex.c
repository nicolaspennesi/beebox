#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

//Convierte un string a su valor en hexa
int stringahex ( char* texto, char* hexa )
{
   char *ptr = texto;
   unsigned int byte;
   size_t i;
   for ( i = 0; i < sizeof hexa; ++i )
   {
      if ( sscanf(ptr, "%2x", &byte) != 1 ) //Buscar otra opción
      {
         break;
      }
      hexa[i] = byte;
      ptr += 2;
   }


/*Para ver salida*/

/*
   size_t j;
   for ( j = i, i = 0; i < j; ++i )
   {
      printf("hexa[%lu] = %X\n", (long unsigned)i, (unsigned)hexa[i]&0xFF);
   }
*/

   return 0;
}

