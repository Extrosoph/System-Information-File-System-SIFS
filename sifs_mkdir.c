#include <stdio.h>
#include <string.h>
#include "sifs.h"


int main(int argcount, char *argvalue[])
{
	char *volumename = argvalue[1];
	char *dirname = argvalue[2];
	if(SIFS_mkdir(volumename, dirname) != 0)
	{
		SIFS_perror(argvalue[0]);
		exit(EXIT_FAILURE);
	}
	return EXIT_SUCCESS;
}

