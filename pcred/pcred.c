/* $Id$ */

#include <sys/param.h>

#include <ctype.h>
#include <stdio.h>
#include <unistd.h>

#include "pathnames.h"

void readproc(FILE *);
void handlestate(char *, int);

int
main(int argc, char *argv[])
{
	char *s, fil[MAXPATHLEN];
	int isnum;
	size_t siz;
	FILE *fp;

	while (*++argv != NULL) {
		isnum = 1;
		for (s = *argv; *s != '\0'; s++)
			if (!isdigit(*s)) {
				isnum = 0;
				break;
			}
		if (isnum) {
			strlcpy(fil, _PATH_PROC, sizeof(fil));
			strlcat(fil, "/", sizeof(fil));
			strlcat(fil, *argv, sizeof(fil));
		} else
			/* XXX: check return value */
			strlcpy(fil, *argv, sizeof(fil));
		strlcat(fil, _RPATH_CRED, sizeof(fil));

		if ((fp = fopen(fil, "r")) == NULL) {
			warn("cannot examine %s", *argv);
			continue;
		}

		readproc(fp);
		fclose(fp);
	}
}

#define ST_CMD 1
#define ST_PID 2
#define ST_PPID 3
#define ST_PGID 4
#define ST_SID 5
#define ST_DEV 6
#define ST_FLAGS 7
#define ST_START 8
#define ST_USERTM 9
#define ST_SYSTM 10
#define ST_WCHAN 11
#define ST_EUID 12
#define ST_GID 13

struct lbuf {
	int pos, max;
	char *buf;
};

void  lbuf_init(struct lbuf **);
void  lbuf_append(struct lbuf *, char);
char *lbuf_get(struct lbuf *);
void  lbuf_free(struct lbuf **);
void  lbuf_reset(struct lbuf *lb);

void
lbuf_init(struct lbuf **lb)
{
	if ((*lb = malloc(sizeof(**lb))) == NULL)
		err(1, "lbuf_init");
	(*lb)->pos = (*lb)->max = -1;
	(*lb)->buf = NULL;
}

void
lbuf_append(struct lbuf *lb, char ch)
{
	if (++lb->pos >= lb->max) {
		lb->max += 30;
		if ((lb->buf = realloc(lb->buf, lb->max)) == NULL)
			err(1, "lbuf_append");
	}
	lb->buf[lb->pos] = ch;
}

char *
lbuf_get(struct lbuf *lb)
{
	return lb->buf;
}

void
lbuf_free(struct lbuf **lb)
{
	free((*lb)->buf);
	free(*lb);
	*lb = NULL;
}

void
lbuf_reset(struct lbuf *lb)
{
	lb->pos = -1;
}

void
readproc(FILE *fp)
{
	int state, ch;
	char *p;
	struct lbuf *lb;

	lbuf_init(&lb);

	state = ST_CMD;
	ch = 1;
	while (ch != EOF) {
		ch = fgetc(fp);
		switch (ch) {
		case ' ':  /* FALLTHROUGH */
		case '\n': /* FALLTHROUGH */
		case EOF:
			lbuf_append(lb, '\0');
			handlestate(lbuf_get(lb), state++);
			lbuf_reset(lb);
			break;
		default:
			lbuf_append(lb, ch);
			break;
		}
	}
}

void
handlestate(char *arg, int state)
{
	char *p;
	int t;

	switch (state) {
	case ST_PID:
		printf("%s:", arg);
		break;
	case ST_GID:
		printf("\tgroups: ");
		for (p = strtok(arg, ","); p != NULL; ) {
			/* Don't repeat effective gid. */
#define print t
			print = 0;
			if (p == arg)
				print = 1;
			else if (strcmp(p, arg) != 0)
				print = 1;
			if (print)
				printf("%s", p);
			p = strtok((char *)NULL, ",");
			if (print && p != NULL)
				printf(" ");
#undef print
		}
		printf("\n");
		break;
	}
}

/*

19593:  e/r/suid=204336  e/r/sgid=2004
	groups: 33548 44332 2004

	1   2   3    4    5   6		  7     8	     9		10       11    12   13
	cmd pid ppid pgid sid major,minor flags start,ustart user,uuser sys,usys wchan euid gid,gid,...

	crypto 7 0 0 0 -1,-1 noflags 1086910508,30000 0,0 0,0 crypto_wait 0, 0,0
	xterm 7107 1 9184 0 -1,-1 noflags 1087066946,610000 1,117187 0,726562 select 1000, 45,1000,0,201
	XFree86 7234 11339 7234 14869 -1,-1 ctty 1086910526,850000 10644,898437 1401,718750 select 1000, 1000,1000
	less 7380 25635 30571 25635 5,6 ctty 1087346856,620000 0,0 0,0 ttyin 1000, 1000,1000,0,201
	kmthread 8 0 0 0 -1,-1 noflags 1086910508,30000 0,0 0,0 kmalloc 0, 0,0
	getty 8479 1 8479 8479 12,2 ctty,sldr 1086910516,350000 0,0 0,7812 ttyin 0, 0,0
	bash 8519 3493 8519 8519 5,10 ctty,sldr 1087066948,630000 0,7812 0,7812 ttyin 1000, 1000,1000,0,201
	sendmail 8847 1 8847 8847 -1,-1 sldr 1086910516,450000 4,304687 7,929687 select 0, 0,0
	usb0 9 0 0 0 -1,-1 noflags 1086910508,30000 0,0 0,78125 usbevt 0, 0,0
	sh 9544 1 12452 0 -1,-1 noflags 1087346966,670000 0,7812 0,0 pause 1000, 1000,1000,0,201
	cat 10448 3854 10448 3854 5,11 ctty 1087595649,850000 0,0 0,0 nochan 1000, 1000,1000,0,201
	cat 10448 3854 10448 3854 5,11 ctty 1087595649,850000 0,0 0,0 nochan 1000, 1000,1000,0,201
*/
