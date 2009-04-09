
/*
 * \brief   DDEUSB usbcam capturing and dope output
 * \date    2009-04-07
 * \author  Dirk Vogt <dvogt@os.inf.tu-dresden.de>
 * 
 * It's starting to get really a mess!
 * Mixing kernel and userland code is somehow ugly
*/




/*
 * This file is part of the DDEUSB package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */




#include <linux/types.h>
#include <linux/kthread.h>
#include <linux/videodev.h>
#include <l4/dope/dopelib.h>
#include <linux/fs.h>
#include <linux/videodev.h>
#include <l4/dope/vscreen.h>
#include <l4/log/l4log.h> 
#include <media/v4l2-common.h>

#include "libv4lconvert/libv4lconvert.h"

#define MAXVIDEVS 16
#define BPP 24


long   app_id;                  /* DOpE application id      */
static int run[256];
static int running;


struct my_video_device {
	struct inode inode;
	struct file file;
	struct video_device * vdev;
	int open;
};

static void (*callback)(int , int);
void devchange_callback(int i, int chg);

static struct my_video_device videodev[MAXVIDEVS];

static DEFINE_MUTEX(vdev_lock);

static int devs=0;

static int vdev_open(int vdev_id) {
	int ret=-ENODEV;
	mutex_lock(&vdev_lock);
	LOG("OPEN_CALLED!!");	
	if (!videodev[vdev_id].open) {	
		if (videodev[vdev_id].vdev->fops->open)
			ret = videodev[vdev_id].vdev->fops->open(&videodev[vdev_id].file);
		else 
		{
			LOG("ERROR: no open");
		}
		videodev[vdev_id].open=1;
	}
	mutex_unlock(&vdev_lock);	
	if(! ret)
		return vdev_id;
	else
		return -ENODEV;
}

int read(int i,  void *buf, size_t nbyte)
{
	if(videodev[i].vdev->fops->read)
		return videodev[i].vdev->fops->read( &videodev[i].file, buf, nbyte, 0);
	else {
		LOG("ERROR: no read");
		return -1;
	}
}

int ioctl(int i, unsigned int cmd, void* arg)
{
		if(videodev[i].vdev->fops->ioctl)
			return videodev[i].vdev->fops->ioctl(&videodev[i].file, cmd, (unsigned int)arg);
		else {
			LOG("ERROR: no ioctl");
			return -1;
		}
}

/*********************************************/



int video_register_device(struct video_device * vdev, int i, int j)
{
	mutex_lock(&vdev_lock);	
	videodev[devs].vdev = vdev;
	if (callback)
		callback(devs,1);
	
	devs++;
	mutex_unlock(&vdev_lock);
	return 0;	
}

void video_device_release_empty(struct video_device *vdev)
{
}

int current_uid()
{
	return 0;
}

void put_unaligned_le16(u16 val, void *p)
{
        *((__le16 *)p) = cpu_to_le16(val);
}


void put_unaligned_le32(u32 val, void *p)
{
        *((__le32 *)p) = cpu_to_le32(val);
}


struct video_device * __must_check video_device_alloc(void) {
	return (struct video_device *) kmalloc (sizeof (struct video_device), GFP_KERNEL);
	}

/* this release function frees the vdev pointer */
void video_device_release(struct video_device *vdev) {
	kfree(vdev);
}



/*********************************************/

void video_unregister_device(struct video_device * vdev)
{
        mutex_lock(&vdev_lock); 
	int i;
	for(i=0 ; i< MAXVIDEVS;i++)
		if(videodev[i].vdev==vdev)
			break;
	if(videodev[i-1].vdev==vdev)
	{
		videodev[i-1].vdev;
		if (callback)
			callback(i,0);
	}

        mutex_unlock(&vdev_lock);       
}

static int close(int vdev_id)
{
	int ret=-1;
	mutex_lock(&vdev_lock); 
        videodev[vdev_id].open = 0;
		if (videodev[vdev_id].vdev->fops->release)
			ret = videodev[vdev_id].vdev->fops->release(&videodev[vdev_id].file);
        mutex_unlock(&vdev_lock); 
	return ret;
}

static int register_videv_change_callback(void (*func)(int , int )) {
		callback=func;
		int i;
		for(i=0 ; i< MAXVIDEVS;i++) {
			if(videodev[i].vdev) {

 				callback(i,1);
				
			}
		}
	return 0;
}

