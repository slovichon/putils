/* $Id$ */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void h(int);

int
main(int argc, char *argv[])
{
	int pid;

	pid = fork();
	if (pid)
		printf("pid: %d addr: %p\n", pid, h);
	else {
		signal(SIGINT, h);
		sleep(120);
		printf("\nexiting\r");
	}
	exit(0);
}

void
h(int a)
{
	printf("recv sig\n");
}
