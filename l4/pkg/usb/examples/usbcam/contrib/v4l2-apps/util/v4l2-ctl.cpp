/*
    Copyright (C) 2003-2004  Kevin Thayer <nufan_wfk at yahoo dot com>

    Cleanup and VBI and audio in/out options, introduction in v4l-dvb,
    support for most new APIs since 2006.
    Copyright (C) 2004, 2006, 2007  Hans Verkuil <hverkuil@xs4all.nl>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <unistd.h>
#include <features.h>		/* Uses _GNU_SOURCE to define getsubopt in stdlib.h */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <dirent.h>
#include <math.h>
#include <sys/klog.h>

#include <linux/videodev2.h>

#include <list>
#include <vector>
#include <map>
#include <string>
#include <algorithm>

/* Short option list

   Please keep in alphabetical order.
   That makes it easier to see which short options are still free.

   In general the lower case is used to set something and the upper
   case is used to retrieve a setting. */
enum Option {
	OptGetSlicedVbiFormat = 'B',
	OptSetSlicedVbiFormat = 'b',
	OptGetCtrl = 'C',
	OptSetCtrl = 'c',
	OptSetDevice = 'd',
	OptGetDriverInfo = 'D',
	OptGetFreq = 'F',
	OptSetFreq = 'f',
	OptHelp = 'h',
	OptGetInput = 'I',
	OptSetInput = 'i',
	OptListCtrls = 'l',
	OptListCtrlsMenus = 'L',
	OptListOutputs = 'N',
	OptListInputs = 'n',
	OptGetOutput = 'O',
	OptSetOutput = 'o',
	OptGetStandard = 'S',
	OptSetStandard = 's',
	OptGetTuner = 'T',
	OptSetTuner = 't',
	OptGetVideoFormat = 'V',
	OptSetVideoFormat = 'v',

	OptGetSlicedVbiOutFormat = 128,
	OptGetOverlayFormat,
	OptGetOutputOverlayFormat,
	OptGetVbiFormat,
	OptGetVbiOutFormat,
	OptGetVideoOutFormat,
	OptSetSlicedVbiOutFormat,
	OptSetOutputOverlayFormat,
	OptSetOverlayFormat,
	//OptSetVbiFormat, TODO
	//OptSetVbiOutFormat, TODO
	OptSetVideoOutFormat,
	OptTryVideoOutFormat,
	OptTrySlicedVbiOutFormat,
	OptTrySlicedVbiFormat,
	OptTryVideoFormat,
	OptTryOutputOverlayFormat,
	OptTryOverlayFormat,
	//OptTryVbiFormat, TODO
	//OptTryVbiOutFormat, TODO
	OptAll,
	OptStreamOff,
	OptStreamOn,
	OptListStandards,
	OptListFormats,
	OptLogStatus,
	OptVerbose,
	OptSilent,
	OptGetSlicedVbiCap,
	OptGetSlicedVbiOutCap,
	OptGetFBuf,
	OptSetFBuf,
	OptGetCrop,
	OptSetCrop,
	OptGetOutputCrop,
	OptSetOutputCrop,
	OptGetOverlayCrop,
	OptSetOverlayCrop,
	OptGetOutputOverlayCrop,
	OptSetOutputOverlayCrop,
	OptGetAudioInput,
	OptSetAudioInput,
	OptGetAudioOutput,
	OptSetAudioOutput,
	OptListAudioOutputs,
	OptListAudioInputs,
	OptGetCropCap,
	OptGetOutputCropCap,
	OptGetOverlayCropCap,
	OptGetOutputOverlayCropCap,
	OptOverlay,
	OptGetJpegComp,
	OptSetJpegComp,
	OptListDevices,
	OptLast = 256
};

static char options[OptLast];

static int app_result;
static int verbose;

static unsigned capabilities;

typedef std::vector<struct v4l2_ext_control> ctrl_list;
static ctrl_list user_ctrls;
static ctrl_list mpeg_ctrls;

typedef std::map<std::string, unsigned> ctrl_strmap;
static ctrl_strmap ctrl_str2id;
typedef std::map<unsigned, std::string> ctrl_idmap;
static ctrl_idmap ctrl_id2str;

typedef std::list<std::string> ctrl_get_list;
static ctrl_get_list get_ctrls;

typedef std::map<std::string,std::string> ctrl_set_map;
static ctrl_set_map set_ctrls;

typedef std::vector<std::string> dev_vec;
typedef std::map<std::string, std::string> dev_map;

typedef struct {
	unsigned flag;
	const char *str;
} flag_def;

static const flag_def service_def[] = {
	{ V4L2_SLICED_TELETEXT_B,  "teletext" },
	{ V4L2_SLICED_VPS,         "vps" },
	{ V4L2_SLICED_CAPTION_525, "cc" },
	{ V4L2_SLICED_WSS_625,     "wss" },
	{ 0, NULL }
};

/* fmts specified */
#define FmtWidth		(1L<<0)
#define FmtHeight		(1L<<1)
#define FmtChromaKey		(1L<<2)
#define FmtGlobalAlpha		(1L<<3)
#define FmtPixelFormat		(1L<<4)
#define FmtLeft			(1L<<5)
#define FmtTop			(1L<<6)
#define FmtField		(1L<<7)

/* crop specified */
#define CropWidth		(1L<<0)
#define CropHeight		(1L<<1)
#define CropLeft		(1L<<2)
#define CropTop 		(1L<<3)

static struct option long_options[] = {
	{"list-audio-inputs", no_argument, 0, OptListAudioInputs},
	{"list-audio-outputs", no_argument, 0, OptListAudioOutputs},
	{"all", no_argument, 0, OptAll},
	{"device", required_argument, 0, OptSetDevice},
	{"get-fmt-video", no_argument, 0, OptGetVideoFormat},
	{"set-fmt-video", required_argument, 0, OptSetVideoFormat},
	{"try-fmt-video", required_argument, 0, OptTryVideoFormat},
	{"get-fmt-video-out", no_argument, 0, OptGetVideoOutFormat},
	{"set-fmt-video-out", required_argument, 0, OptSetVideoOutFormat},
	{"try-fmt-video-out", required_argument, 0, OptTryVideoOutFormat},
	{"help", no_argument, 0, OptHelp},
	{"get-output", no_argument, 0, OptGetOutput},
	{"set-output", required_argument, 0, OptSetOutput},
	{"list-outputs", no_argument, 0, OptListOutputs},
	{"list-inputs", no_argument, 0, OptListInputs},
	{"get-input", no_argument, 0, OptGetInput},
	{"set-input", required_argument, 0, OptSetInput},
	{"get-audio-input", no_argument, 0, OptGetAudioInput},
	{"set-audio-input", required_argument, 0, OptSetAudioInput},
	{"get-audio-output", no_argument, 0, OptGetAudioOutput},
	{"set-audio-output", required_argument, 0, OptSetAudioOutput},
	{"get-freq", no_argument, 0, OptGetFreq},
	{"set-freq", required_argument, 0, OptSetFreq},
	{"streamoff", no_argument, 0, OptStreamOff},
	{"streamon", no_argument, 0, OptStreamOn},
	{"list-standards", no_argument, 0, OptListStandards},
	{"list-formats", no_argument, 0, OptListFormats},
	{"get-standard", no_argument, 0, OptGetStandard},
	{"set-standard", required_argument, 0, OptSetStandard},
	{"info", no_argument, 0, OptGetDriverInfo},
	{"list-ctrls", no_argument, 0, OptListCtrls},
	{"list-ctrls-menus", no_argument, 0, OptListCtrlsMenus},
	{"set-ctrl", required_argument, 0, OptSetCtrl},
	{"get-ctrl", required_argument, 0, OptGetCtrl},
	{"get-tuner", no_argument, 0, OptGetTuner},
	{"set-tuner", required_argument, 0, OptSetTuner},
	{"verbose", no_argument, 0, OptVerbose},
	{"log-status", no_argument, 0, OptLogStatus},
	{"get-fmt-overlay", no_argument, 0, OptGetOverlayFormat},
	{"set-fmt-overlay", required_argument, 0, OptSetOverlayFormat},
	{"try-fmt-overlay", required_argument, 0, OptTryOverlayFormat},
	{"get-fmt-output-overlay", no_argument, 0, OptGetOutputOverlayFormat},
	{"set-fmt-output-overlay", required_argument, 0, OptSetOutputOverlayFormat},
	{"try-fmt-output-overlay", required_argument, 0, OptTryOutputOverlayFormat},
	{"get-fmt-sliced-vbi", no_argument, 0, OptGetSlicedVbiFormat},
	{"set-fmt-sliced-vbi", required_argument, 0, OptSetSlicedVbiFormat},
	{"try-fmt-sliced-vbi", required_argument, 0, OptTrySlicedVbiFormat},
	{"get-fmt-sliced-vbi-out", no_argument, 0, OptGetSlicedVbiOutFormat},
	{"set-fmt-sliced-vbi-out", required_argument, 0, OptSetSlicedVbiOutFormat},
	{"try-fmt-sliced-vbi-out", required_argument, 0, OptTrySlicedVbiOutFormat},
	{"get-fmt-vbi", no_argument, 0, OptGetVbiFormat},
	{"get-fmt-vbi-out", no_argument, 0, OptGetVbiOutFormat},
	{"get-sliced-vbi-cap", no_argument, 0, OptGetSlicedVbiCap},
	{"get-sliced-vbi-out-cap", no_argument, 0, OptGetSlicedVbiOutCap},
	{"get-fbuf", no_argument, 0, OptGetFBuf},
	{"set-fbuf", required_argument, 0, OptSetFBuf},
	{"get-cropcap", no_argument, 0, OptGetCropCap},
	{"get-crop", no_argument, 0, OptGetCrop},
	{"set-crop", required_argument, 0, OptSetCrop},
	{"get-cropcap-output", no_argument, 0, OptGetOutputCropCap},
	{"get-crop-output", no_argument, 0, OptGetOutputCrop},
	{"set-crop-output", required_argument, 0, OptSetOutputCrop},
	{"get-cropcap-overlay", no_argument, 0, OptGetOverlayCropCap},
	{"get-crop-overlay", no_argument, 0, OptGetOverlayCrop},
	{"set-crop-overlay", required_argument, 0, OptSetOverlayCrop},
	{"get-cropcap-output-overlay", no_argument, 0, OptGetOutputOverlayCropCap},
	{"get-crop-output-overlay", no_argument, 0, OptGetOutputOverlayCrop},
	{"set-crop-output-overlay", required_argument, 0, OptSetOutputOverlayCrop},
	{"get-jpeg-comp", no_argument, 0, OptGetJpegComp},
	{"set-jpeg-comp", required_argument, 0, OptSetJpegComp},
	{"overlay", required_argument, 0, OptOverlay},
	{"list-devices", no_argument, 0, OptListDevices},
	{0, 0, 0, 0}
};

