/* $Id$ */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>

#include <kvm.h>

extern kvm_t *kd;

unsigned long getsp(struct kinfo_proc2 *);
