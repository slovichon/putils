/* $Id$ */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>

#if defined(__i386__)
#include <i386/pcb.h>
#elif defined(__amd64__)
#include <amd64/pcb.h>
#endif

#include <kvm.h>

#include "pstack.h"

char *
getsp(kvm_t *kd, struct kinfo_proc2 *kip, unsigned long *addr)
{
	struct user u;

	*addr = 0UL;
	if (kvm_read(kd, kip->p_addr, &u, sizeof(u)) == -1)
		return (kvm_geterr(kd));

#if defined(__i386__)
	*addr = (unsigned long)u.u_pcb.pcb_esp;
#elif defined(__amd64)
	*addr = (unsigned long)u.u_pcb.pcb_usersp;
#else
	return ("unsupported architecture");
#endif
	return (*addr ? NULL : "unknown error");
}
