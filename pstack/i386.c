/* $Id$ */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <machine/pcb.h>

#include <kvm.h>
#include "pstack.h"

unsigned long
getsp(struct kinfo_proc2 *kp)
{
	struct user u;

	if (kvm_read(kd, kp->p_addr, &u, sizeof(u)) == -1) {
		warnx("kvm_read: %s", kvm_geterr(kd));
		return (0);
	}
	return ((unsigned long)u.u_pcb.pcb_esp);
}