static void usage(void)
{
	printf("Usage:\n");
	printf("Common options:\n"
	       "  --all              display all information available\n"
	       "  -C, --get-ctrl=<ctrl>[,<ctrl>...]\n"
	       "                     get the value of the controls [VIDIOC_G_EXT_CTRLS]\n"
	       "  -c, --set-ctrl=<ctrl>=<val>[,<ctrl>=<val>...]\n"
	       "                     set the controls to the values specified [VIDIOC_S_EXT_CTRLS]\n"
	       "  -D, --info         show driver info [VIDIOC_QUERYCAP]\n"
	       "  -d, --device=<dev> use device <dev> instead of /dev/video0\n"
	       "                     if <dev> is a single digit, then /dev/video<dev> is used\n"
	       "  -F, --get-freq     query the frequency [VIDIOC_G_FREQUENCY]\n"
	       "  -f, --set-freq=<freq>\n"
	       "                     set the frequency to <freq> MHz [VIDIOC_S_FREQUENCY]\n"
	       "  -h, --help         display this help message\n"
	       "  -I, --get-input    query the video input [VIDIOC_G_INPUT]\n"
	       "  -i, --set-input=<num>\n"
	       "                     set the video input to <num> [VIDIOC_S_INPUT]\n"
	       "  -l, --list-ctrls   display all controls and their values [VIDIOC_QUERYCTRL]\n"
	       "  -L, --list-ctrls-menus\n"
	       "		     display all controls, their values and the menus [VIDIOC_QUERYMENU]\n"
	       "  -N, --list-outputs display video outputs [VIDIOC_ENUMOUTPUT]\n"
	       "  -n, --list-inputs  display video inputs [VIDIOC_ENUMINPUT]\n"
	       "  -O, --get-output   query the video output [VIDIOC_G_OUTPUT]\n"
	       "  -o, --set-output=<num>\n"
	       "                     set the video output to <num> [VIDIOC_S_OUTPUT]\n"
	       "  -S, --get-standard\n"
	       "                     query the video standard [VIDIOC_G_STD]\n"
	       "  -s, --set-standard=<num>\n"
	       "                     set the video standard to <num> [VIDIOC_S_STD]\n"
	       "                     <num> can be a numerical v4l2_std value, or it can be one of:\n"
	       "                     pal-X (X = B/G/H/N/Nc/I/D/K/M/60) or just 'pal' (V4L2_STD_PAL)\n"
	       "                     ntsc-X (X = M/J/K) or just 'ntsc' (V4L2_STD_NTSC)\n"
	       "                     secam-X (X = B/G/H/D/K/L/Lc) or just 'secam' (V4L2_STD_SECAM)\n"
	       "  --list-standards   display supported video standards [VIDIOC_ENUMSTD]\n"
	       "  -T, --get-tuner    query the tuner settings [VIDIOC_G_TUNER]\n"
	       "  -t, --set-tuner=<mode>\n"
	       "                     set the audio mode of the tuner [VIDIOC_S_TUNER]\n"
	       "                     Possible values: mono, stereo, lang2, lang1, bilingual\n"
	       "  --list-formats     display supported video formats [VIDIOC_ENUM_FMT]\n"
	       "  -V, --get-fmt-video\n"
	       "     		     query the video capture format [VIDIOC_G_FMT]\n"
	       "  -v, --set-fmt-video=width=<w>,height=<h>,pixelformat=<f>\n"
	       "                     set the video capture format [VIDIOC_S_FMT]\n"
	       "                     pixelformat is either the format index as reported by\n"
	       "                     --list-formats, or the fourcc value as a string\n"
	       "  --list-devices     list all v4l devices\n"
	       "  --silent           only set the result code, do not print any messages\n"
	       "  --verbose          turn on verbose ioctl status reporting\n"
	       "\n");
	printf("Uncommon options:\n"
	       "  --try-fmt-video=width=<w>,height=<h>,pixelformat=<f>\n"
	       "                     try the video capture format [VIDIOC_TRY_FMT]\n"
	       "                     pixelformat is either the format index as reported by\n"
	       "                     --list-formats, or the fourcc value as a string\n"
	       "  --get-fmt-video-out\n"
	       "     		     query the video output format [VIDIOC_G_FMT]\n"
	       "  --set-fmt-video-out=width=<w>,height=<h>\n"
	       "                     set the video output format [VIDIOC_S_FMT]\n"
	       "  --try-fmt-video-out=width=<w>,height=<h>\n"
	       "                     try the video output format [VIDIOC_TRY_FMT]\n"
	       "  --get-fmt-overlay  query the video overlay format [VIDIOC_G_FMT]\n"
	       "  --get-fmt-output-overlay\n"
	       "     		     query the video output overlay format [VIDIOC_G_FMT]\n"
	       "  --set-fmt-overlay\n"
	       "  --try-fmt-overlay\n"
	       "  --set-fmt-output-overlay\n"
	       "  --try-fmt-output-overlay=chromakey=<key>,global_alpha=<alpha>,\n"
	       "                           top=<t>,left=<l>,width=<w>,height=<h>,field=<f>\n"
	       "     		     set/try the video or video output overlay format [VIDIOC_TRY_FMT]\n"
	       "                     <f> can be one of:\n"
	       "                     any, none, top, bottom, interlaced, seq_tb, seq_bt, alternate,\n"
	       "                     interlaced_tb, interlaced_bt\n"
	       "  --get-sliced-vbi-cap\n"
	       "		     query the sliced VBI capture capabilities [VIDIOC_G_SLICED_VBI_CAP]\n"
	       "  --get-sliced-vbi-out-cap\n"
	       "		     query the sliced VBI output capabilities [VIDIOC_G_SLICED_VBI_CAP]\n"
	       "  -B, --get-fmt-sliced-vbi\n"
	       "		     query the sliced VBI capture format [VIDIOC_G_FMT]\n"
	       "  --get-fmt-sliced-vbi-out\n"
	       "		     query the sliced VBI output format [VIDIOC_G_FMT]\n"
	       "  -b, --set-fmt-sliced-vbi\n"
	       "  --try-fmt-sliced-vbi\n"
	       "  --set-fmt-sliced-vbi-out\n"
	       "  --try-fmt-sliced-vbi-out=<mode>\n"
	       "                     (try to) set the sliced VBI capture/output format to <mode> [VIDIOC_S/TRY_FMT]\n"
	       "                     <mode> is a comma separated list of:\n"
	       "                     off:      turn off sliced VBI (cannot be combined with other modes)\n"
	       "                     teletext: teletext (PAL/SECAM)\n"
	       "                     cc:       closed caption (NTSC)\n"
	       "                     wss:      widescreen signal (PAL/SECAM)\n"
	       "                     vps:      VPS (PAL/SECAM)\n"
	       "  --get-fmt-vbi      query the VBI capture format [VIDIOC_G_FMT]\n"
	       "  --get-fmt-vbi-out  query the VBI output format [VIDIOC_G_FMT]\n"
	       "  --overlay=<on>     turn overlay on (1) or off (0) (VIDIOC_OVERLAY)\n"
	       "  --get-fbuf         query the overlay framebuffer data [VIDIOC_G_FBUF]\n"
	       "  --set-fbuf=chromakey=<0/1>,global_alpha=<0/1>,local_alpha=<0/1>,local_inv_alpha=<0/1>\n"
	       "		     set the overlay framebuffer [VIDIOC_S_FBUF]\n"
	       "  --get-cropcap      query the crop capabilities [VIDIOC_CROPCAP]\n"
	       "  --get-crop	     query the video capture crop window [VIDIOC_G_CROP]\n"
	       "  --set-crop=top=<x>,left=<y>,width=<w>,height=<h>\n"
	       "                     set the video capture crop window [VIDIOC_S_CROP]\n"
	       "  --get-cropcap-output\n"
	       "                     query the crop capabilities for video output [VIDIOC_CROPCAP]\n"
	       "  --get-crop-output  query the video output crop window [VIDIOC_G_CROP]\n"
	       "  --set-crop-output=top=<x>,left=<y>,width=<w>,height=<h>\n"
	       "                     set the video output crop window [VIDIOC_S_CROP]\n"
	       "  --get-cropcap-overlay\n"
	       "                     query the crop capabilities for video overlay [VIDIOC_CROPCAP]\n"
	       "  --get-crop-overlay query the video overlay crop window [VIDIOC_G_CROP]\n"
	       "  --set-crop-overlay=top=<x>,left=<y>,width=<w>,height=<h>\n"
	       "                     set the video overlay crop window [VIDIOC_S_CROP]\n"
	       "  --get-cropcap-output-overlay\n"
	       "                     query the crop capabilities for video output overlays [VIDIOC_CROPCAP]\n"
	       "  --get-crop-output-overlay\n"
	       "                     query the video output overlay crop window [VIDIOC_G_CROP]\n"
	       "  --set-crop-output-overlay=top=<x>,left=<y>,width=<w>,height=<h>\n"
	       "                     set the video output overlay crop window [VIDIOC_S_CROP]\n"
	       "  --get-jpeg-comp    query the JPEG compression [VIDIOC_G_JPEGCOMP]\n"
	       "  --set-jpeg-comp=quality=<q>,markers=<markers>,comment=<c>,app<n>=<a>\n"
	       "                     set the JPEG compression [VIDIOC_S_JPEGCOMP]\n"
	       "                     <n> is the app segment: 0-9 or a-f, <a> is the actual string.\n"
	       "                     <markers> is a colon separated list of:\n"
	       "                     dht:      Define Huffman Tables\n"
	       "                     dqt:      Define Quantization Tables\n"
	       "                     dri:      Define Restart Interval\n"
	       "  --set-audio-output=<num>\n"
	       "                     set the audio output to <num> [VIDIOC_S_AUDOUT]\n"
	       "  --get-audio-input  query the audio input [VIDIOC_G_AUDIO]\n"
	       "  --set-audio-input=<num>\n"
	       "                     set the audio input to <num> [VIDIOC_S_AUDIO]\n"
	       "  --get-audio-output query the audio output [VIDIOC_G_AUDOUT]\n"
	       "  --set-audio-output=<num>\n"
	       "                     set the audio output to <num> [VIDIOC_S_AUDOUT]\n"
	       "  --list-audio-outputs\n"
	       "                     display audio outputs [VIDIOC_ENUMAUDOUT]\n"
	       "  --list-audio-inputs\n"
	       "                     display audio inputs [VIDIOC_ENUMAUDIO]\n"
	       "\n");
	printf("Expert options:\n"
	       "  --streamoff        turn the stream off [VIDIOC_STREAMOFF]\n"
	       "  --streamon         turn the stream on [VIDIOC_STREAMOFF]\n"
	       "  --log-status       log the board status in the kernel log [VIDIOC_LOG_STATUS]\n");
	exit(0);
}

static std::string num2s(unsigned num)
{
	char buf[10];

	sprintf(buf, "%08x", num);
	return buf;
}

static std::string buftype2s(int type)
{
	switch (type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		return "Video Capture";
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		return "Video Output";
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
		return "Video Overlay";
	case V4L2_BUF_TYPE_VBI_CAPTURE:
		return "VBI Capture";
	case V4L2_BUF_TYPE_VBI_OUTPUT:
		return "VBI Output";
	case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
		return "Sliced VBI Capture";
	case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
		return "Sliced VBI Output";
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY:
		return "Video Output Overlay";
	case V4L2_BUF_TYPE_PRIVATE:
		return "Private";
	default:
		return "Unknown (" + num2s(type) + ")";
	}
}

static std::string fcc2s(unsigned int val)
{
	std::string s;

	s += val & 0xff;
	s += (val >> 8) & 0xff;
	s += (val >> 16) & 0xff;
	s += (val >> 24) & 0xff;
	return s;
}

static std::string field2s(int val)
{
	switch (val) {
	case V4L2_FIELD_ANY:
		return "Any";
	case V4L2_FIELD_NONE:
		return "None";
	case V4L2_FIELD_TOP:
		return "Top";
	case V4L2_FIELD_BOTTOM:
		return "Bottom";
	case V4L2_FIELD_INTERLACED:
		return "Interlaced";
	case V4L2_FIELD_SEQ_TB:
		return "Sequential Top-Bottom";
	case V4L2_FIELD_SEQ_BT:
		return "Sequential Bottom-Top";
	case V4L2_FIELD_ALTERNATE:
		return "Alternating";
	case V4L2_FIELD_INTERLACED_TB:
		return "Interlaced Top-Bottom";
	case V4L2_FIELD_INTERLACED_BT:
		return "Interlaced Bottom-Top";
	default:
		return "Unknown (" + num2s(val) + ")";
	}
}

static std::string colorspace2s(int val)
{
	switch (val) {
	case V4L2_COLORSPACE_SMPTE170M:
		return "Broadcast NTSC/PAL (SMPTE170M/ITU601)";
	case V4L2_COLORSPACE_SMPTE240M:
		return "1125-Line (US) HDTV (SMPTE240M)";
	case V4L2_COLORSPACE_REC709:
		return "HDTV and modern devices (ITU709)";
	case V4L2_COLORSPACE_BT878:
		return "Broken Bt878";
	case V4L2_COLORSPACE_470_SYSTEM_M:
		return "NTSC/M (ITU470/ITU601)";
	case V4L2_COLORSPACE_470_SYSTEM_BG:
		return "PAL/SECAM BG (ITU470/ITU601)";
	case V4L2_COLORSPACE_JPEG:
		return "JPEG (JFIF/ITU601)";
	case V4L2_COLORSPACE_SRGB:
		return "SRGB";
	default:
		return "Unknown (" + num2s(val) + ")";
	}
}

static std::string flags2s(unsigned val, const flag_def *def)
{
	std::string s;

	while (def->flag) {
		if (val & def->flag) {
			if (s.length()) s += " ";
			s += def->str;
		}
		def++;
	}
	return s;
}

static void print_sliced_vbi_cap(struct v4l2_sliced_vbi_cap &cap)
{
	printf("\tType           : %s\n", buftype2s(cap.type).c_str());
	printf("\tService Set    : %s\n",
			flags2s(cap.service_set, service_def).c_str());
	for (int i = 0; i < 24; i++) {
		printf("\tService Line %2d: %8s / %-8s\n", i,
				flags2s(cap.service_lines[0][i], service_def).c_str(),
				flags2s(cap.service_lines[1][i], service_def).c_str());
	}
}

static std::string name2var(unsigned char *name)
{
	std::string s;
	int add_underscore = 0;

	while (*name) {
		if (isalnum(*name)) {
			if (add_underscore)
				s += '_';
			add_underscore = 0;
			s += std::string(1, tolower(*name));
		}
		else if (s.length()) add_underscore = 1;
		name++;
	}
	return s;
}

static void print_qctrl(int fd, struct v4l2_queryctrl *queryctrl,
		struct v4l2_ext_control *ctrl, int show_menus)
{
	struct v4l2_querymenu qmenu = { 0 };
	std::string s = name2var(queryctrl->name);
	int i;

