#Optional External Renderers
USE_LIBSIXEL=0
USE_AALIB=1
USE_LIBCACA=1

#Optional video/audio playback support
USE_FFMPEG=0
USE_PORTAUDIO=0

#Optional quantizer with dither
USE_QUANTPNM=0

#Optional debug output
USE_DEBUG=0


CC=gcc
CFLAGS=
LDFLAGS=
IMG_LDFLAGS=
VID_LDFLAGS=-lavformat -lavcodec -lavutil -lswscale
DEPS=newdraw imgconvert
HDEPS=term_encode.h utf8.h stb_image.h stb_image_resize.h edge_detect.h quant.h

ifeq ($(USE_LIBSIXEL),1)
	CFLAGS+=-DUSE_LIBSIXEL
	LDFLAGS+=-lsixel -lm
else
	IMG_LDFLAGS+=-static
	LDFLAGS+=-lm
endif

ifeq ($(USE_AALIB),1)
	CFLAGS+=-DUSE_AALIB
	LDFLAGS+=-laa
endif

ifeq ($(USE_LIBCACA),1)
	CFLAGS+=-DUSE_LIBCACA
	LDFLAGS+=-lcaca -lz
endif

ifeq ($(USE_FFMPEG),1)
	DEPS+=vidconvert
endif

ifeq ($(USE_PORTAUDIO),1)
	CFLAGS+=-DUSE_PORTAUDIO
	VID_LDFLAGS+=-lportaudio
endif

ifeq ($(USE_QUANTPNM),1)
	CFLAGS+=-DUSE_QUANTPNM
	HDEPS+=quant-pnm.h
endif


ifeq ($(USE_DEBUG),1)
	CFLAGS+=-g -DDEBUG
endif

all: $(DEPS)

newdraw: newdraw.c Makefile utf8.h
	$(CC) $(CFLAGS) -o newdraw newdraw.c -static

imgconvert: imgconvert.c term_encode.c Makefile $(HDEPS)
	$(CC) $(CFLAGS) -o imgconvert imgconvert.c term_encode.c $(IMG_LDFLAGS) $(LDFLAGS)

vidconvert: vidconvert.c term_encode.c Makefile $(HDEPS)
	$(CC) $(CFLAGS) -o vidconvert vidconvert.c term_encode.c  $(VID_LDFLAGS) $(LDFLAGS)

clean:
	rm -f newdraw 
	rm -f imgconvert
	rm -f vidconvert