struct video_device* video_devdata(struct file *file)
{
	struct my_video_device *vdev = container_of(file,  struct my_video_device, file) ;
	return vdev->vdev;
}
		


int get_brightness_adj(unsigned char *image, long size, int *brightness) {
  long i, tot = 0;
  for (i=0;i<size*3;i++)
    tot += image[i];
  *brightness = (128 - tot/(size*3))/3;
  return !((tot/(size*3)) >= 126 && (tot/(size*3)) <= 130);
}


static void exit_callback(dope_event *e, void *arg) {

	while(running) schedule();
	dope_deinit_app(app_id);
	dope_deinit();
	
}

static void press_callback(dope_event *e,void *arg);



static int init_gfx(void * data)
{
 
 	if (dope_init()) return -1;
 
 	/* register DOpE-application */
 	app_id = dope_init_app("USBcam");
 
 	/* create menu window with one button for each effect */
 	dope_cmd(app_id,"mainwin = new Window()");
 	dope_cmd(app_id,"mg = new Grid()");
 	dope_cmd(app_id,"mainwin.set(-content mg)");
 	dope_cmd(app_id,"mainwin.set(-w 100 -h 120)");
 
 
 	dope_cmd(app_id, "exit = new Button(-text Exit)");
 	dope_cmd(app_id, "mg.place(exit, -column 1 -row 99)");
 	dope_bind(app_id, "exit", "commit", exit_callback, (void *)0);
 
 	dope_cmd(app_id,"mainwin.open()");
 	register_videv_change_callback(devchange_callback);
 	dope_eventloop(app_id);
	return 0;
	
}

void start_gui()
{
		kthread_run(init_gfx,NULL,"usbcam_gui");
}





void devchange_callback(int i, int chg){
	char strbuf[128];
	if(chg){
		dope_cmdf(app_id, "b%d = new Button()",i);
		dope_cmdf(app_id, "b%d.set(-text \"videodev %d\")",i,i);
		dope_cmdf(app_id, "mg.place(b%d,-column 1 -row %d)",i,i);
		dope_cmd(app_id,"mainwin.open()");
		dope_cmd(app_id,"mainwin.set(-content mg)");

		sprintf(strbuf,"b%d",i);
		dope_bind(app_id,strbuf,"press", press_callback, (void *)i);
	}
	else
	{
	        dope_cmdf(app_id, "mg.remove(b%d)",i);
	}
}





int capture_v4l1(int fd, int id) {
	char strbuf[128];
	char *scr_adr =NULL ;
	char* buffer;
	struct video_capability cap;
  	struct video_window win;
  	struct video_picture vpic;
	
	if (ioctl(fd, VIDIOCGCAP, &cap) < 0) {
    		printk("And also not a V4L1 dev?\n");
	        close(fd);
			running--;
	        return 1;
    }


	if (ioctl(fd, VIDIOCGWIN,  &win) < 0) {
		close(fd);
		running--;
		return 1;
	}

	if (ioctl(fd, VIDIOCGPICT, &vpic) < 0) {
		close(fd);
		running--;
		return 1;
	}

	printk("depth: %d palette:%d\n",vpic.depth, vpic.palette);


#if 0

	vpic.palette=VIDEO_PALETTE_RGB32;
	vpic.depth=32;
	if(ioctl(fd, VIDIOCSPICT, &vpic) < 0) {
		printk("capture format not supported :(\n");
		close(fd);
		return -1;

	} 
#endif 


	dope_cmdf(app_id, "video_win%d = new Window()",id);
	dope_cmdf(app_id, "vscr%d=new VScreen()",id );
	dope_cmdf(app_id, "vscr%d.setmode(%d,%d,\"%s\")",id ,win.width, win.height, "RGBA32" );
	dope_cmdf(app_id, "video_win%d.set( -background off -content vscr%d)",id,id );
	dope_cmdf(app_id, "video_win%d.open()",id);

	sprintf(strbuf,"vscr%d",id);
	scr_adr = vscr_get_fb(app_id, strbuf);

	char * buf = kmalloc(win.width * win.height *3* sizeof(char), GFP_KERNEL );


	/* loop */
	while(run[id]) 
	{
		read(fd,buf,win.width * win.height * 3* sizeof(char));
		int i;
		for(i=0;i<win.width * win.height;i++){
			scr_adr[i*4] = 0xff;

			scr_adr[i*4+1] = buf[i*3+2];
			scr_adr[i*4+2] = buf[i*3+1];		
			scr_adr[i*4+3] = buf[i*3];
		}
		
		dope_cmdf(app_id, "vscr%d.refresh()",id  );

	}

	close(fd); 
	dope_cmdf(app_id,"video_win%d.close()",id);
	return 0;
}




