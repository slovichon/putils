/* $Id$ */

#include <sys/param.h>
#include <sys/sysctl.h>

#include <kvm.h>

unsigned long getsp(kvm_t *, struct kinfo_proc2 *);