	qmenu.id = queryctrl->id;
	switch (queryctrl->type) {
	case V4L2_CTRL_TYPE_INTEGER:
		printf("%31s (int)  : min=%d max=%d step=%d default=%d value=%d",
				s.c_str(),
				queryctrl->minimum, queryctrl->maximum,
				queryctrl->step, queryctrl->default_value,
				ctrl->value);
		break;
	case V4L2_CTRL_TYPE_INTEGER64:
		printf("%31s (int64): value=%lld", s.c_str(), ctrl->value64);
		break;
	case V4L2_CTRL_TYPE_BOOLEAN:
		printf("%31s (bool) : default=%d value=%d",
				s.c_str(),
				queryctrl->default_value, ctrl->value);
		break;
	case V4L2_CTRL_TYPE_MENU:
		printf("%31s (menu) : min=%d max=%d default=%d value=%d",
				s.c_str(),
				queryctrl->minimum, queryctrl->maximum,
				queryctrl->default_value, ctrl->value);
		break;
	case V4L2_CTRL_TYPE_BUTTON:
		printf("%31s (button)\n", s.c_str());
		break;
	default: break;
	}
	if (queryctrl->flags) {
		const flag_def def[] = {
			{ V4L2_CTRL_FLAG_GRABBED,    "grabbed" },
			{ V4L2_CTRL_FLAG_READ_ONLY,  "read-only" },
			{ V4L2_CTRL_FLAG_UPDATE,     "update" },
			{ V4L2_CTRL_FLAG_INACTIVE,   "inactive" },
			{ V4L2_CTRL_FLAG_SLIDER,     "slider" },
			{ V4L2_CTRL_FLAG_WRITE_ONLY, "write-only" },
			{ 0, NULL }
		};
		printf(" flags=%s", flags2s(queryctrl->flags, def).c_str());
	}
	printf("\n");
	if (queryctrl->type == V4L2_CTRL_TYPE_MENU && show_menus) {
		for (i = 0; i <= queryctrl->maximum; i++) {
			qmenu.index = i;
			if (ioctl(fd, VIDIOC_QUERYMENU, &qmenu))
				continue;
			printf("\t\t\t\t%d: %s\n", i, qmenu.name);
		}
	}
}

static int print_control(int fd, struct v4l2_queryctrl &qctrl, int show_menus)
{
	struct v4l2_control ctrl = { 0 };
	struct v4l2_ext_control ext_ctrl = { 0 };
	struct v4l2_ext_controls ctrls = { 0 };

	if (qctrl.flags & V4L2_CTRL_FLAG_DISABLED)
		return 1;
	if (qctrl.type == V4L2_CTRL_TYPE_CTRL_CLASS) {
		printf("\n%s\n\n", qctrl.name);
		return 1;
	}
	ext_ctrl.id = qctrl.id;
	ctrls.ctrl_class = V4L2_CTRL_ID2CLASS(qctrl.id);
	ctrls.count = 1;
	ctrls.controls = &ext_ctrl;
	if (V4L2_CTRL_ID2CLASS(qctrl.id) != V4L2_CTRL_CLASS_USER &&
	    qctrl.id < V4L2_CID_PRIVATE_BASE) {
		if (ioctl(fd, VIDIOC_G_EXT_CTRLS, &ctrls)) {
			printf("error %d getting ext_ctrl %s\n",
					errno, qctrl.name);
			return 0;
		}
	}
	else {
		ctrl.id = qctrl.id;
		if (ioctl(fd, VIDIOC_G_CTRL, &ctrl)) {
			printf("error %d getting ctrl %s\n",
					errno, qctrl.name);
			return 0;
		}
		ext_ctrl.value = ctrl.value;
	}
	print_qctrl(fd, &qctrl, &ext_ctrl, show_menus);
	return 1;
}

static void list_controls(int fd, int show_menus)
{
	struct v4l2_queryctrl qctrl = { V4L2_CTRL_FLAG_NEXT_CTRL };
	int id;

	while (ioctl(fd, VIDIOC_QUERYCTRL, &qctrl) == 0) {
			print_control(fd, qctrl, show_menus);
		qctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
	}
	if (qctrl.id != V4L2_CTRL_FLAG_NEXT_CTRL)
		return;
	for (id = V4L2_CID_USER_BASE; id < V4L2_CID_LASTP1; id++) {
		qctrl.id = id;
		if (ioctl(fd, VIDIOC_QUERYCTRL, &qctrl) == 0)
			print_control(fd, qctrl, show_menus);
	}
	for (qctrl.id = V4L2_CID_PRIVATE_BASE;
			ioctl(fd, VIDIOC_QUERYCTRL, &qctrl) == 0; qctrl.id++) {
		print_control(fd, qctrl, show_menus);
	}
}

static void find_controls(int fd)
{
	struct v4l2_queryctrl qctrl = { V4L2_CTRL_FLAG_NEXT_CTRL };
	int id;

	while (ioctl(fd, VIDIOC_QUERYCTRL, &qctrl) == 0) {
		if (qctrl.type != V4L2_CTRL_TYPE_CTRL_CLASS &&
		    !(qctrl.flags & V4L2_CTRL_FLAG_DISABLED)) {
			ctrl_str2id[name2var(qctrl.name)] = qctrl.id;
			ctrl_id2str[qctrl.id] = name2var(qctrl.name);
		}
		qctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
	}
	if (qctrl.id != V4L2_CTRL_FLAG_NEXT_CTRL)
		return;
	for (id = V4L2_CID_USER_BASE; id < V4L2_CID_LASTP1; id++) {
		qctrl.id = id;
		if (ioctl(fd, VIDIOC_QUERYCTRL, &qctrl) == 0 &&
		    !(qctrl.flags & V4L2_CTRL_FLAG_DISABLED)) {
			ctrl_str2id[name2var(qctrl.name)] = qctrl.id;
			ctrl_id2str[qctrl.id] = name2var(qctrl.name);
		}
	}
	for (qctrl.id = V4L2_CID_PRIVATE_BASE;
			ioctl(fd, VIDIOC_QUERYCTRL, &qctrl) == 0; qctrl.id++) {
		if (!(qctrl.flags & V4L2_CTRL_FLAG_DISABLED)) {
			ctrl_str2id[name2var(qctrl.name)] = qctrl.id;
			ctrl_id2str[qctrl.id] = name2var(qctrl.name);
		}
	}
}

static std::string fbufcap2s(unsigned cap)
{
	std::string s;

	if (cap & V4L2_FBUF_CAP_EXTERNOVERLAY)
		s += "\t\t\tExtern Overlay\n";
	if (cap & V4L2_FBUF_CAP_CHROMAKEY)
		s += "\t\t\tChromakey\n";
	if (cap & V4L2_FBUF_CAP_GLOBAL_ALPHA)
		s += "\t\t\tGlobal Alpha\n";
	if (cap & V4L2_FBUF_CAP_LOCAL_ALPHA)
		s += "\t\t\tLocal Alpha\n";
	if (cap & V4L2_FBUF_CAP_LOCAL_INV_ALPHA)
		s += "\t\t\tLocal Inverted Alpha\n";
	if (cap & V4L2_FBUF_CAP_LIST_CLIPPING)
		s += "\t\t\tClipping List\n";
	if (cap & V4L2_FBUF_CAP_BITMAP_CLIPPING)
		s += "\t\t\tClipping Bitmap\n";
	if (s.empty()) s += "\t\t\t\n";
	return s;
}

static std::string fbufflags2s(unsigned fl)
{
	std::string s;

	if (fl & V4L2_FBUF_FLAG_PRIMARY)
		s += "\t\t\tPrimary Graphics Surface\n";
	if (fl & V4L2_FBUF_FLAG_OVERLAY)
		s += "\t\t\tOverlay Matches Capture/Output Size\n";
	if (fl & V4L2_FBUF_FLAG_CHROMAKEY)
		s += "\t\t\tChromakey\n";
	if (fl & V4L2_FBUF_FLAG_GLOBAL_ALPHA)
		s += "\t\t\tGlobal Alpha\n";
	if (fl & V4L2_FBUF_FLAG_LOCAL_ALPHA)
		s += "\t\t\tLocal Alpha\n";
	if (fl & V4L2_FBUF_FLAG_LOCAL_INV_ALPHA)
		s += "\t\t\tLocal Inverted Alpha\n";
	if (s.empty()) s += "\t\t\t\n";
	return s;
}

static void printfbuf(const struct v4l2_framebuffer &fb)
{
	int is_ext = fb.capability & V4L2_FBUF_CAP_EXTERNOVERLAY;

	printf("Framebuffer Format:\n");
	printf("\tCapability    : %s", fbufcap2s(fb.capability).c_str() + 3);
	printf("\tFlags         : %s", fbufflags2s(fb.flags).c_str() + 3);
	if (fb.base)
		printf("\tBase          : 0x%p\n", fb.base);
	printf("\tWidth         : %d\n", fb.fmt.width);
	printf("\tHeight        : %d\n", fb.fmt.height);
	printf("\tPixel Format  : '%s'\n", fcc2s(fb.fmt.pixelformat).c_str());
	if (!is_ext) {
		printf("\tBytes per Line: %d\n", fb.fmt.bytesperline);
		printf("\tSize image    : %d\n", fb.fmt.sizeimage);
		printf("\tColorspace    : %s\n", colorspace2s(fb.fmt.colorspace).c_str());
		if (fb.fmt.priv)
			printf("\tCustom Info   : %08x\n", fb.fmt.priv);
	}
}

static std::string markers2s(unsigned markers)
{
	std::string s;

	if (markers & V4L2_JPEG_MARKER_DHT)
		s += "\t\tDefine Huffman Tables\n";
	if (markers & V4L2_JPEG_MARKER_DQT)
		s += "\t\tDefine Quantization Tables\n";
	if (markers & V4L2_JPEG_MARKER_DRI)
		s += "\t\tDefine Restart Interval\n";
	if (markers & V4L2_JPEG_MARKER_COM)
		s += "\t\tDefine Comment\n";
	if (markers & V4L2_JPEG_MARKER_APP)
		s += "\t\tDefine APP segment\n";
	return s;
}

static void printjpegcomp(const struct v4l2_jpegcompression &jc)
{
	printf("JPEG compression:\n");
	printf("\tQuality: %d\n", jc.quality);
	if (jc.COM_len)
		printf("\tComment: '%s'\n", jc.COM_data);
	if (jc.APP_len)
		printf("\tAPP%x   : '%s'\n", jc.APPn, jc.APP_data);
	printf("\tMarkers: 0x%08lx\n", jc.jpeg_markers);
	printf("%s", markers2s(jc.jpeg_markers).c_str());
}

static void printcrop(const struct v4l2_crop &crop)
{
	printf("Crop: Left %d, Top %d, Width %d, Height %d\n",
			crop.c.left, crop.c.top, crop.c.width, crop.c.height);
}

static void printcropcap(const struct v4l2_cropcap &cropcap)
{
	printf("Crop Capability %s:\n", buftype2s(cropcap.type).c_str());
	printf("\tBounds      : Left %d, Top %d, Width %d, Height %d\n",
			cropcap.bounds.left, cropcap.bounds.top, cropcap.bounds.width, cropcap.bounds.height);
	printf("\tDefault     : Left %d, Top %d, Width %d, Height %d\n",
			cropcap.defrect.left, cropcap.defrect.top, cropcap.defrect.width, cropcap.defrect.height);
	printf("\tPixel Aspect: %u/%u\n", cropcap.pixelaspect.numerator, cropcap.pixelaspect.denominator);
}

