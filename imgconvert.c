/*
 * Copyright (c) 2022, Daniel Tabor
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

//Implemenation block
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_THREAD_LOCALS
#include "stb_image.h"

#include "term_encode.h"


static void usage(char* cmd) {
	fprintf(stderr,"Usage:\n");
	fprintf(stderr,"%s [-h] [-sp 16|256|24 | -p # | -bw] [-w #] [-c] [-b binfile]\n",cmd);
	fprintf(stderr,"     ");
	#ifdef USE_QUANTPNM
	fprintf(stderr,"[-dither] ");
	#endif //USE_QUANTPNM
	fprintf(stderr,"[-crop x y w h]  [([-edge | -line | -glow | -hi | -apple2bw] 0xRRGGBB) | -apple2 ]\n");
	fprintf(stderr,"     renderer imgfile\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"-h     : Print usage message\n");
	fprintf(stderr,"-sp    : Use standard 16 or 256 color palette (or 24 shade grey scale)\n");
	fprintf(stderr,"-p     : Use a true color palette of # colors (<= 256)\n");
	fprintf(stderr,"-bw    : Disable colors (as possible)\n");
	fprintf(stderr,"-w     : Set the character width (terminal width used by default)\n");
	fprintf(stderr,"-c     : Clear terminal\n");
	fprintf(stderr,"-b     : Binary file to save (for newdraw)\n");
	#ifdef USE_QUANTPNM
	fprintf(stderr,"-dither: Use palette quantizer with dither\n");
	#endif //USE_QUANTPNM
	fprintf(stderr,"-crop  : Crop the image before processing\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"-= Filters =-\n");
	fprintf(stderr,"-edge     : Render edge detection (scaled) using specified color\n");
	fprintf(stderr,"-line     : Render edges as solid lines using specified color\n");
	fprintf(stderr,"-glow     : Mix edge detection (scaled) with image\n");
	fprintf(stderr,"-hi       : Mix edge line with images\n");
	fprintf(stderr,"-apple2bw : Render like an apple2 hi-res black and white  image\n");
	fprintf(stderr,"-apple2   : Render like an apple2 hi-res color image\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"-= Renderers =-\n");
	#ifdef USE_LIBSIXEL
	fprintf(stderr,"-sixel  : Sixel\n");
	#endif //USE_LIBSIXEL
	fprintf(stderr,"-simple : ANSI Simple\n");
	fprintf(stderr,"-half   : ANSI Half-Character\n");
	fprintf(stderr,"-qchar  : ANSI Quarter-Character\n");
	fprintf(stderr,"-six    : ANSI Sextant-Character\n");
	fprintf(stderr,"-bra    : ANSI Braille-Character\n");
	#ifdef USE_AALIB
	fprintf(stderr,"-aa     : ASCII Art\n");
	fprintf(stderr,"-aaext  : ASCII Art with extended characters\n");
	fprintf(stderr,"-aafg   : ASCII Art with Foreground Color\n");
	fprintf(stderr,"-aafgext: ASCII Art with Foreground Color and\n");
	fprintf(stderr,"          extended characters\n");
	fprintf(stderr,"-aabg   : ASCII Art with Background Color\n");
	fprintf(stderr,"-aabgext: ASCII Art with Background Color and\n");
	fprintf(stderr,"          extended characters\n");
	#endif //USE_AALIB
	#ifdef USE_LIBCACA
	fprintf(stderr,"-caca   : CACA (Color ASCII Art)\n");
	fprintf(stderr,"-cacablk: CACA with block characters\n");
	#endif
	fprintf(stderr,"\n");
	exit(1);
}
	
int main(int argc, char** argv) {
	size_t i;
	term_encode_t enc;
	char* imgpath = 0;
	int imgwidth;
	int imgheight;
	int imgchannels;
	
	term_encode_init(&enc);
	
	i=1;
	while( i < argc ) {
		if( strcmp(argv[i],"-h") == 0 ) {
			usage(argv[0]);
		}
		else if( strcmp(argv[i],"-sp") == 0 ) {
			if( i >= argc-1 ) {
				usage(argv[0]);
			}
			if( enc.reqpalsize ) {
				usage(argv[0]);
			}
			enc.stdpal = 1;
			i++;
			if( strcmp(argv[i],"16") == 0 ) {
				enc.reqpalsize = 16;
			}
			else if( strcmp(argv[i],"256") == 0 ) {
				enc.reqpalsize = 256;
			}
			else if( strcmp(argv[i],"24") == 0 ) {
				enc.reqpalsize = 24;
			}
			else {
				usage(argv[0]);
			}
		}
		else if( strcmp(argv[i],"-p") == 0 ) {
			if( i >= argc-1 ) {
				usage(argv[0]);
			}
			else if( enc.reqpalsize ) {
				usage(argv[0]);
			}
			else if( enc.stdpal ) {
				usage(argv[0]);
			}
			enc.reqpalsize = atoi(argv[++i]);
			if( enc.reqpalsize <= 0 || enc.reqpalsize > 256 ) {
				usage(argv[0]);
			}
		}
		else if( strcmp(argv[i],"-bw" ) == 0 ) {
			if( enc.reqpalsize ) {
				usage(argv[0]);
			}
			else if( enc.stdpal ) {
				usage(argv[0]);
			}
			enc.stdpal = 1;
			enc.reqpalsize = 0;
		}
		else if( strcmp(argv[i],"-w") == 0 ) {
			if( i >= argc-1 ) {
				usage(argv[0]);
			}
			enc.win_width = atoi(argv[++i]);
			if( enc.win_width == 0 ) {
				usage(argv[0]);
			}
		}
		else if( strcmp(argv[i],"-c") == 0 ) {
			enc.clearterm = 1;
		}
		else if( strcmp(argv[i],"-b") == 0 ) {
			if( i >= argc-1 ) {
				usage(argv[0]);
			}
			else if( enc.binaryfp != 0 ) {
				usage(argv[0]);
			}
			enc.binaryfp = fopen(argv[++i],"wb");
			if( enc.binaryfp == 0 ) {
				fprintf(stderr,"Failed to open binary files for writing.\n");
				exit(1);
			}
			enc.encbinary = 1;
		}
		#ifdef USE_QUANTPNM
		else if( strcmp(argv[i],"-dither") == 0 ) {
			enc.dither = 1;
		}
		#endif //USE_QUANTPNM
		else if( strcmp(argv[i],"-crop") == 0 ) {
			if( i >= argc-4 ) {
				usage(argv[0]);
			}
			enc.crop.x = atoi(argv[++i]);
			enc.crop.y = atoi(argv[++i]);
			enc.crop.w = atoi(argv[++i]);
			enc.crop.h = atoi(argv[++i]);
			if( enc.crop.w == 0 || enc.crop.h == 0 ) {
				usage(argv[0]);
			}
		}
		else if( strcmp(argv[i],"-edge") == 0 ) {
			if( i >= argc-1 || enc.filter != ENC_FILTER_NONE ) {
				usage(argv[0]);
			}
			enc.filter = ENC_FILTER_EDGE_SCALE;
			errno = 0;
			enc.color_rgb = strtoul(argv[++i],0,16);
			if( errno ) {
				usage(argv[0]);
			}
			if( ((enc.color_rgb>>16)&0xFF) >= 0x80 || 
				  ((enc.color_rgb>> 8)&0xFF) >= 0x80 ||
				  ((enc.color_rgb    )&0xFF) >= 0x80 ) {
				enc.invert = 0;
			} else {
				enc.invert = 1;
			}
		}
		else if( strcmp(argv[i],"-line") == 0 ) {
			if( i >= argc-1 || enc.filter != ENC_FILTER_NONE ) {
				usage(argv[0]);
			}
			enc.filter = ENC_FILTER_EDGE_LINE;
			errno = 0;
			enc.color_rgb = strtoul(argv[++i],0,16);
			if( errno ) {
				usage(argv[0]);
			}
			if( ((enc.color_rgb>>16)&0xFF) >= 0x80 || 
				  ((enc.color_rgb>> 8)&0xFF) >= 0x80 ||
				  ((enc.color_rgb    )&0xFF) >= 0x80 ) {
				enc.invert = 0;
			} else {
				enc.invert = 1;
			}
		}
		else if( strcmp(argv[i],"-glow") == 0 ) {
			if( i >= argc-1 || enc.filter != ENC_FILTER_NONE ) {
				usage(argv[0]);
			}
			enc.filter = ENC_FILTER_EDGE_GLOW;
			errno = 0;
			enc.color_rgb = strtoul(argv[++i],0,16);
			if( errno ) {
				usage(argv[0]);
			}
		}
		else if( strcmp(argv[i],"-hi") == 0 ) {
			if( i >= argc-1 || enc.filter != ENC_FILTER_NONE ) {
				usage(argv[0]);
			}
			enc.filter = ENC_FILTER_EDGE_HIGHLIGHT;
			errno = 0;
			enc.color_rgb = strtoul(argv[++i],0,16);
			if( errno ) {
				usage(argv[0]);
			}
		}
		else if( strcmp(argv[i],"-apple2bw") == 0 ) {
			if( i >= argc-1 || enc.filter != ENC_FILTER_NONE ) {
				usage(argv[0]);
			}
			enc.filter = ENC_FILTER_APPLE2_BW;
			errno = 0;
			enc.color_rgb = strtoul(argv[++i],0,16);
			if( errno ) {
				usage(argv[0]);
			}
		}
		else if( strcmp(argv[i],"-apple2") == 0 ) {
			if( enc.filter != ENC_FILTER_NONE ) {
				usage(argv[0]);
			}
			enc.filter = ENC_FILTER_APPLE2;
		}
		#ifdef USE_LIBSIXEL
		else if( strcmp(argv[i],"-sixel") == 0 ) {
			if( enc.renderer ) {
				usage(argv[0]);
			}
			enc.renderer = ENC_RENDER_SIXEL;
		}
		#endif //SIXEL
		else if( strcmp(argv[i],"-simple") == 0 ) {
			if( enc.renderer ) {
				usage(argv[0]);
			}
			enc.renderer = ENC_RENDER_SIMPLE;
		}
		else if( strcmp(argv[i],"-half") == 0 ) {
			if( enc.renderer ) {
				usage(argv[0]);
			}
			enc.renderer = ENC_RENDER_HALF;
		}
		else if( strcmp(argv[i],"-qchar") == 0 ) {
			if( enc.renderer ) {
				usage(argv[0]);
			}
			enc.renderer = ENC_RENDER_QUARTER;
		}
		else if( strcmp(argv[i],"-six") == 0 ) {
			if( enc.renderer ) {
				usage(argv[0]);
			}
			enc.renderer = ENC_RENDER_SEXTANT;
		}
		else if( strcmp(argv[i],"-bra") == 0 ) {
			if( enc.renderer ) {
				usage(argv[0]);
			}
			enc.renderer = ENC_RENDER_BRAILLE;
		}
		#ifdef USE_AALIB
		else if( strcmp(argv[i],"-aa") == 0 ) {
			if( enc.renderer ) {
				usage(argv[0]);
			}
			enc.renderer = ENC_RENDER_AA;
		}
		else if( strcmp(argv[i],"-aaext") == 0 ) {
			if( enc.renderer ) {
				usage(argv[0]);
			}
			enc.renderer = ENC_RENDER_AAEXT;
		}
		else if( strcmp(argv[i],"-aafg") == 0 ) {
			if( enc.renderer ) {
				usage(argv[0]);
			}
			enc.renderer = ENC_RENDER_AAFG;
		}
		else if( strcmp(argv[i],"-aafgext") == 0 ) {
			if( enc.renderer ) {
				usage(argv[0]);
			}
			enc.renderer = ENC_RENDER_AAFGEXT;
		}
		else if( strcmp(argv[i],"-aabg") == 0 ) {
			if( enc.renderer ) {
				usage(argv[0]);
			}
			enc.renderer = ENC_RENDER_AABG;
		}
		else if( strcmp(argv[i],"-aabgext") == 0 ) {
			if( enc.renderer ) {
				usage(argv[0]);
			}
			enc.renderer = ENC_RENDER_AABGEXT;
		}
		#endif //USE_AALIB
		#ifdef USE_LIBCACA
		else if( strcmp(argv[i],"-caca") == 0 ) {
			if( enc.renderer ) {
				usage(argv[0]);
			}
			enc.renderer = ENC_RENDER_CACA;
		}
		else if( strcmp(argv[i],"-cacablk") == 0 ) {
			if( enc.renderer ) {
				usage(argv[0]);
			}
			enc.renderer = ENC_RENDER_CACABLK;
		}
		#endif
		else {
			if( imgpath != 0 ) {
				usage(argv[0]);
			}
			imgpath = argv[i];
		}
		i++;
	}
	
	if( imgpath == 0 ) {
		fprintf(stderr,"No Image path specified\n");
		exit(1);
	}
	
	if( enc.renderer == ENC_RENDER_NONE ) {
		fprintf(stderr,"A renderer must be enabled.\n");
		exit(1);
	}
	enc.enctext = 1;
	
	if( enc.win_width == 0  && enc.renderer != ENC_RENDER_SIXEL ) {
		term_encode_detect_win_width(&enc);
	}
	
	//Load Image File
	enc.imgpixels = stbi_load(imgpath, &imgwidth, &imgheight, &imgchannels, 3);
	if( enc.imgpixels == 0 ) {
		printf("Failed to load image\n");
		exit(1);
	}
	enc.imgwidth = imgwidth;
	enc.imgheight = imgheight;
	
	term_encode(&enc);
	
	term_encode_destroy(&enc);
}