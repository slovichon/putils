#include <stdio.h>

int
main(int argc, char *argv[])
{
	pid_t pid;
	while (*argv != NULL) {
		if (strncmp(*argv, _PATH_PROC "/") == 0)
			*argv += strlen(_PATH_PROC "/");
		pid = atoi(*argv);
		
	}
}