static void printfmt(struct v4l2_format vfmt)
{
	const flag_def vbi_def[] = {
		{ V4L2_VBI_UNSYNC,     "unsynchronized" },
		{ V4L2_VBI_INTERLACED, "interlaced" },
		{ 0, NULL }
	};
	printf("Format %s:\n", buftype2s(vfmt.type).c_str());

	switch (vfmt.type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		printf("\tWidth/Height  : %u/%u\n", vfmt.fmt.pix.width, vfmt.fmt.pix.height);
		printf("\tPixel Format  : '%s'\n", fcc2s(vfmt.fmt.pix.pixelformat).c_str());
		printf("\tField         : %s\n", field2s(vfmt.fmt.pix.field).c_str());
		printf("\tBytes per Line: %u\n", vfmt.fmt.pix.bytesperline);
		printf("\tSize Image    : %u\n", vfmt.fmt.pix.sizeimage);
		printf("\tColorspace    : %s\n", colorspace2s(vfmt.fmt.pix.colorspace).c_str());
		if (vfmt.fmt.pix.priv)
			printf("\tCustom Info   : %08x\n", vfmt.fmt.pix.priv);
		break;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY:
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
		printf("\tLeft/Top    : %d/%d\n",
				vfmt.fmt.win.w.left, vfmt.fmt.win.w.top);
		printf("\tWidth/Height: %d/%d\n",
				vfmt.fmt.win.w.width, vfmt.fmt.win.w.height);
		printf("\tField       : %s\n", field2s(vfmt.fmt.win.field).c_str());
		printf("\tChroma Key  : 0x%08x\n", vfmt.fmt.win.chromakey);
		printf("\tGlobal Alpha: 0x%02x\n", vfmt.fmt.win.global_alpha);
		printf("\tClip Count  : %u\n", vfmt.fmt.win.clipcount);
		printf("\tClip Bitmap : %s\n", vfmt.fmt.win.bitmap ? "Yes" : "No");
		break;
	case V4L2_BUF_TYPE_VBI_CAPTURE:
	case V4L2_BUF_TYPE_VBI_OUTPUT:
		printf("\tSampling Rate   : %u Hz\n", vfmt.fmt.vbi.sampling_rate);
		printf("\tOffset          : %u samples (%g secs after leading edge)\n",
				vfmt.fmt.vbi.offset,
				(double)vfmt.fmt.vbi.offset / (double)vfmt.fmt.vbi.sampling_rate);
		printf("\tSamples per Line: %u\n", vfmt.fmt.vbi.samples_per_line);
		printf("\tSample Format   : %s\n", fcc2s(vfmt.fmt.vbi.sample_format).c_str());
		printf("\tStart 1st Field : %u\n", vfmt.fmt.vbi.start[0]);
		printf("\tCount 1st Field : %u\n", vfmt.fmt.vbi.count[0]);
		printf("\tStart 2nd Field : %u\n", vfmt.fmt.vbi.start[1]);
		printf("\tCount 2nd Field : %u\n", vfmt.fmt.vbi.count[1]);
		if (vfmt.fmt.vbi.flags)
			printf("\tFlags           : %s\n", flags2s(vfmt.fmt.vbi.flags, vbi_def).c_str());
		break;
	case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
	case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
		printf("\tService Set    : %s\n",
				flags2s(vfmt.fmt.sliced.service_set, service_def).c_str());
		for (int i = 0; i < 24; i++) {
			printf("\tService Line %2d: %8s / %-8s\n", i,
			       flags2s(vfmt.fmt.sliced.service_lines[0][i], service_def).c_str(),
			       flags2s(vfmt.fmt.sliced.service_lines[1][i], service_def).c_str());
		}
		printf("\tI/O Size       : %u\n", vfmt.fmt.sliced.io_size);
		break;
	case V4L2_BUF_TYPE_PRIVATE:
		break;
	}
}

static void print_video_formats(int fd, enum v4l2_buf_type type)
{
	struct v4l2_fmtdesc fmt;

	fmt.index = 0;
	fmt.type = type;
	while (ioctl(fd, VIDIOC_ENUM_FMT, &fmt) >= 0) {
		printf("\tIndex       : %d\n", fmt.index);
		printf("\tType        : %s\n", buftype2s(type).c_str());
		printf("\tPixel Format: '%s'", fcc2s(fmt.pixelformat).c_str());
		if (fmt.flags)
			printf(" (compressed)");
		printf("\n");
		printf("\tName        : %s\n", fmt.description);
		printf("\n");
		fmt.index++;
	}
}

static char *pts_to_string(char *str, unsigned long pts)
{
	static char buf[256];
	int hours, minutes, seconds, fracsec;
	float fps;
	int frame;
	char *p = (str) ? str : buf;

	static const int MPEG_CLOCK_FREQ = 90000;
	seconds = pts / MPEG_CLOCK_FREQ;
	fracsec = pts % MPEG_CLOCK_FREQ;

	minutes = seconds / 60;
	seconds = seconds % 60;

	hours = minutes / 60;
	minutes = minutes % 60;

	fps = 30;
	frame = (int)ceilf(((float)fracsec / (float)MPEG_CLOCK_FREQ) * fps);

	snprintf(p, sizeof(buf), "%d:%02d:%02d:%d", hours, minutes, seconds,
		 frame);
	return p;
}

static const char *audmode2s(int audmode)
{
	switch (audmode) {
		case V4L2_TUNER_MODE_STEREO: return "stereo";
		case V4L2_TUNER_MODE_LANG1: return "lang1";
		case V4L2_TUNER_MODE_LANG2: return "lang2";
		case V4L2_TUNER_MODE_LANG1_LANG2: return "bilingual";
		case V4L2_TUNER_MODE_MONO: return "mono";
		default: return "unknown";
	}
}

static std::string rxsubchans2s(int rxsubchans)
{
	std::string s;

	if (rxsubchans & V4L2_TUNER_SUB_MONO)
		s += "mono ";
	if (rxsubchans & V4L2_TUNER_SUB_STEREO)
		s += "stereo ";
	if (rxsubchans & V4L2_TUNER_SUB_LANG1)
		s += "lang1 ";
	if (rxsubchans & V4L2_TUNER_SUB_LANG2)
		s += "lang2 ";
	return s;
}

static std::string tcap2s(unsigned cap)
{
	std::string s;

	if (cap & V4L2_TUNER_CAP_LOW)
		s += "62.5 Hz ";
	else
		s += "62.5 kHz ";
	if (cap & V4L2_TUNER_CAP_NORM)
		s += "multi-standard ";
	if (cap & V4L2_TUNER_CAP_STEREO)
		s += "stereo ";
	if (cap & V4L2_TUNER_CAP_LANG1)
		s += "lang1 ";
	if (cap & V4L2_TUNER_CAP_LANG2)
		s += "lang2 ";
	return s;
}

static std::string cap2s(unsigned cap)
{
	std::string s;

	if (cap & V4L2_CAP_VIDEO_CAPTURE)
		s += "\t\tVideo Capture\n";
	if (cap & V4L2_CAP_VIDEO_OUTPUT)
		s += "\t\tVideo Output\n";
	if (cap & V4L2_CAP_VIDEO_OVERLAY)
		s += "\t\tVideo Overlay\n";
	if (cap & V4L2_CAP_VIDEO_OUTPUT_OVERLAY)
		s += "\t\tVideo Output Overlay\n";
	if (cap & V4L2_CAP_VBI_CAPTURE)
		s += "\t\tVBI Capture\n";
	if (cap & V4L2_CAP_VBI_OUTPUT)
		s += "\t\tVBI Output\n";
	if (cap & V4L2_CAP_SLICED_VBI_CAPTURE)
		s += "\t\tSliced VBI Capture\n";
	if (cap & V4L2_CAP_SLICED_VBI_OUTPUT)
		s += "\t\tSliced VBI Output\n";
	if (cap & V4L2_CAP_RDS_CAPTURE)
		s += "\t\tRDS Capture\n";
	if (cap & V4L2_CAP_TUNER)
		s += "\t\tTuner\n";
	if (cap & V4L2_CAP_AUDIO)
		s += "\t\tAudio\n";
	if (cap & V4L2_CAP_RADIO)
		s += "\t\tRadio\n";
	if (cap & V4L2_CAP_READWRITE)
		s += "\t\tRead/Write\n";
	if (cap & V4L2_CAP_ASYNCIO)
		s += "\t\tAsync I/O\n";
	if (cap & V4L2_CAP_STREAMING)
		s += "\t\tStreaming\n";
	return s;
}

static v4l2_std_id parse_pal(const char *pal)
{
	if (pal[0] == '-') {
		switch (pal[1]) {
			case '6':
				return V4L2_STD_PAL_60;
			case 'b':
			case 'B':
			case 'g':
			case 'G':
				return V4L2_STD_PAL_BG;
			case 'h':
			case 'H':
				return V4L2_STD_PAL_H;
			case 'n':
			case 'N':
				if (pal[2] == 'c' || pal[2] == 'C')
					return V4L2_STD_PAL_Nc;
				return V4L2_STD_PAL_N;
			case 'i':
			case 'I':
				return V4L2_STD_PAL_I;
			case 'd':
			case 'D':
			case 'k':
			case 'K':
				return V4L2_STD_PAL_DK;
			case 'M':
			case 'm':
				return V4L2_STD_PAL_M;
			case '-':
				break;
		}
	}
	fprintf(stderr, "pal specifier not recognised\n");
	return 0;
}

static v4l2_std_id parse_secam(const char *secam)
{
	if (secam[0] == '-') {
		switch (secam[1]) {
			case 'b':
			case 'B':
			case 'g':
			case 'G':
			case 'h':
			case 'H':
				return V4L2_STD_SECAM_B | V4L2_STD_SECAM_G | V4L2_STD_SECAM_H;
			case 'd':
			case 'D':
			case 'k':
			case 'K':
				return V4L2_STD_SECAM_DK;
			case 'l':
			case 'L':
				if (secam[2] == 'C' || secam[2] == 'c')
					return V4L2_STD_SECAM_LC;
				return V4L2_STD_SECAM_L;
			case '-':
				break;
		}
	}
	fprintf(stderr, "secam specifier not recognised\n");
	return 0;
}

static v4l2_std_id parse_ntsc(const char *ntsc)
{
	if (ntsc[0] == '-') {
		switch (ntsc[1]) {
			case 'm':
			case 'M':
				return V4L2_STD_NTSC_M;
			case 'j':
			case 'J':
				return V4L2_STD_NTSC_M_JP;
			case 'k':
			case 'K':
				return V4L2_STD_NTSC_M_KR;
			case '-':
				break;
		}
	}
	fprintf(stderr, "ntsc specifier not recognised\n");
	return 0;
}

static int doioctl(int fd, int request, void *parm, const char *name)
{
	int retVal = ioctl(fd, request, parm);

	if (retVal < 0) {
		app_result = -1;
	}
	if (options[OptSilent]) return retVal;
	if (retVal < 0)
		printf("%s: failed: %s\n", name, strerror(errno));
	else if (verbose)
		printf("%s: ok\n", name);

	return retVal;
}

static bool is_v4l_dev(const char *name)
{
	return !memcmp(name, "vtx", 3) ||
		!memcmp(name, "video", 5) ||
		!memcmp(name, "radio", 5) ||
		!memcmp(name, "vbi", 3);
}

static int calc_node_val(const char *s)
{
	int n = 0;

	s = strrchr(s, '/') + 1;
	if (!memcmp(s, "video", 5)) n = 0;
	else if (!memcmp(s, "radio", 5)) n = 0x100;
	else if (!memcmp(s, "vbi", 3)) n = 0x200;
	else if (!memcmp(s, "vtx", 3)) n = 0x300;
	n += atol(s + (n >= 0x200 ? 3 : 5));
	return n;
}

static bool sort_on_device_name(const std::string &s1, const std::string &s2)
{
	int n1 = calc_node_val(s1.c_str());
	int n2 = calc_node_val(s2.c_str());

	return n1 < n2;
}

static void list_devices()
{
	DIR *dp;
	struct dirent *ep;
	dev_vec files;
	dev_map links;
	dev_map cards;
	struct v4l2_capability vcap;

	dp = opendir("/dev");
	if (dp == NULL) {
		perror ("Couldn't open the directory");
		return;
	}
	while (ep = readdir(dp))
		if (is_v4l_dev(ep->d_name))
			files.push_back(std::string("/dev/") + ep->d_name);
	closedir(dp);

#if 0
	dp = opendir("/dev/v4l");
	if (dp) {
		while (ep = readdir(dp))
			if (is_v4l_dev(ep->d_name))
				files.push_back(std::string("/dev/v4l/") + ep->d_name);
		closedir(dp);
	}
#endif

	/* Find device nodes which are links to other device nodes */
	for (dev_vec::iterator iter = files.begin();
			iter != files.end(); ) {
		char link[64+1];
		int link_len;
		std::string target;

		link_len = readlink(iter->c_str(), link, 64);
		if (link_len < 0) {	/* Not a link or error */
			iter++;
			continue;
		}
		link[link_len] = '\0';

		/* Only remove from files list if target itself is in list */
		if (link[0] != '/')	/* Relative link */
			target = std::string("/dev/");
		target += link;
		if (find(files.begin(), files.end(), target) == files.end()) {
			iter++;
			continue;
		}

		/* Move the device node from files to links */
		if (links[target].empty())
			links[target] = *iter;
		else
			links[target] += ", " + *iter;
		files.erase(iter);
	}

	std::sort(files.begin(), files.end(), sort_on_device_name);

	for (dev_vec::iterator iter = files.begin();
			iter != files.end(); ++iter) {
		int fd = open(iter->c_str(), O_RDWR);
		std::string bus_info;

		if (fd < 0)
			continue;
		doioctl(fd, VIDIOC_QUERYCAP, &vcap, "VIDIOC_QUERYCAP");
		close(fd);
		bus_info = (const char *)vcap.bus_info;
		if (cards[bus_info].empty())
			cards[bus_info] += std::string((char *)vcap.card) + " (" + bus_info + "):\n";
		cards[bus_info] += "\t" + (*iter);
		if (!(links[*iter].empty()))
			cards[bus_info] += " <- " + links[*iter];
		cards[bus_info] += "\n";
	}
	for (dev_map::iterator iter = cards.begin();
			iter != cards.end(); ++iter) {
		printf("%s\n", iter->second.c_str());
	}
}

static int parse_subopt(char **subs, const char * const *subopts, char **value)
{
	int opt = getsubopt(subs, (char * const *)subopts, value);

	if (opt == -1) {
		fprintf(stderr, "Invalid suboptions specified\n");
		usage();
		exit(1);
	}
	if (value == NULL) {
		fprintf(stderr, "No value given to suboption <%s>\n",
				subopts[opt]);
		usage();
		exit(1);
	}
	return opt;
}