static int capture_thread (void * data)
{
	char strbuf[128];

	int id = (int) data;
	char *scr_adr =NULL ;
	char* buffer;
	running++;
  	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
	struct v4l2_format dest_fmt;
    unsigned int min;


	int fd = vdev_open(id);

	if (fd < 0) {
		LOG("OPEN FAILED");
		running--;

		return 1;
	}

	if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) 
	{
    		LOG("not a video4linux2 device?\n");
			running--;
	        return capture_v4l1(fd,id);
    }


	if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
			LOG("Device does not support readwrite :o(");
			close(fd);running--;
	        return 1;
	}

	

    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = 320; 
    fmt.fmt.pix.height      = 240;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;


	dest_fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    dest_fmt.fmt.pix.width       = 320; 
    dest_fmt.fmt.pix.height      = 240;
	dest_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
    dest_fmt.fmt.pix.field       = V4L2_FIELD_NONE;

	int bpp=8;

	if (ioctl (fd, VIDIOC_S_FMT, &fmt))
	{
			LOG("IOCTL FAILED :o(");
			close(fd);
	        return 1;
	}

	min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
            fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
                fmt.fmt.pix.sizeimage = min;

	
	LOG("VIDEO PLAYBACK -- w: %d, h: %d, pixelformat: %c%c%c%c", dest_fmt.fmt.pix.width,
	                                                       dest_fmt.fmt.pix.height,
														  ((char *)&fmt.fmt.pix.pixelformat)[0],
														  ((char *)&fmt.fmt.pix.pixelformat)[1],
														  ((char *)&fmt.fmt.pix.pixelformat)[2],
														  ((char *)&fmt.fmt.pix.pixelformat)[3]);

    dope_cmdf(app_id, "video_win%d = new Window()",id);
	dope_cmdf(app_id, "vscr%d=new VScreen()",id );
	dope_cmdf(app_id, "vscr%d.setmode(%d,%d,\"%s\")",id ,fmt.fmt.pix.width,fmt.fmt.pix.height , "YUV420" );
	dope_cmdf(app_id, "video_win%d.set( -background off -content vscr%d)",id,id );
	dope_cmdf(app_id, "video_win%d.open()",id);

	sprintf(strbuf,"vscr%d",id);
	scr_adr = vscr_get_fb(app_id, strbuf);
	
	char * buf = kmalloc(fmt.fmt.pix.sizeimage, GFP_KERNEL );
	
	struct v4lconvert_data * conv = v4lconvert_create(fd);
	

	/* loop */
	while(run[id]) 
	{
		int ret= read(fd,buf,fmt.fmt.pix.sizeimage);
		if(ret<0) {
			printk("read failed : %d \n", ret);
			run[id]=0;
			break;
		}
		
		v4lconvert_convert(conv,&fmt,  &dest_fmt, buf, fmt.fmt.pix.sizeimage,scr_adr,dest_fmt.fmt.pix.width* dest_fmt.fmt.pix.height*bpp);

		dope_cmdf(app_id, "vscr%d.refresh()",id  );
	

	}
	dope_cmdf(app_id,"video_win%d.close()",id);
	dope_cmdf(app_id, "b%d.set(-state 0)", id);
	v4lconvert_destroy(conv);
    close(fd); 
	running--;
	return 0;

}


static void press_callback(dope_event *e,void *arg) {
	int id = (int)arg;
	if(run[id]) {
		dope_cmdf(app_id, "b%d.set(-state 0)", id);
		run[id]=0;
	} else {	
		dope_cmdf(app_id, "b%d.set(-state 1)", id);
		run[id]=1;		
		kthread_run(capture_thread,(void*)id,"capture_%d",id);
	}
}

