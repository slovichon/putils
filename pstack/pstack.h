/* $Id$ */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>

#include <kvm.h>

char *getsp(kvm_t *, struct kinfo_proc2 *, unsigned long *);