static void parse_next_subopt(char **subs, char **value)
{
	static char *const subopts[] = {
	    NULL
	};
	int opt = getsubopt(subs, subopts, value);

	if (value == NULL) {
		fprintf(stderr, "No value given to suboption <%s>\n",
				subopts[opt]);
		usage();
		exit(1);
	}
}

static void print_std(const char *prefix, const char *stds[], unsigned long long std)
{
	int first = 1;

	printf("\t%s-", prefix);
	while (*stds) {
		if (std & 1) {
			if (!first)
				printf("/");
			first = 0;
			printf("%s", *stds);
		}
		stds++;
		std >>= 1;
	}
	printf("\n");
}

static void do_crop(int fd, unsigned int set_crop, struct v4l2_rect &vcrop, v4l2_buf_type type)
{
    struct v4l2_crop in_crop;

    in_crop.type = type;
    if (doioctl(fd, VIDIOC_G_CROP, &in_crop, "VIDIOC_G_CROP") == 0) {
	if (set_crop & CropWidth)
	    in_crop.c.width = vcrop.width;
	if (set_crop & CropHeight)
	    in_crop.c.height = vcrop.height;
	if (set_crop & CropLeft)
	    in_crop.c.left = vcrop.left;
	if (set_crop & CropTop)
	    in_crop.c.top = vcrop.top;
	doioctl(fd, VIDIOC_S_CROP, &in_crop, "VIDIOC_S_CROP");
    }
}

static void parse_crop(char *optarg, unsigned int &set_crop, v4l2_rect &vcrop)
{
    char *value;
    char *subs = optarg;

    while (*subs != '\0') {
	static const char *const subopts[] = {
	    "left",
	    "top",
	    "width",
	    "height",
	    NULL
	};

	switch (parse_subopt(&subs, subopts, &value)) {
	    case 0:
		vcrop.left = strtol(value, 0L, 0);
		set_crop |= CropLeft;
		break;
	    case 1:
		vcrop.top = strtol(value, 0L, 0);
		set_crop |= CropTop;
		break;
	    case 2:
		vcrop.width = strtol(value, 0L, 0);
		set_crop |= CropWidth;
		break;
	    case 3:
		vcrop.height = strtol(value, 0L, 0);
		set_crop |= CropHeight;
		break;
	}
    }
}

static enum v4l2_field parse_field(const char *s)
{
	if (!strcmp(s, "any")) return V4L2_FIELD_ANY;
	if (!strcmp(s, "none")) return V4L2_FIELD_NONE;
	if (!strcmp(s, "top")) return V4L2_FIELD_TOP;
	if (!strcmp(s, "bottom")) return V4L2_FIELD_BOTTOM;
	if (!strcmp(s, "interlaced")) return V4L2_FIELD_INTERLACED;
	if (!strcmp(s, "seq_tb")) return V4L2_FIELD_SEQ_TB;
	if (!strcmp(s, "seq_bt")) return V4L2_FIELD_SEQ_BT;
	if (!strcmp(s, "alternate")) return V4L2_FIELD_ALTERNATE;
	if (!strcmp(s, "interlaced_tb")) return V4L2_FIELD_INTERLACED_TB;
	if (!strcmp(s, "interlaced_bt")) return V4L2_FIELD_INTERLACED_BT;
	return V4L2_FIELD_ANY;
}

