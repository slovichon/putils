/* $Id$ */

#include <sys/param.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/ucred.h>
#include <sys/vnode.h>

#include <isofs/cd9660/cd9660_node.h>
#include <isofs/udf/ecma167-udf.h>
#include <isofs/udf/udf.h>

#include <ufs/ufs/quota.h>
#define _KERNEL /* XXX */
#include <ufs/ufs/inode.h>
#undef _KERNEL

#include <err.h>
#include <kvm.h>
#include <nlist.h>

#include "pfiles.h"

int getattr_ext2fs(struct pfile *, void *);
int getattr_ffs(struct pfile *, void *);
int getattr_isofs(struct pfile *, void *);
int getattr_mfs(struct pfile *, void *);
int getattr_msdosfs(struct pfile *, void *);
int getattr_nfs(struct pfile *, void *);
int getattr_ntfs(struct pfile *, void *);
int getattr_udf(struct pfile *, void *);
int getattr_xfs(struct pfile *, void *);

struct nlist vnops_nl[] = {
#define X_CD9660_VNODEOP_ENTRIES 0
	{ "cd9660_vnodeop_entries", 0L },
#define X_EXT2FS_VNODEOP_ENTRIES 1
	{ "ext2fs_vnodeop_entries", 0L },
#define X_FFS_VNODEOP_ENTRIES 2
	{ "ffs_vnodeop_entries", 0L },
#define X_MFS_VNODEOP_ENTRIES 3
	{ "mfs_vnodeop_entries", 0L },
#define X_MSDOSFS_VNODEOP_ENTRIES 4
	{ "msdosfs_vnodeop_entries", 0L },
#define X_NFSV2_VNODEOP_ENTRIES 5
	{ "nfsv2_vnodeop_entries", 0L },
#define X_NTFS_VNODEOP_ENTRIES 6
	{ "ntfs_vnodeop_entries", 0L },
#define X_UDF_VNODEOP_ENTRIES 7
	{ "udf_vnodeop_entries", 0L },
#define X_XFS_VNODEOP_ENTRIES 8
	{ "xfs_vnodeop_entries", 0L },
};

struct vnop_dispatch_tab {
	int	  vdt_nl;
	int	(*vdt_getattr)(struct pfile *, void *);
} vnops_tab[] = {
	{ X_CD9660_VNODEOP_ENTRIES,	getattr_isofs },
	{ X_EXT2FS_VNODEOP_ENTRIES,	getattr_ext2fs },
	{ X_FFS_VNODEOP_ENTRIES,	getattr_ffs },
	{ X_MFS_VNODEOP_ENTRIES,	getattr_mfs },
	{ X_MSDOSFS_VNODEOP_ENTRIES,	getattr_msdosfs },
	{ X_NFSV2_VNODEOP_ENTRIES,	getattr_nfs },
	{ X_NTFS_VNODEOP_ENTRIES,	getattr_ntfs },
	{ X_UDF_VNODEOP_ENTRIES,	getattr_udf },
	{ X_XFS_VNODEOP_ENTRIES,	getattr_xfs }
};

int
getattr_ffs(struct pfile *pf, void *v_data)
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

static mode_t
udf_getmode(struct udf_node *n)
{
	struct file_entry fentry;
	u_int32_t perm;
	u_int16_t flags;
	mode_t mode;

	if (kvm_read(kd, (u_long)n->fentry, &fentry,
	    sizeof(fentry)) != sizeof(fentry))
		errx(1, "kvm_read: %s", kvm_geterr(kd));
	perm = letoh32(fentry.perm);
	flags = letoh16(fentry.icbtag.flags);

	mode = perm & UDF_FENTRY_PERM_USER_MASK;
	mode |= ((perm & UDF_FENTRY_PERM_GRP_MASK) >> 2);
	mode |= ((perm & UDF_FENTRY_PERM_OWNER_MASK) >> 4);
	mode |= ((flags & UDF_ICB_TAG_FLAGS_STICKY) << 4);
	mode |= ((flags & UDF_ICB_TAG_FLAGS_SETGID) << 6);
	mode |= ((flags & UDF_ICB_TAG_FLAGS_SETUID) << 8);
	return (mode);
}

int
getattr_udf(struct pfile *pf, void *v_data)
{
	struct udf_node *n = (struct udf_node *)v_data;

        pf->pf_mode = udf_getmode(n);
        pf->pf_dev = n->i_dev;
        pf->pf_ino = n->hash_id;
        pf->pf_uid =
        pf->pf_gid =
        pf->pf_rdev = 0;  /* XXX */

#if 0
        vap->va_mode = udf_permtomode(node);
        /*
         * XXX The spec says that -1 is valid for uid/gid and indicates an
         * invalid uid/gid.  How should this be represented?
         */
        vap->va_uid = (letoh32(fentry->uid) == -1) ? 0 : letoh32(fentry->uid);
        vap->va_gid = (letoh32(fentry->gid) == -1) ? 0 : letoh32(fentry->gid);
#endif
	return (0);
}

int
getattr_msdosfs(struct pfile *pf, void *v_data)
{

	return (0);
}

int
getattr_nfs(struct pfile *pf, void *v_data)
{

	return (0);
}

int
getattr_ext2fs(struct pfile *pf, void *v_data)
{

	return (0);
}

int
getattr_xfs(struct pfile *pf, void *v_data)
{
	return (0);
}

int
getattr_mfs(struct pfile *pf, void *v_data)
{
	return (0);
}

int
getattr_ntfs(struct pfile *pf, void *v_data)
{
	return (0);
}
