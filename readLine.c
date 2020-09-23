#include <stdlib.h>
#include <unistd.h>

ssize_t readLine(int fildes, void *buf, size_t nbyte)
{
	size_t i = 0;
	ssize_t n = 1;
	char c = ' ';

	//Lê um caractere
	n = read(fildes, &c, 1);

	/* lê caractere a cacter até ao '\n' */
	while((n > 0) && (c != '\n')) {
		// Adiciona ao buffer
		((char*) buf)[i] = c;
		i++;
		//Lê um caractere
		n = read(fildes, &c, 1);
	}

	if (n>0) ((char*) buf)[i] = c;

	// se deu erro na leitura retorna esse mesmo erro
	if(n < 0)
		return n;
	return i++;
}
