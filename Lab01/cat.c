#include <stdio.h>
#include <unistd.h> 
#include <fcntl.h> 
#include <errno.h> 
#include <string.h>
#include <stdlib.h>

void fatal(char * mesaj_eroare)
{
	perror(mesaj_eroare);
	exit(0);
}

void cat(int fd)
{
	char buf[1024];
	
	lseek(fd, 0, SEEK_SET);

	int r, w;

	while ((r = read(fd, buf, sizeof(buf))))
	{
		if (r < 0)
			fatal("Eroare la citire");

		w = write(1, buf, r);
		if (w < 0)
			fatal("Eroare la scriere");
	}

}

int main(int argc, char *argv[])
{
	
	int fd = open(argv[1], O_RDONLY);

	if (fd < 0)
		fatal("Nu pot deschide un fisier");
	
	cat(fd);

	close(fd);

	return 0;
}
