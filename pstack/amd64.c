/* $Id$ */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>

#include <machine/pcb.h>

#include <kvm.h>

#include "pstack.h"

void
getsp(kvm_t *kd, struct kinfo_proc2 *kp, unsigned long *addr)
{
	struct user u;

	*addr = 0UL;
	if (kvm_read(kd, kp->p_addr, &u, sizeof(u)) == -1)
		return (kvm_geterr(kd));
	*addr = (unsigned long)u.u_pcb.pcb_rsp;
}
