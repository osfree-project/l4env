/*!
 * \file   generic_fprov/examples/hostfs/linux_fs_com.c
 * \brief  file system adaption
 *
 * \date   07/28/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#include <oskit/io/blkio.h>
#include <oskit/io/absio.h>
#include <oskit/fs/filesystem.h>
#include <oskit/fs/dir.h>
#include <oskit/fs/openfile.h>
#include <oskit/fs/linux.h>
#include <oskit/diskpart/diskpart.h>
#include <oskit/principal.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <oskit/c/assert.h>
#include <string.h>

#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include "fs.h"

/* Maximum number of partitions to handle */
#define MAX_PARTS 30

static oskit_filesystem_t *fs_rootfs;	/* Our root filesystem */
oskit_dir_t *fs_rootdir;		/* Our root directory */
oskit_fsnamespace_t *fs_namespace;	/* Our filesystem namespace */

/* Type converters, so our callers dont need to know about oskit things */
extern inline oskit_absio_t* f2oskit(fs_file_t *f);
extern inline fs_file_t *    oskit2f(oskit_absio_t *a);

extern inline oskit_absio_t* f2oskit(fs_file_t *f) { return (oskit_absio_t*)f;}
extern inline fs_file_t *    oskit2f(oskit_absio_t *a){return (fs_file_t*)a;}

/* Identity of current client process. */
static oskit_principal_t *cur_principal;

/* A helper function for OSKit to determine the current context */
oskit_error_t oskit_get_call_context(const struct oskit_guid *iid,
				     void **out_if){
    if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	memcmp(iid, &oskit_principal_iid, sizeof(*iid)) == 0) {
	*out_if = cur_principal;
	oskit_principal_addref(cur_principal);
	return 0;
    }

    *out_if = 0;
    return OSKIT_E_NOINTERFACE;
}


/* Initialize the filesystem. If partname="disk", the disk is read as
 * superfloppy, ie unpartitioned */
