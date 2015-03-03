#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>



int get_tamanioarchivo(int fd) //funcion que devuelve el tamaño de un archivo (sacada deinternet)
{
	struct stat stat_struct;
	if(fstat(fd, &stat_struct) == -1){
		return(1);
	}
	return (int)stat_struct.st_size;
}