int main(int argc, char **argv)
{
	char *value, *subs;
	int i;

	int fd = -1;

	/* bitfield for fmts */
	unsigned int set_fmts = 0;
	unsigned int set_fmts_out = 0;
	unsigned int set_crop = 0;
	unsigned int set_crop_out = 0;
	unsigned int set_crop_overlay = 0;
	unsigned int set_crop_out_overlay = 0;
	unsigned int set_fbuf = 0;
	unsigned int set_overlay_fmt = 0;
	unsigned int set_overlay_fmt_out = 0;

	int mode = V4L2_TUNER_MODE_STEREO;	/* set audio mode */

	/* command args */
	int ch;
	const char *device = "/dev/video0";	/* -d device */
	struct v4l2_format vfmt;	/* set_format/get_format for video */
	struct v4l2_format vfmt_out;	/* set_format/get_format video output */
	struct v4l2_format vbi_fmt;	/* set_format/get_format for sliced VBI */
	struct v4l2_format vbi_fmt_out;	/* set_format/get_format for sliced VBI output */
	struct v4l2_format raw_fmt;	/* set_format/get_format for VBI */
	struct v4l2_format raw_fmt_out;	/* set_format/get_format for VBI output */
	struct v4l2_format overlay_fmt;	/* set_format/get_format video overlay */
	struct v4l2_format overlay_fmt_out;	/* set_format/get_format video overlay output */
	struct v4l2_tuner tuner;        /* set_tuner/get_tuner */
	struct v4l2_capability vcap;	/* list_cap */
	struct v4l2_input vin;		/* list_inputs */
	struct v4l2_output vout;	/* list_outputs */
	struct v4l2_audio vaudio;	/* list audio inputs */
	struct v4l2_audioout vaudout;   /* audio outputs */
	struct v4l2_rect vcrop; 	/* crop rect */
	struct v4l2_rect vcrop_out; 	/* crop rect */
	struct v4l2_rect vcrop_overlay; 	/* crop rect */
	struct v4l2_rect vcrop_out_overlay; 	/* crop rect */
	struct v4l2_framebuffer fbuf;   /* fbuf */
	struct v4l2_jpegcompression jpegcomp; /* jpeg compression */
	int input;			/* set_input/get_input */
	int output;			/* set_output/get_output */
	v4l2_std_id std;		/* get_std/set_std */
	double freq = 0;		/* get/set frequency */
	struct v4l2_frequency vf;	/* get_freq/set_freq */
	struct v4l2_standard vs;	/* list_std */
	int overlay;			/* overlay */
	unsigned int *set_overlay_fmt_ptr;
	struct v4l2_format *overlay_fmt_ptr;
	char short_options[26 * 2 * 2 + 1];
	int idx = 0;
	int ret;

	memset(&vfmt, 0, sizeof(vfmt));
	memset(&vbi_fmt, 0, sizeof(vbi_fmt));
	memset(&raw_fmt, 0, sizeof(raw_fmt));
	memset(&vfmt_out, 0, sizeof(vfmt_out));
	memset(&vbi_fmt_out, 0, sizeof(vbi_fmt_out));
	memset(&overlay_fmt, 0, sizeof(overlay_fmt));
	memset(&overlay_fmt_out, 0, sizeof(overlay_fmt_out));
	memset(&raw_fmt_out, 0, sizeof(raw_fmt_out));
	memset(&tuner, 0, sizeof(tuner));
	memset(&vcap, 0, sizeof(vcap));
	memset(&vin, 0, sizeof(vin));
	memset(&vout, 0, sizeof(vout));
	memset(&vaudio, 0, sizeof(vaudio));
	memset(&vaudout, 0, sizeof(vaudout));
	memset(&vcrop, 0, sizeof(vcrop));
	memset(&vcrop_out, 0, sizeof(vcrop_out));
	memset(&vcrop_overlay, 0, sizeof(vcrop_overlay));
	memset(&vcrop_out_overlay, 0, sizeof(vcrop_out_overlay));
	memset(&vf, 0, sizeof(vf));
	memset(&vs, 0, sizeof(vs));
	memset(&fbuf, 0, sizeof(fbuf));
	memset(&jpegcomp, 0, sizeof(jpegcomp));

	if (argc == 1) {
		usage();
		return 0;
	}
	for (i = 0; long_options[i].name; i++) {
		if (!isalpha(long_options[i].val))
			continue;
		short_options[idx++] = long_options[i].val;
		if (long_options[i].has_arg == required_argument)
			short_options[idx++] = ':';
	}
	while (1) {
		int option_index = 0;

		short_options[idx] = 0;
		ch = getopt_long(argc, argv, short_options,
				 long_options, &option_index);
		if (ch == -1)
			break;

		options[(int)ch] = 1;
		switch (ch) {
		case OptHelp:
			usage();
			return 0;
		case OptSetDevice:
			device = optarg;
			if (device[0] >= '0' && device[0] <= '9' && device[1] == 0) {
				static char newdev[20];
				char dev = device[0];

				sprintf(newdev, "/dev/video%c", dev);
				device = newdev;
			}
			break;
		case OptSetVideoFormat:
		case OptTryVideoFormat:
			subs = optarg;
			while (*subs != '\0') {
				static const char *const subopts[] = {
					"width",
					"height",
					"pixelformat",
					NULL
				};

				switch (parse_subopt(&subs, subopts, &value)) {
				case 0:
					vfmt.fmt.pix.width = strtol(value, 0L, 0);
					set_fmts |= FmtWidth;
					break;
				case 1:
					vfmt.fmt.pix.height = strtol(value, 0L, 0);
					set_fmts |= FmtHeight;
					break;
				case 2:
					if (strlen(value) == 4)
						vfmt.fmt.pix.pixelformat =
						    v4l2_fourcc(value[0], value[1],
							    value[2], value[3]);
					else
						vfmt.fmt.pix.pixelformat = strtol(value, 0L, 0);
					set_fmts |= FmtPixelFormat;
					break;
				}
			}
			break;
		case OptSetVideoOutFormat:
		case OptTryVideoOutFormat:
			subs = optarg;
			while (*subs != '\0') {
				static const char *const subopts[] = {
					"width",
					"height",
					NULL
				};

				switch (parse_subopt(&subs, subopts, &value)) {
				case 0:
					vfmt_out.fmt.pix.width = strtol(value, 0L, 0);
					set_fmts_out |= FmtWidth;
					break;
				case 1:
					vfmt_out.fmt.pix.height = strtol(value, 0L, 0);
					set_fmts_out |= FmtHeight;
					break;
				}
			}
			break;
		case OptSetOverlayFormat:
		case OptTryOverlayFormat:
		case OptSetOutputOverlayFormat:
		case OptTryOutputOverlayFormat:
			switch (ch) {
			case OptSetOverlayFormat:
			case OptTryOverlayFormat:
				set_overlay_fmt_ptr = &set_overlay_fmt;
				overlay_fmt_ptr = &overlay_fmt;
				break;
			case OptSetOutputOverlayFormat:
			case OptTryOutputOverlayFormat:
				set_overlay_fmt_ptr = &set_overlay_fmt_out;
				overlay_fmt_ptr = &overlay_fmt_out;
				break;
			}
			subs = optarg;
			while (*subs != '\0') {
				static const char *const subopts[] = {
					"chromakey",
					"global_alpha",
					"left",
					"top",
					"width",
					"height",
					"field",
					NULL
				};

				switch (parse_subopt(&subs, subopts, &value)) {
				case 0:
					overlay_fmt_ptr->fmt.win.chromakey = strtol(value, 0L, 0);
					*set_overlay_fmt_ptr |= FmtChromaKey;
					break;
				case 1:
					overlay_fmt_ptr->fmt.win.global_alpha = strtol(value, 0L, 0);
					*set_overlay_fmt_ptr |= FmtGlobalAlpha;
					break;
				case 2:
					overlay_fmt_ptr->fmt.win.w.left = strtol(value, 0L, 0);
					*set_overlay_fmt_ptr |= FmtLeft;
					break;
				case 3:
					overlay_fmt_ptr->fmt.win.w.top = strtol(value, 0L, 0);
					*set_overlay_fmt_ptr |= FmtTop;
					break;
				case 4:
					overlay_fmt_ptr->fmt.win.w.width = strtol(value, 0L, 0);
					*set_overlay_fmt_ptr |= FmtWidth;
					break;
				case 5:
					overlay_fmt_ptr->fmt.win.w.height = strtol(value, 0L, 0);
					*set_overlay_fmt_ptr |= FmtHeight;
					break;
				case 6:
					overlay_fmt_ptr->fmt.win.field = parse_field(value);
					*set_overlay_fmt_ptr |= FmtField;
					break;
				}
			}
			break;
		case OptSetFBuf:
			subs = optarg;
			while (*subs != '\0') {
				static const char *const subopts[] = {
					"chromakey",
					"global_alpha",
					"local_alpha",
					"local_inv_alpha",
					NULL
				};

				switch (parse_subopt(&subs, subopts, &value)) {
				case 0:
					fbuf.flags |= strtol(value, 0L, 0) ? V4L2_FBUF_FLAG_CHROMAKEY : 0;
					set_fbuf |= V4L2_FBUF_FLAG_CHROMAKEY;
					break;
				case 1:
					fbuf.flags |= strtol(value, 0L, 0) ? V4L2_FBUF_FLAG_GLOBAL_ALPHA : 0;
					set_fbuf |= V4L2_FBUF_FLAG_GLOBAL_ALPHA;
					break;
				case 2:
					fbuf.flags |= strtol(value, 0L, 0) ? V4L2_FBUF_FLAG_LOCAL_ALPHA : 0;
					set_fbuf |= V4L2_FBUF_FLAG_LOCAL_ALPHA;
					break;
				case 3:
					fbuf.flags |= strtol(value, 0L, 0) ? V4L2_FBUF_FLAG_LOCAL_INV_ALPHA : 0;
					set_fbuf |= V4L2_FBUF_FLAG_LOCAL_INV_ALPHA;
					break;
				}
			}
			break;
		case OptOverlay:
			overlay = strtol(optarg, 0L, 0);
			break;
		case OptSetCrop:
			parse_crop(optarg, set_crop, vcrop);
			break;
		case OptSetOutputCrop:
			parse_crop(optarg, set_crop_out, vcrop_out);
			break;
		case OptSetOverlayCrop:
			parse_crop(optarg, set_crop_overlay, vcrop_overlay);
			break;
		case OptSetOutputOverlayCrop:
			parse_crop(optarg, set_crop_out_overlay, vcrop_out_overlay);
			break;
		case OptSetInput:
			input = strtol(optarg, 0L, 0);
			break;
		case OptSetOutput:
			output = strtol(optarg, 0L, 0);
			break;
		case OptSetAudioInput:
			vaudio.index = strtol(optarg, 0L, 0);
			break;
		case OptSetAudioOutput:
			vaudout.index = strtol(optarg, 0L, 0);
			break;
		case OptSetFreq:
			freq = strtod(optarg, NULL);
			break;
		case OptSetStandard:
			if (!strncmp(optarg, "pal", 3)) {
				if (optarg[3])
					std = parse_pal(optarg + 3);
				else
					std = V4L2_STD_PAL;
			}
			else if (!strncmp(optarg, "ntsc", 4)) {
				if (optarg[4])
					std = parse_ntsc(optarg + 4);
				else
					std = V4L2_STD_NTSC;
			}
			else if (!strncmp(optarg, "secam", 5)) {
				if (optarg[5])
					std = parse_secam(optarg + 5);
				else
					std = V4L2_STD_SECAM;
			}
			else {
				std = strtol(optarg, 0L, 0) | (1ULL << 63);
			}
			break;
		case OptGetCtrl:
			subs = optarg;
			while (*subs != '\0') {
				parse_next_subopt(&subs, &value);
				if (strchr(value, '=')) {
				    usage();
				    exit(1);
				}
				else {
				    get_ctrls.push_back(value);
				}
			}
			break;
		case OptSetCtrl:
			subs = optarg;
			while (*subs != '\0') {
				parse_next_subopt(&subs, &value);
				if (const char *equal = strchr(value, '=')) {
				    set_ctrls[std::string(value, (equal - value))] = equal + 1;
				}
				else {
				    fprintf(stderr, "control '%s' without '='\n", value);
				    exit(1);
				}
			}
			break;
		case OptSetTuner:
			if (!strcmp(optarg, "stereo"))
				mode = V4L2_TUNER_MODE_STEREO;
			else if (!strcmp(optarg, "lang1"))
				mode = V4L2_TUNER_MODE_LANG1;
			else if (!strcmp(optarg, "lang2"))
				mode = V4L2_TUNER_MODE_LANG2;
			else if (!strcmp(optarg, "bilingual"))
				mode = V4L2_TUNER_MODE_LANG1_LANG2;
			else if (!strcmp(optarg, "mono"))
				mode = V4L2_TUNER_MODE_MONO;
			else {
				fprintf(stderr, "Unknown audio mode\n");
				usage();
				return 1;
			}
			break;
		case OptSetSlicedVbiFormat:
		case OptSetSlicedVbiOutFormat:
		case OptTrySlicedVbiFormat:
		case OptTrySlicedVbiOutFormat:
		{
			bool foundOff = false;
			v4l2_format *fmt = &vbi_fmt;

			if (ch == OptSetSlicedVbiOutFormat || ch == OptTrySlicedVbiOutFormat)
				fmt = &vbi_fmt_out;
			fmt->fmt.sliced.service_set = 0;
			subs = optarg;
			while (*subs != '\0') {
				static const char *const subopts[] = {
					"off",
					"teletext",
					"cc",
					"wss",
					"vps",
					NULL
				};

				switch (parse_subopt(&subs, subopts, &value)) {
				case 0:
					foundOff = true;
					break;
				case 1:
					fmt->fmt.sliced.service_set |=
					    V4L2_SLICED_TELETEXT_B;
					break;
				case 2:
					fmt->fmt.sliced.service_set |=
					    V4L2_SLICED_CAPTION_525;
					break;
				case 3:
					fmt->fmt.sliced.service_set |=
					    V4L2_SLICED_WSS_625;
					break;
				case 4:
					fmt->fmt.sliced.service_set |=
					    V4L2_SLICED_VPS;
					break;
				}
			}
			if (foundOff && fmt->fmt.sliced.service_set) {
				fprintf(stderr, "Sliced VBI mode 'off' cannot be combined with other modes\n");
				usage();
				return 1;
			}
			break;
		}
		case OptSetJpegComp:
		{
			subs = optarg;
			while (*subs != '\0') {
				static const char *const subopts[] = {
					"app0", "app1", "app2", "app3",
					"app4", "app5", "app6", "app7",
					"app8", "app9", "appa", "appb",
					"appc", "appd", "appe", "appf",
					"quality",
					"markers",
					"comment",
					NULL
				};
				int len;
				int opt = parse_subopt(&subs, subopts, &value);

				switch (opt) {
				case 16:
					jpegcomp.quality = strtol(value, 0L, 0);
					break;
				case 17:
					if (strstr(value, "dht"))
						jpegcomp.jpeg_markers |= V4L2_JPEG_MARKER_DHT;
					if (strstr(value, "dqt"))
						jpegcomp.jpeg_markers |= V4L2_JPEG_MARKER_DQT;
					if (strstr(value, "dri"))
						jpegcomp.jpeg_markers |= V4L2_JPEG_MARKER_DRI;
					break;
				case 18:
					len = strlen(value);
					if (len > sizeof(jpegcomp.COM_data) - 1)
						len = sizeof(jpegcomp.COM_data) - 1;
					jpegcomp.COM_len = len;
					memcpy(jpegcomp.COM_data, value, len);
					jpegcomp.COM_data[len] = '\0';
					break;
				default:
					if (opt < 0 || opt > 15)
						break;
					len = strlen(value);
					if (len > sizeof(jpegcomp.APP_data) - 1)
						len = sizeof(jpegcomp.APP_data) - 1;
					if (jpegcomp.APP_len) {
						fprintf(stderr, "Only one APP segment can be set\n");
						break;
					}
					jpegcomp.APP_len = len;
					memcpy(jpegcomp.APP_data, value, len);
					jpegcomp.APP_data[len] = '\0';
					jpegcomp.APPn = opt;
					break;
				}
			}
			break;
		}
		case OptListDevices:
			list_devices();
			break;
		case ':':
			fprintf(stderr, "Option `%s' requires a value\n",
				argv[optind]);
			usage();
			return 1;
		case '?':
			fprintf(stderr, "Unknown argument `%s'\n",
				argv[optind]);
			usage();
			return 1;
		}
	}
	if (optind < argc) {
		printf("unknown arguments: ");
		while (optind < argc)
			printf("%s ", argv[optind++]);
		printf("\n");
		usage();
		return 1;
	}

	if ((fd = open(device, O_RDWR)) < 0) {
		fprintf(stderr, "Failed to open %s: %s\n", device,
			strerror(errno));
		exit(1);
	}

	verbose = options[OptVerbose];
	doioctl(fd, VIDIOC_QUERYCAP, &vcap, "VIDIOC_QUERYCAP");
	capabilities = vcap.capabilities;
	find_controls(fd);
	for (ctrl_get_list::iterator iter = get_ctrls.begin(); iter != get_ctrls.end(); ++iter) {
	    if (ctrl_str2id.find(*iter) == ctrl_str2id.end()) {
		fprintf(stderr, "unknown control '%s'\n", (*iter).c_str());
		exit(1);
	    }
	}
	for (ctrl_set_map::iterator iter = set_ctrls.begin(); iter != set_ctrls.end(); ++iter) {
	    if (ctrl_str2id.find(iter->first) == ctrl_str2id.end()) {
		fprintf(stderr, "unknown control '%s'\n", iter->first.c_str());
		exit(1);
	    }
	}

	if (options[OptAll]) {
		options[OptGetVideoFormat] = 1;
		options[OptGetCrop] = 1;
		options[OptGetVideoOutFormat] = 1;
		options[OptGetDriverInfo] = 1;
		options[OptGetInput] = 1;
		options[OptGetOutput] = 1;
		options[OptGetAudioInput] = 1;
		options[OptGetAudioOutput] = 1;
		options[OptGetStandard] = 1;
		options[OptGetFreq] = 1;
		options[OptGetTuner] = 1;
		options[OptGetOverlayFormat] = 1;
		options[OptGetOutputOverlayFormat] = 1;
		options[OptGetVbiFormat] = 1;
		options[OptGetVbiOutFormat] = 1;
		options[OptGetSlicedVbiFormat] = 1;
		options[OptGetSlicedVbiOutFormat] = 1;
		options[OptGetFBuf] = 1;
		options[OptGetCropCap] = 1;
		options[OptGetOutputCropCap] = 1;
		options[OptGetJpegComp] = 1;
		options[OptSilent] = 1;
	}

	/* Information Opts */

	if (options[OptGetDriverInfo]) {
		printf("Driver Info:\n");
		printf("\tDriver name   : %s\n", vcap.driver);
		printf("\tCard type     : %s\n", vcap.card);
		printf("\tBus info      : %s\n", vcap.bus_info);
		printf("\tDriver version: %d\n", vcap.version);
		printf("\tCapabilities  : 0x%08X\n", vcap.capabilities);
		printf("%s", cap2s(vcap.capabilities).c_str());
	}

	/* Set options */

	if (options[OptStreamOff]) {
		int dummy = 0;
		doioctl(fd, VIDIOC_STREAMOFF, &dummy, "VIDIOC_STREAMOFF");
	}

	if (options[OptSetFreq]) {
		double fac = 16;

		if (doioctl(fd, VIDIOC_G_TUNER, &tuner, "VIDIOC_G_TUNER") == 0) {
			fac = (tuner.capability & V4L2_TUNER_CAP_LOW) ? 16000 : 16;
		}
		vf.tuner = 0;
		vf.type = tuner.type;
		vf.frequency = __u32(freq * fac);
		if (doioctl(fd, VIDIOC_S_FREQUENCY, &vf,
			"VIDIOC_S_FREQUENCY") == 0)
			printf("Frequency set to %d (%f MHz)\n", vf.frequency,
					vf.frequency / fac);
	}

	if (options[OptSetStandard]) {
		if (std & (1ULL << 63)) {
			vs.index = std & 0xffff;
			if (ioctl(fd, VIDIOC_ENUMSTD, &vs) >= 0) {
				std = vs.id;
			}
		}
		if (doioctl(fd, VIDIOC_S_STD, &std, "VIDIOC_S_STD") == 0)
			printf("Standard set to %08llx\n", (unsigned long long)std);
	}

	if (options[OptSetInput]) {
		if (doioctl(fd, VIDIOC_S_INPUT, &input, "VIDIOC_S_INPUT") == 0) {
			printf("Video input set to %d", input);
			vin.index = input;
			if (ioctl(fd, VIDIOC_ENUMINPUT, &vin) >= 0)
				printf(" (%s)", vin.name);
			printf("\n");
		}
	}

	if (options[OptSetOutput]) {
		if (doioctl(fd, VIDIOC_S_OUTPUT, &output, "VIDIOC_S_OUTPUT") == 0)
			printf("Output set to %d\n", output);
	}

	if (options[OptSetAudioInput]) {
		if (doioctl(fd, VIDIOC_S_AUDIO, &vaudio, "VIDIOC_S_AUDIO") == 0)
			printf("Audio input set to %d\n", vaudio.index);
	}

	if (options[OptSetAudioOutput]) {
		if (doioctl(fd, VIDIOC_S_AUDOUT, &vaudout, "VIDIOC_S_AUDOUT") == 0)
			printf("Audio output set to %d\n", vaudout.index);
	}

	if (options[OptSetTuner]) {
		struct v4l2_tuner vt;

		memset(&vt, 0, sizeof(struct v4l2_tuner));
		if (doioctl(fd, VIDIOC_G_TUNER, &vt, "VIDIOC_G_TUNER") == 0) {
			vt.audmode = mode;
			doioctl(fd, VIDIOC_S_TUNER, &vt, "VIDIOC_S_TUNER");
		}
	}

	if (options[OptSetVideoFormat] || options[OptTryVideoFormat]) {
		struct v4l2_format in_vfmt;

		in_vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (doioctl(fd, VIDIOC_G_FMT, &in_vfmt, "VIDIOC_G_FMT") == 0) {
			if (set_fmts & FmtWidth)
				in_vfmt.fmt.pix.width = vfmt.fmt.pix.width;
			if (set_fmts & FmtHeight)
				in_vfmt.fmt.pix.height = vfmt.fmt.pix.height;
			if (set_fmts & FmtPixelFormat) {
				in_vfmt.fmt.pix.pixelformat = vfmt.fmt.pix.pixelformat;
				if (in_vfmt.fmt.pix.pixelformat < 256) {
					struct v4l2_fmtdesc fmt;

					fmt.index = in_vfmt.fmt.pix.pixelformat;
					fmt.type = in_vfmt.type;
					if (doioctl(fd, VIDIOC_ENUM_FMT, &fmt, "VIDIOC_ENUM_FMT"))
						goto set_vid_fmt_error;
					in_vfmt.fmt.pix.pixelformat = fmt.pixelformat;
				}
			}
			if (options[OptSetVideoFormat])
				ret = doioctl(fd, VIDIOC_S_FMT, &in_vfmt, "VIDIOC_S_FMT");
			else
				ret = doioctl(fd, VIDIOC_TRY_FMT, &in_vfmt, "VIDIOC_TRY_FMT");
			if (ret == 0 && verbose)
				printfmt(in_vfmt);
		}
	}
set_vid_fmt_error:

	if (options[OptSetVideoOutFormat] || options[OptTryVideoOutFormat]) {
		struct v4l2_format in_vfmt;

		in_vfmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		if (doioctl(fd, VIDIOC_G_FMT, &in_vfmt, "VIDIOC_G_FMT") == 0) {
			if (set_fmts_out & FmtWidth)
				in_vfmt.fmt.pix.width = vfmt_out.fmt.pix.width;
			if (set_fmts_out & FmtHeight)
				in_vfmt.fmt.pix.height = vfmt_out.fmt.pix.height;

			if (options[OptSetVideoOutFormat])
				ret = doioctl(fd, VIDIOC_S_FMT, &in_vfmt, "VIDIOC_S_FMT");
			else
				ret = doioctl(fd, VIDIOC_TRY_FMT, &in_vfmt, "VIDIOC_TRY_FMT");
			if (ret == 0 && verbose)
				printfmt(in_vfmt);
		}
	}

	if (options[OptSetSlicedVbiFormat] || options[OptTrySlicedVbiFormat]) {
		vbi_fmt.type = V4L2_BUF_TYPE_SLICED_VBI_CAPTURE;
		if (options[OptSetSlicedVbiFormat])
			ret = doioctl(fd, VIDIOC_S_FMT, &vbi_fmt, "VIDIOC_S_FMT");
		else
			ret = doioctl(fd, VIDIOC_TRY_FMT, &vbi_fmt, "VIDIOC_TRY_FMT");
		if (ret == 0 && verbose)
			printfmt(vbi_fmt);
	}

	if (options[OptSetSlicedVbiOutFormat] || options[OptTrySlicedVbiOutFormat]) {
		vbi_fmt_out.type = V4L2_BUF_TYPE_SLICED_VBI_OUTPUT;
		if (options[OptSetSlicedVbiOutFormat])
			ret = doioctl(fd, VIDIOC_S_FMT, &vbi_fmt_out, "VIDIOC_S_FMT");
		else
			ret = doioctl(fd, VIDIOC_TRY_FMT, &vbi_fmt_out, "VIDIOC_TRY_FMT");
		if (ret == 0 && verbose)
			printfmt(vbi_fmt_out);
	}

	if (options[OptSetOverlayFormat] || options[OptTryOverlayFormat]) {
		struct v4l2_format fmt;

		fmt.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
		if (doioctl(fd, VIDIOC_G_FMT, &fmt, "VIDIOC_G_FMT") == 0) {
			if (set_overlay_fmt & FmtChromaKey)
				fmt.fmt.win.chromakey = overlay_fmt.fmt.win.chromakey;
			if (set_overlay_fmt & FmtGlobalAlpha)
				fmt.fmt.win.global_alpha = overlay_fmt.fmt.win.global_alpha;
			if (set_overlay_fmt & FmtLeft)
				fmt.fmt.win.w.left = overlay_fmt.fmt.win.w.left;
			if (set_overlay_fmt & FmtTop)
				fmt.fmt.win.w.top = overlay_fmt.fmt.win.w.top;
			if (set_overlay_fmt & FmtWidth)
				fmt.fmt.win.w.width = overlay_fmt.fmt.win.w.width;
			if (set_overlay_fmt & FmtHeight)
				fmt.fmt.win.w.height = overlay_fmt.fmt.win.w.height;
			if (set_overlay_fmt & FmtField)
				fmt.fmt.win.field = overlay_fmt.fmt.win.field;
			if (options[OptSetOverlayFormat])
				ret = doioctl(fd, VIDIOC_S_FMT, &fmt, "VIDIOC_S_FMT");
			else
				ret = doioctl(fd, VIDIOC_TRY_FMT, &fmt, "VIDIOC_TRY_FMT");
			if (ret == 0 && verbose)
				printfmt(fmt);
		}
	}

	if (options[OptSetOutputOverlayFormat] || options[OptTryOutputOverlayFormat]) {
		struct v4l2_format fmt;

		fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY;
		if (doioctl(fd, VIDIOC_G_FMT, &fmt, "VIDIOC_G_FMT") == 0) {
			if (set_overlay_fmt_out & FmtChromaKey)
				fmt.fmt.win.chromakey = overlay_fmt_out.fmt.win.chromakey;
			if (set_overlay_fmt_out & FmtGlobalAlpha)
				fmt.fmt.win.global_alpha = overlay_fmt_out.fmt.win.global_alpha;
			if (set_overlay_fmt_out & FmtLeft)
				fmt.fmt.win.w.left = overlay_fmt_out.fmt.win.w.left;
			if (set_overlay_fmt_out & FmtTop)
				fmt.fmt.win.w.top = overlay_fmt_out.fmt.win.w.top;
			if (set_overlay_fmt_out & FmtWidth)
				fmt.fmt.win.w.width = overlay_fmt_out.fmt.win.w.width;
			if (set_overlay_fmt_out & FmtHeight)
				fmt.fmt.win.w.height = overlay_fmt_out.fmt.win.w.height;
			if (set_overlay_fmt_out & FmtField)
				fmt.fmt.win.field = overlay_fmt_out.fmt.win.field;
			if (options[OptSetOutputOverlayFormat])
				ret = doioctl(fd, VIDIOC_S_FMT, &fmt, "VIDIOC_S_FMT");
			else
				ret = doioctl(fd, VIDIOC_TRY_FMT, &fmt, "VIDIOC_TRY_FMT");
			if (ret == 0 && verbose)
				printfmt(fmt);
		}
	}

	if (options[OptSetFBuf]) {
		struct v4l2_framebuffer fb;

		if (doioctl(fd, VIDIOC_G_FBUF, &fb, "VIDIOC_G_FBUF") == 0) {
			fb.flags &= ~set_fbuf;
			fb.flags |= fbuf.flags;
			doioctl(fd, VIDIOC_S_FBUF, &fb, "VIDIOC_S_FBUF");
		}
	}

	if (options[OptSetJpegComp]) {
		doioctl(fd, VIDIOC_S_JPEGCOMP, &jpegcomp, "VIDIOC_S_JPEGCOMP");
	}

	if (options[OptOverlay]) {
		doioctl(fd, VIDIOC_OVERLAY, &overlay, "VIDIOC_OVERLAY");
	}

	if (options[OptSetCrop]) {
		do_crop(fd, set_crop, vcrop, V4L2_BUF_TYPE_VIDEO_CAPTURE);
	}

	if (options[OptSetOutputCrop]) {
		do_crop(fd, set_crop_out, vcrop_out, V4L2_BUF_TYPE_VIDEO_OUTPUT);
	}

	if (options[OptSetOverlayCrop]) {
		do_crop(fd, set_crop_overlay, vcrop_overlay, V4L2_BUF_TYPE_VIDEO_OVERLAY);
	}

	if (options[OptSetOutputOverlayCrop]) {
		do_crop(fd, set_crop_out_overlay, vcrop_out_overlay, V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY);
	}

	if (options[OptSetCtrl] && !set_ctrls.empty()) {
		struct v4l2_ext_controls ctrls = { 0 };

		for (ctrl_set_map::iterator iter = set_ctrls.begin();
				iter != set_ctrls.end(); ++iter) {
			struct v4l2_ext_control ctrl = { 0 };

			ctrl.id = ctrl_str2id[iter->first];
			ctrl.value = strtol(iter->second.c_str(), NULL, 0);
			if (V4L2_CTRL_ID2CLASS(ctrl.id) == V4L2_CTRL_CLASS_MPEG)
				mpeg_ctrls.push_back(ctrl);
			else
				user_ctrls.push_back(ctrl);
		}
		for (unsigned i = 0; i < user_ctrls.size(); i++) {
			struct v4l2_control ctrl;

			ctrl.id = user_ctrls[i].id;
			ctrl.value = user_ctrls[i].value;
			if (doioctl(fd, VIDIOC_S_CTRL, &ctrl, "VIDIOC_S_CTRL")) {
				fprintf(stderr, "%s: %s\n",
					ctrl_id2str[ctrl.id].c_str(),
					strerror(errno));
			}
		}
		if (mpeg_ctrls.size()) {
			ctrls.ctrl_class = V4L2_CTRL_CLASS_MPEG;
			ctrls.count = mpeg_ctrls.size();
			ctrls.controls = &mpeg_ctrls[0];
			if (doioctl(fd, VIDIOC_S_EXT_CTRLS, &ctrls, "VIDIOC_S_EXT_CTRLS")) {
				if (ctrls.error_idx >= ctrls.count) {
					fprintf(stderr, "Error setting MPEG controls: %s\n",
						strerror(errno));
				}
				else {
					fprintf(stderr, "%s: %s\n",
						ctrl_id2str[mpeg_ctrls[ctrls.error_idx].id].c_str(),
						strerror(errno));
				}
			}
		}
	}

	/* Get options */

	if (options[OptGetVideoFormat]) {
		vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (doioctl(fd, VIDIOC_G_FMT, &vfmt, "VIDIOC_G_FMT") == 0)
			printfmt(vfmt);
	}

	if (options[OptGetVideoOutFormat]) {
		vfmt_out.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		if (doioctl(fd, VIDIOC_G_FMT, &vfmt_out, "VIDIOC_G_FMT") == 0)
			printfmt(vfmt_out);
	}

	if (options[OptGetOverlayFormat]) {
		struct v4l2_format fmt;

		fmt.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
		if (doioctl(fd, VIDIOC_G_FMT, &fmt, "VIDIOC_G_FMT") == 0)
			printfmt(fmt);
	}

	if (options[OptGetOutputOverlayFormat]) {
		struct v4l2_format fmt;

		fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY;
		if (doioctl(fd, VIDIOC_G_FMT, &fmt, "VIDIOC_G_FMT") == 0)
			printfmt(fmt);
	}

	if (options[OptGetSlicedVbiFormat]) {
		vbi_fmt.type = V4L2_BUF_TYPE_SLICED_VBI_CAPTURE;
		if (doioctl(fd, VIDIOC_G_FMT, &vbi_fmt, "VIDIOC_G_FMT") == 0)
			printfmt(vbi_fmt);
	}

	if (options[OptGetSlicedVbiOutFormat]) {
		vbi_fmt_out.type = V4L2_BUF_TYPE_SLICED_VBI_OUTPUT;
		if (doioctl(fd, VIDIOC_G_FMT, &vbi_fmt_out, "VIDIOC_G_FMT") == 0)
			printfmt(vbi_fmt_out);
	}

	if (options[OptGetVbiFormat]) {
		raw_fmt.type = V4L2_BUF_TYPE_VBI_CAPTURE;
		if (doioctl(fd, VIDIOC_G_FMT, &raw_fmt, "VIDIOC_G_FMT") == 0)
			printfmt(raw_fmt);
	}

	if (options[OptGetVbiOutFormat]) {
		raw_fmt_out.type = V4L2_BUF_TYPE_VBI_OUTPUT;
		if (doioctl(fd, VIDIOC_G_FMT, &raw_fmt_out, "VIDIOC_G_FMT") == 0)
			printfmt(raw_fmt_out);
	}

	if (options[OptGetFBuf]) {
		struct v4l2_framebuffer fb;
		if (doioctl(fd, VIDIOC_G_FBUF, &fb, "VIDIOC_G_FBUF") == 0)
			printfbuf(fb);
	}

	if (options[OptGetJpegComp]) {
		struct v4l2_jpegcompression jc;
		if (doioctl(fd, VIDIOC_G_JPEGCOMP, &jc, "VIDIOC_G_JPEGCOMP") == 0)
			printjpegcomp(jc);
	}

	if (options[OptGetCropCap]) {
		struct v4l2_cropcap cropcap;

		cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (doioctl(fd, VIDIOC_CROPCAP, &cropcap, "VIDIOC_CROPCAP") == 0)
			printcropcap(cropcap);
	}

	if (options[OptGetOutputCropCap]) {
		struct v4l2_cropcap cropcap;

		cropcap.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		if (doioctl(fd, VIDIOC_CROPCAP, &cropcap, "VIDIOC_CROPCAP") == 0)
			printcropcap(cropcap);
	}

	if (options[OptGetOverlayCropCap]) {
		struct v4l2_cropcap cropcap;

		cropcap.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
		if (doioctl(fd, VIDIOC_CROPCAP, &cropcap, "VIDIOC_CROPCAP") == 0)
			printcropcap(cropcap);
	}

	if (options[OptGetOutputOverlayCropCap]) {
		struct v4l2_cropcap cropcap;

		cropcap.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY;
		if (doioctl(fd, VIDIOC_CROPCAP, &cropcap, "VIDIOC_CROPCAP") == 0)
			printcropcap(cropcap);
	}

	if (options[OptGetCrop]) {
		struct v4l2_crop crop;

		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (doioctl(fd, VIDIOC_G_CROP, &crop, "VIDIOC_G_CROP") == 0)
			printcrop(crop);
	}

	if (options[OptGetOutputCrop]) {
		struct v4l2_crop crop;

		crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		if (doioctl(fd, VIDIOC_G_CROP, &crop, "VIDIOC_G_CROP") == 0)
			printcrop(crop);
	}

	if (options[OptGetOverlayCrop]) {
		struct v4l2_crop crop;

		crop.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
		if (doioctl(fd, VIDIOC_G_CROP, &crop, "VIDIOC_G_CROP") == 0)
			printcrop(crop);
	}

	if (options[OptGetOutputOverlayCrop]) {
		struct v4l2_crop crop;

		crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY;
		if (doioctl(fd, VIDIOC_G_CROP, &crop, "VIDIOC_G_CROP") == 0)
			printcrop(crop);
	}

	if (options[OptGetInput]) {
		if (doioctl(fd, VIDIOC_G_INPUT, &input, "VIDIOC_G_INPUT") == 0) {
			printf("Video input : %d", input);
			vin.index = input;
			if (ioctl(fd, VIDIOC_ENUMINPUT, &vin) >= 0)
				printf(" (%s)", vin.name);
			printf("\n");
		}
	}

	if (options[OptGetOutput]) {
		if (doioctl(fd, VIDIOC_G_OUTPUT, &output, "VIDIOC_G_OUTPUT") == 0) {
			printf("Video output: %d", output);
			vout.index = output;
			if (ioctl(fd, VIDIOC_ENUMOUTPUT, &vout) >= 0) {
				printf(" (%s)", vout.name);
			}
			printf("\n");
		}
	}

	if (options[OptGetAudioInput]) {
		if (doioctl(fd, VIDIOC_G_AUDIO, &vaudio, "VIDIOC_G_AUDIO") == 0)
			printf("Audio input : %d (%s)\n", vaudio.index, vaudio.name);
	}

	if (options[OptGetAudioOutput]) {
		if (doioctl(fd, VIDIOC_G_AUDOUT, &vaudout, "VIDIOC_G_AUDOUT") == 0)
			printf("Audio output: %d (%s)\n", vaudout.index, vaudout.name);
	}

	if (options[OptGetFreq]) {
		double fac = 16;

		if (doioctl(fd, VIDIOC_G_TUNER, &tuner, "VIDIOC_G_TUNER") == 0) {
			fac = (tuner.capability & V4L2_TUNER_CAP_LOW) ? 16000 : 16;
		}
		vf.tuner = 0;
		if (doioctl(fd, VIDIOC_G_FREQUENCY, &vf, "VIDIOC_G_FREQUENCY") == 0)
			printf("Frequency: %d (%f MHz)\n", vf.frequency,
					vf.frequency / fac);
	}

	if (options[OptGetStandard]) {
		if (doioctl(fd, VIDIOC_G_STD, &std, "VIDIOC_G_STD") == 0) {
			static const char *pal[] = {
				"B", "B1", "G", "H", "I", "D", "D1", "K",
				"M", "N", "Nc", "60",
				NULL
			};
			static const char *ntsc[] = {
				"M", "M-JP", "443", "M-KR",
				NULL
			};
			static const char *secam[] = {
				"B", "D", "G", "H", "K", "K1", "L", "Lc",
				NULL
			};
			static const char *atsc[] = {
				"ATSC-8-VSB", "ATSC-16-VSB",
				NULL
			};

			printf("Video Standard = 0x%08llx\n", (unsigned long long)std);
			if (std & 0xfff) {
				print_std("PAL", pal, std);
			}
			if (std & 0xf000) {
				print_std("NTSC", ntsc, std >> 12);
			}
			if (std & 0xff0000) {
				print_std("SECAM", secam, std >> 16);
			}
			if (std & 0xf000000) {
				print_std("ATSC/HDTV", atsc, std >> 24);
			}
		}
	}

	if (options[OptGetCtrl] && !get_ctrls.empty()) {
		struct v4l2_ext_controls ctrls = { 0 };

		mpeg_ctrls.clear();
		user_ctrls.clear();
		for (ctrl_get_list::iterator iter = get_ctrls.begin();
				iter != get_ctrls.end(); ++iter) {
			struct v4l2_ext_control ctrl = { 0 };

			ctrl.id = ctrl_str2id[*iter];
			if (V4L2_CTRL_ID2CLASS(ctrl.id) == V4L2_CTRL_CLASS_MPEG)
				mpeg_ctrls.push_back(ctrl);
			else
				user_ctrls.push_back(ctrl);
		}
		for (unsigned i = 0; i < user_ctrls.size(); i++) {
			struct v4l2_control ctrl;

			ctrl.id = user_ctrls[i].id;
			doioctl(fd, VIDIOC_G_CTRL, &ctrl, "VIDIOC_G_CTRL");
			printf("%s: %d\n", ctrl_id2str[ctrl.id].c_str(), ctrl.value);
		}
		if (mpeg_ctrls.size()) {
			ctrls.ctrl_class = V4L2_CTRL_CLASS_MPEG;
			ctrls.count = mpeg_ctrls.size();
			ctrls.controls = &mpeg_ctrls[0];
			doioctl(fd, VIDIOC_G_EXT_CTRLS, &ctrls, "VIDIOC_G_EXT_CTRLS");
			for (unsigned i = 0; i < mpeg_ctrls.size(); i++) {
				struct v4l2_ext_control ctrl = mpeg_ctrls[i];

				printf("%s: %d\n", ctrl_id2str[ctrl.id].c_str(), ctrl.value);
			}
		}
	}

	if (options[OptGetTuner]) {
		struct v4l2_tuner vt;
		memset(&vt, 0, sizeof(struct v4l2_tuner));
		if (doioctl(fd, VIDIOC_G_TUNER, &vt, "VIDIOC_G_TUNER") == 0) {
			printf("Tuner:\n");
			printf("\tName                 : %s\n", vt.name);
			printf("\tCapabilities         : %s\n", tcap2s(vt.capability).c_str());
			if (vt.capability & V4L2_TUNER_CAP_LOW)
				printf("\tFrequency range      : %.1f MHz - %.1f MHz\n",
				     vt.rangelow / 16000.0, vt.rangehigh / 16000.0);
			else
				printf("\tFrequency range      : %.1f MHz - %.1f MHz\n",
				     vt.rangelow / 16.0, vt.rangehigh / 16.0);
			printf("\tSignal strength/AFC  : %d%%/%d\n", (int)(vt.signal / 655.35), vt.afc);
			printf("\tCurrent audio mode   : %s\n", audmode2s(vt.audmode));
			printf("\tAvailable subchannels: %s\n",
					rxsubchans2s(vt.rxsubchans).c_str());
		}
	}

	if (options[OptLogStatus]) {
		static char buf[40960];
		int len;

		if (doioctl(fd, VIDIOC_LOG_STATUS, NULL, "VIDIOC_LOG_STATUS") == 0) {
			printf("\nStatus Log:\n\n");
			len = klogctl(3, buf, sizeof(buf) - 1);
			if (len >= 0) {
				char *p = buf;
				char *q;

				buf[len] = 0;
				while ((q = strstr(p, "START STATUS CARD #"))) {
					p = q + 1;
				}
				if (p) {
					while (p > buf && *p != '<') p--;
					q = p;
					while ((q = strstr(q, "<6>"))) {
						memcpy(q, "   ", 3);
					}
					printf("%s", p);
				}
			}
		}
	}

	/* List options */

	if (options[OptListInputs]) {
		vin.index = 0;
		printf("ioctl: VIDIOC_ENUMINPUT\n");
		while (ioctl(fd, VIDIOC_ENUMINPUT, &vin) >= 0) {
			if (vin.index)
				printf("\n");
			printf("\tInput   : %d\n", vin.index);
			printf("\tName    : %s\n", vin.name);
			printf("\tType    : 0x%08X\n", vin.type);
			printf("\tAudioset: 0x%08X\n", vin.audioset);
			printf("\tTuner   : 0x%08X\n", vin.tuner);
			printf("\tStandard: 0x%016llX ( ", (unsigned long long)vin.std);
			if (vin.std & 0x000FFF)
				printf("PAL ");	// hack
			if (vin.std & 0x00F000)
				printf("NTSC ");	// hack
			if (vin.std & 0x7F0000)
				printf("SECAM ");	// hack
			printf(")\n");
			printf("\tStatus  : %d\n", vin.status);
			vin.index++;
		}
	}

	if (options[OptListOutputs]) {
		vout.index = 0;
		printf("ioctl: VIDIOC_ENUMOUTPUT\n");
		while (ioctl(fd, VIDIOC_ENUMOUTPUT, &vout) >= 0) {
			if (vout.index)
				printf("\n");
			printf("\tOutput  : %d\n", vout.index);
			printf("\tName    : %s\n", vout.name);
			printf("\tType    : 0x%08X\n", vout.type);
			printf("\tAudioset: 0x%08X\n", vout.audioset);
			printf("\tStandard: 0x%016llX ( ", (unsigned long long)vout.std);
			if (vout.std & 0x000FFF)
				printf("PAL ");	// hack
			if (vout.std & 0x00F000)
				printf("NTSC ");	// hack
			if (vout.std & 0x7F0000)
				printf("SECAM ");	// hack
			printf(")\n");
			vout.index++;
		}
	}

	if (options[OptListAudioInputs]) {
		struct v4l2_audio vaudio;	/* list audio inputs */
		vaudio.index = 0;
		printf("ioctl: VIDIOC_ENUMAUDIO\n");
		while (ioctl(fd, VIDIOC_ENUMAUDIO, &vaudio) >= 0) {
			if (vaudio.index)
				printf("\n");
			printf("\tInput   : %d\n", vaudio.index);
			printf("\tName    : %s\n", vaudio.name);
			vaudio.index++;
		}
	}

	if (options[OptListAudioOutputs]) {
		struct v4l2_audioout vaudio;	/* list audio outputs */
		vaudio.index = 0;
		printf("ioctl: VIDIOC_ENUMAUDOUT\n");
		while (ioctl(fd, VIDIOC_ENUMAUDOUT, &vaudio) >= 0) {
			if (vaudio.index)
				printf("\n");
			printf("\tOutput  : %d\n", vaudio.index);
			printf("\tName    : %s\n", vaudio.name);
			vaudio.index++;
		}
	}

	if (options[OptListStandards]) {
		printf("ioctl: VIDIOC_ENUMSTD\n");
		vs.index = 0;
		while (ioctl(fd, VIDIOC_ENUMSTD, &vs) >= 0) {
			if (vs.index)
				printf("\n");
			printf("\tIndex       : %d\n", vs.index);
			printf("\tID          : 0x%016llX\n", (unsigned long long)vs.id);
			printf("\tName        : %s\n", vs.name);
			printf("\tFrame period: %d/%d\n",
			       vs.frameperiod.numerator,
			       vs.frameperiod.denominator);
			printf("\tFrame lines : %d\n", vs.framelines);
			vs.index++;
		}
	}

	if (options[OptListFormats]) {
		printf("ioctl: VIDIOC_ENUM_FMT\n");
		print_video_formats(fd, V4L2_BUF_TYPE_VIDEO_CAPTURE);
		print_video_formats(fd, V4L2_BUF_TYPE_VIDEO_OUTPUT);
		print_video_formats(fd, V4L2_BUF_TYPE_VIDEO_OVERLAY);
	}

	if (options[OptGetSlicedVbiCap]) {
		struct v4l2_sliced_vbi_cap cap;

		cap.type = V4L2_BUF_TYPE_SLICED_VBI_CAPTURE;
		if (doioctl(fd, VIDIOC_G_SLICED_VBI_CAP, &cap, "VIDIOC_G_SLICED_VBI_CAP") == 0) {
			print_sliced_vbi_cap(cap);
		}
	}

	if (options[OptGetSlicedVbiOutCap]) {
		struct v4l2_sliced_vbi_cap cap;

		cap.type = V4L2_BUF_TYPE_SLICED_VBI_OUTPUT;
		if (doioctl(fd, VIDIOC_G_SLICED_VBI_CAP, &cap, "VIDIOC_G_SLICED_VBI_CAP") == 0) {
			print_sliced_vbi_cap(cap);
		}
	}

	if (options[OptListCtrlsMenus]) {
		list_controls(fd, 1);
	}

	if (options[OptListCtrls]) {
		list_controls(fd, 0);
	}

	if (options[OptStreamOn]) {
		int dummy = 0;
		doioctl(fd, VIDIOC_STREAMON, &dummy, "VIDIOC_STREAMON");
	}

	close(fd);
	exit(app_result);
}
