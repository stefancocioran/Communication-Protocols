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

void tac(int fd)
{
	char buf[1024], chr[1];
	int r, w, count, i;

	lseek(fd, 0, SEEK_END);

	while (lseek(fd, 0, SEEK_CUR) > 1)
	{	
		lseek(fd, -2, SEEK_CUR);

		if (lseek(fd, 0, SEEK_CUR) <= 2)
			break;

		r = read(fd, chr, 1);

		if (r < 0)
			fatal("Eroare la citire");
		
		count = 0;

		while (chr[0] != '\n')
		{	
			buf[count++] = chr[0];

			if (lseek(fd, 0, SEEK_CUR) < 2)
				break;

			lseek(fd, -2, SEEK_CUR);
			r = read(fd, chr, 1);

			if (r < 0)
				fatal("Eroare la citire");

		}

		for (i = count - 1; i >= 0 && count > 0; i--)
		{
			w = write(1, &buf[i], 1);

			if (w < 0)
				fatal("Eroare la scriere");

		}

		printf("\n");
	}
	
	close(fd);

}

int main(int argc, char *argv[])
{
	
	int fd = open(argv[1], O_RDONLY);

	if (fd < 0)
		fatal("Nu pot deschide un fisier");
	
	tac(fd);

	close(fd);

	return 0;
}
