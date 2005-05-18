/* $Id$ */

#include <sys/param.h>
#include <sys/lock.h>
#include <sys/vnode.h>

#include <ufs/ufs/quota.h>

#define _KERNEL /* XXX */
#include <ufs/ufs/inode.h>
#undef _KERNEL

#include "pfiles.h"

void
vattr_ffs(struct pfile *pf, struct vnode *vp)
{
	struct inode *ip = VTOI(vp);
	
	pf->pf_mode = ip->i_ffs_mode & ~IFMT;
	pf->pf_dev = ip->i_dev;
	pf->pf_ino = ip->i_number;
	pf->pf_uid = ip->i_ffs_uid;
	pf->pf_gid = ip->i_ffs_gid;
	pf->pf_rdev = (dev_t)ip->i_ffs_rdev;
}
