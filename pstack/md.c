/* $Id$ */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <i386/pcb.h>

#include <kvm.h>

#include "pstack.h"

unsigned long
getsp(kvm_t *kd, struct kinfo_proc2 *kip)
{
	struct user u;

	kvm_read(kd, kip->p_addr, &u, sizeof(u));
	return (u.u_pcb.pcb_esp);
}
