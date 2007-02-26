/**
 * \file        presenter/server/src/l4vfs_fprov/main.c
 * \brief       Replacement for traditional fileprovider of the presenter,
 *              which is using the l4vfs-api.
 *
 * \date        06/01/2006
 * \author      Steffen Liebergeld <s1010824@os.inf.tu-dresden.de>
 * 		based on the original fileprovider by 
 * 		Jens Syckor <js712688@inf.tu-dresden.de>*/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include "presenter_fprov-server.h"
#include <l4/names/libnames.h>
#include <l4/env/errno.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/log/l4log.h>
#include <l4/sys/syscalls.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#ifdef DEBUG
static int _DEBUG = 1;
#else
static int _DEBUG = 0;
#endif

typedef char l4_page_t[L4_PAGESIZE];

static l4_page_t map_page __attribute__ ((aligned(L4_PAGESIZE)));
static l4_page_t io_buf __attribute__ ((aligned(L4_PAGESIZE)));

char *presenter_fprov_name;
char LOG_tag[9] = "preslxfp";

int32_t
presenter_fprov_open_component(CORBA_Object _dice_corba_obj,
                                  const char* pathname,
                                  CORBA_int flags,
                                  CORBA_Server_Environment *_dice_corba_env)
{
	int fd;
	
	LOGd(_DEBUG,"try to open %s with flags: %d",pathname,flags);

  /**
   * HACK: Filedescriptor 0 is not allowed in the presenter.
   */
	if ((fd = open(pathname,flags)) == 0)
    fd = open(pathname,flags);

	if (fd < 0) { 
        	LOG("error, file id smaller than 0...");		
	}
	else { 
		LOGd(_DEBUG,"success, file id %u",fd);
	}

	return fd;
	
}



l4_int32_t presenter_fprov_close_component(CORBA_Object _dice_corba_obj,
                                   l4_int32_t fd,
                                   CORBA_Server_Environment *_dice_corba_env)
{
     LOGd(_DEBUG,"close fd %d",fd);	
     return close(fd);

}

int presenter_fprov_read_component(CORBA_Object _dice_corba_obj,
                                   CORBA_int fd, 
                                   const l4_threadid_t *dm,
                                   CORBA_long count,
                                   l4dm_dataspace_t *ds,
                                   CORBA_Server_Environment *_dice_corba_env)
{
	unsigned int offset;
	off_t fsize_rounded;
	int current_fread, fread = 0;
	int error;
 	l4_addr_t fpage_addr;
	l4_size_t fpage_size;

	if (count <= 0) {
		LOG("error, count <= 0");
		return -1;
	}

	LOGd(_DEBUG,"called with file id %d, count: %d",fd, (int) count);
	
	fsize_rounded = l4_round_page(count);

	/* create new dataspace */
	error = l4dm_mem_open(*dm,fsize_rounded,0,0,"test",ds);
	
	if (error < 0) {
		l4dm_close(ds);
		close(fd);
		LOG("error creating a ds...");
		return -L4_ENOMEM;
	}
	
	/* clear map_page */	
	memset(map_page, 0, sizeof(map_page));
	memset(io_buf,0,sizeof(io_buf));

	/* map in dataspace each page */
	for (offset=0; offset<count; offset+=L4_PAGESIZE) {

	        if ((current_fread = read((int)fd, io_buf, sizeof(io_buf))) == -1) {
            LOG("Error reading file ");
            LOG("errno %d",errno);
            l4dm_close(ds);
            close(fd);
            return -L4_EIO;
	        }

		if (current_fread == 0) 
			break;

	        if (current_fread < sizeof(io_buf)) {
			 /* clear rest of page */
			 memset(io_buf+current_fread, 0, sizeof(io_buf)-current_fread);
		}
	
		/* clear memory */
		l4_fpage_unmap(l4_fpage((l4_umword_t)map_page, L4_LOG2_PAGESIZE,L4_FPAGE_RW, L4_FPAGE_MAP),
		L4_FP_FLUSH_PAGE|L4_FP_ALL_SPACES);

		/* map page of dataspace */
		error = l4dm_map_pages(ds,offset,L4_PAGESIZE,
      (l4_addr_t)map_page,L4_LOG2_PAGESIZE,0,L4DM_RW,
      &fpage_addr,&fpage_size);

		if (error < 0) { 	
			LOG("Error %d requesting offset %08x at ds_manager",error,offset);
			l4dm_close(ds);
			close(fd);
			return -L4_EINVAL;
		}
		
		/* copy file contents */
		memcpy(map_page, io_buf, L4_PAGESIZE);

		fread+=current_fread;
	}

	if ((error = l4dm_transfer(ds, *_dice_corba_obj))) {
		LOG("Error transfering dataspace owners");
		l4dm_close(ds);
		return -L4_EINVAL;
		
	}

	return fread;	
	
}

CORBA_int
presenter_fprov_lseek_component(CORBA_Object _dice_corba_obj,
                                CORBA_int fildes,
                                CORBA_int offset,
                                CORBA_int whence,
                                CORBA_Server_Environment *_dice_corba_env) {

	return lseek(fildes,offset,whence);	
}


long
presenter_fprov_write_component(CORBA_Object _dice_corba_obj,
                                CORBA_int fd,
                                const l4dm_dataspace_t *ds,
                                CORBA_unsigned_long count,
                                CORBA_Server_Environment *_dice_corba_env) {

	l4_page_t write_buf __attribute__ ((aligned(L4_PAGESIZE)));
	l4_addr_t content_addr;
	l4_size_t ds_size;
	l4_int32_t error;
	int result;
	unsigned int offset;
	char *content;

	LOGd(_DEBUG,"write called with fd %d, count %d",fd,(int)count);

	if (count <= 0) return 0;

	/* calculate size of dataspace */
	l4dm_mem_size((l4dm_dataspace_t *)ds,&ds_size);

	error=l4rm_attach((l4dm_dataspace_t *)ds,ds_size,0,L4DM_RW,(void *)&content_addr);

	if (error!=0) {
		LOG("error while attaching dataspace");
		return -1;
	}

	content = (char *)content_addr;

	for (offset=0;offset<ds_size;offset+=L4_PAGESIZE) {
		if (count<L4_PAGESIZE+offset) {
			memcpy(write_buf,&content[offset],count-offset);

			/* write into file */
			result = write(fd,write_buf,count-offset);

			if (result <= 0)
        LOG("error while writing into file, errno %d",errno);

			break;

		}	
		else {
			memcpy(write_buf,&content[offset],L4_PAGESIZE);

			/* write into file */
			result = write(fd,write_buf,L4_PAGESIZE);

			if (result <= 0)
        LOG("error while writing into file, errno %d",errno);
		}

	}	

	return 0;
}

int main(int argc,char **argv) {

	if (argc > 2) {
		LOG("Presenter file provider\n"
      "Usage:\n"
      "  NAME FOR NAME-SERVER");
    return -1;
	}
	if (argc==1) presenter_fprov_name = "PRESENTER_FPROV";
	else presenter_fprov_name = argv[1];

	LOGd(_DEBUG,"starting presenter_fprov server");

	/* try to register at nameserver */	
	if (names_register(presenter_fprov_name)==0) {
		LOG("Failed to register presenter_fprov\n");
	        return -2;
	} 
	else {
		LOGd(_DEBUG,"%s registered at nameserver...",presenter_fprov_name);
	}
	
	/* go into server mode */
 	presenter_fprov_server_loop(0); 
	
	return 0;
	
}
