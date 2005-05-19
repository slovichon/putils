/* $Id$ */

#include <sys/param.h>
#include <sys/lock.h>
#include <sys/nlist.h>
#include <sys/vnode.h>

#include <ufs/ufs/quota.h>

#define _KERNEL /* XXX */
#include <ufs/ufs/inode.h>
#undef _KERNEL

#include "pfiles.h"

struct nlist[] = {
	{ "udf_vnodeop_entries", NULL },
} vnops;

int
getattr_ufs(struct pfile *pf, void *v_data)
{
	struct inode *ip = (struct inode *)v_data;
	
	pf->pf_mode = ip->i_ffs_mode & ~IFMT;
	pf->pf_dev = ip->i_dev;
	pf->pf_ino = ip->i_number;
	pf->pf_uid = ip->i_ffs_uid;
	pf->pf_gid = ip->i_ffs_gid;
	pf->pf_rdev = (dev_t)ip->i_ffs_rdev;
	return (1);
}

int
getattr_isofs(struct pfile *pf, void *v_data)
{
        struct iso_node *ip = (struct iso_node *)v_data;

        pf->pf_mode = ip->inode.iso_mode & ALLPERMS;
        pf->pf_dev = ip->i_dev;
        pf->pf_ino = ip->i_number;
        pf->pf_uid = ip->inode.iso_uid;
        pf->pf_gid = ip->inode.iso_gid;
        pf->pf_rdev = ip->inode.iso_rdev;
	return (1);
}

#if 0
int
getattr_udf(struct pfile *pf, void *v_data)
{
	struct udf_node *n = (struct udf_node *)v_data;

        pf->pf_mode = ip->inode.iso_mode & ALLPERMS;
        pf->pf_dev = ip->i_dev;
        pf->pf_ino = ip->i_number;
        pf->pf_uid = ip->inode.iso_uid;
        pf->pf_gid = ip->inode.iso_gid;
        pf->pf_rdev = ip->inode.iso_rdev;

        vap->va_fsid = node->i_dev;
        vap->va_fileid = node->hash_id;
        vap->va_mode = udf_permtomode(node);
        vap->va_nlink = letoh16(fentry->link_cnt);
        /*
         * XXX The spec says that -1 is valid for uid/gid and indicates an
         * invalid uid/gid.  How should this be represented?
         */
        vap->va_uid = (letoh32(fentry->uid) == -1) ? 0 : letoh32(fentry->uid);
        vap->va_gid = (letoh32(fentry->gid) == -1) ? 0 : letoh32(fentry->gid);
        udf_timetotimespec(&fentry->atime, &vap->va_atime);
        udf_timetotimespec(&fentry->mtime, &vap->va_mtime);
        vap->va_ctime = vap->va_mtime; /* XXX Stored as an Extended Attribute */
        vap->va_rdev = 0; /* XXX */
}
#endif

int
getattr_msdos(struct pfile *pf, void *v_data)
{
	
}

int
getattr_nfs(struct pfile *pf, void *v_data)
{
	
}

int
getattr_ext2fs(struct pfile *pf, void *v_data)
{
	
}

int
getattr_xfs(struct pfile *pf, void *v_data)
{
	
}