int fs_do_init(const char*diskname, const char*partname){
	oskit_blkio_t *disk;
	oskit_blkio_t *part;
	oskit_identity_t id;
	oskit_error_t err;

        oskit_osenv_t *osenv;


	LOGl("Starting, diskname=%s, partname=%s", diskname, partname);

        osenv = oskit_osenv_create_default();
        oskit_register(&oskit_osenv_iid, (void *) osenv);

        // create (empty) service-databases for oskit device driver support
        oskit_dev_init(osenv);
        oskit_linux_init_osenv(osenv);
	start_clock();				/* so gettimeofday works */
	
	// start_blk_devices();
	oskit_linux_init_ide();
	LOGl("initialized ide\n");
	oskit_dev_probe();
	LOGl("Probed devices");

	printf(">>>Initializing filesystem...\n");
	if((err = fs_linux_init())!=0){
	    LOG_Error("fs_linux_init(): %s", l4env_strerror(err));
	    return -L4_EIO;
	}

	printf(">>>Establishing client identity\n");
	id.uid = 0;
	id.gid = 0;
	id.ngroups = 0;
	id.groups = 0;
	if((err = oskit_principal_create(&id, &cur_principal))!=0){
	    LOG_Error("oskit_principal_create(): %s", l4env_strerror(err));
	    return -L4_ENOMEM;
	}

	printf(">>>Opening the disk %s\n", diskname);
	if((err = oskit_linux_block_open(diskname,
					 OSKIT_DEV_OPEN_READ,
					 &disk))!=0){
	    LOG_Error("oskit_linux_block_open(): %s", l4env_strerror(err));
	    return -L4_EIO;
	}

	if(strcmp(partname, "disk")){
	    int numparts;
	    diskpart_t part_array[MAX_PARTS];

	    printf(">>>Reading partition table and looking for partition %s\n",
		   partname);
	    numparts = diskpart_blkio_get_partition(disk, part_array,
						    MAX_PARTS);
	    if (numparts == 0){
		LOG_Error("diskpart_blkio_get_partition() "
			  "returned no partitions");
		goto e_disk;
	    }
	    LOGl("found %d partitions", numparts);
	    if (diskpart_blkio_lookup_bsd_string(part_array, partname,
						 disk, &part) == 0){
		LOG_Error("diskpart_blkio_lookup_bsd_string() could not "
			  "find partition %s", partname);
		goto e_disk;
	    }
	} else {
	    printf(">>>Assuming no partitions\n");
	    part = disk;
	    oskit_blkio_addref(part);
	}

	/* Don't need the disk anymore, the partition has a ref. */
	oskit_blkio_release(disk);
	disk = 0;

	printf(">>>Mounting partition %s RDONLY\n", partname);
	if((err = fs_linux_mount(part, OSKIT_FS_RDONLY, &fs_rootfs))!=0){
	    LOG_Error("fs_linux_mount(): %s", l4env_strerror(err));
	    goto e_part;
	}

	/* Don't need the part anymore, the filesystem has a ref. */
	oskit_blkio_release(part);
	part = 0;

	printf(">>>Getting a handle to the root dir\n");
	if((err = oskit_filesystem_getroot(fs_rootfs, &fs_rootdir))!=0){
	    LOG_Error("oskit_filesystem_getroot(): %s", l4env_strerror(err));
	    goto e_fs_rootfs;
	}

	printf(">>>Creating the file system space\n");
	if((err = oskit_create_fsnamespace(fs_rootdir, fs_rootdir,
					   &fs_namespace))!=0){
	    LOG_Error("oskit_create_fsnamespace(): %s", l4env_strerror(err));
	    goto e_fs_rootdir;
	}
	LOGl("Initialized fs subsystem");
	return 0;

  e_fs_rootdir:
	printf(">>>Releasing rootdir\n");
	assert(oskit_dir_release(fs_rootdir) == 0);
  e_fs_rootfs:
	printf(">>>Unmounting %s\n", partname);
	assert(oskit_filesystem_release(fs_rootfs) == 0);
  e_part:
	if(part) assert(oskit_blkio_release(part)==0);
  e_disk:
	if(disk) assert(oskit_blkio_release(disk)==0);
	return -L4_EIO;
}

int fs_do_done(){
    printf(">>> Stopping...\n");
    oskit_fsnamespace_release(fs_namespace);
    printf(">>>Releasing rootdir\n");
    assert(oskit_dir_release(fs_rootdir) == 0);
    printf(">>>Unmounting\n");
    assert(oskit_filesystem_release(fs_rootfs) == 0);
    return 0;
}

fs_file_t* fs_do_open(const char*filename, long long *size){
    oskit_file_t *f;
    void *addr;
    oskit_absio_t *a;
    int err;

    printf(">>>Looking up file \"%s\"...\n", filename);
    if((err = oskit_fsnamespace_lookup(fs_namespace, filename,
				       FSLOOKUP_FOLLOW,
				       &f))!=0){
	LOG_Error("oskit_fsnamespace_lookup(): %s", l4env_strerror(err));
	return 0;
    }

    err = oskit_file_query(f, &oskit_absio_iid, &addr);
    a = addr;
    oskit_file_release(f);
    if(err){
	LOG_Error("Cannot access file: %s", l4env_strerror(err));
	return 0;
    }

    if((err = oskit_absio_getsize(a, size))!=0){
	LOG_Error("oskit_absio_getsize(): %s", l4env_strerror(err));
	oskit_absio_release(a);
	return 0;
    }
    return oskit2f(a);
}

int fs_do_close(struct fs_file_t *file){
    oskit_absio_release(f2oskit(file));
    return 0;
}

int fs_do_read(struct fs_file_t *file, long long offset,
	       char*addr, l4_size_t count){
    oskit_size_t len;
    int err;

    if((err = oskit_absio_read(f2oskit(file), addr, offset, count,
			       &len))!=0){
	LOG_Error("oskit_absio_read(count=%d, offset=%lld): %s",
		  count, offset, l4env_strerror(err));
	return -L4_EIO;
    }
    return (int)len;
}
