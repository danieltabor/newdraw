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
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include "term_encode.h"

//Implemenation block
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#define UTF8_IMPLEMENTATION
#define UTF8_DOS_CHARACTER_SET
#include "utf8.h"
#define QUANT_IMPLEMENTATION
#ifdef USE_QUANTPNM
#include "quant-pnm.h"
#endif
#include "quant.h"
#define EDGE_DETECT_IMPLEMENTATION
#include "edge_detect.h"

#if USE_LIBSIXEL
#include <sixel/sixel.h>
#endif

#if USE_AALIB
#include <aalib.h>
#endif

#if USE_LIBCACA
#include <caca.h>
#endif

#define ENC_FGCOLOR 0
#define ENC_BGCOLOR 1

static int standard_palette[] = {
	0x000000,0x800000,0x008000,0x808000,0x000080,0x800080,0x008080,0xc0c0c0,
	0x808080,0xff0000,0x00ff00,0xffff00,0x0000ff,0xff00ff,0x00ffff,0xffffff,
	0x000000,0x00005f,0x000087,0x0000af,0x0000d7,0x0000ff,0x005f00,0x005f5f,
	0x005f87,0x005faf,0x005fd7,0x005fff,0x008700,0x00875f,0x008787,0x0087af,
	0x0087d7,0x0087ff,0x00af00,0x00af5f,0x00af87,0x00afaf,0x00afd7,0x00afff,
	0x00d700,0x00d75f,0x00d787,0x00d7af,0x00d7d7,0x00d7ff,0x00ff00,0x00ff5f,
	0x00ff87,0x00ffaf,0x00ffd7,0x00ffff,0x5f0000,0x5f005f,0x5f0087,0x5f00af,
	0x5f00d7,0x5f00ff,0x5f5f00,0x5f5f5f,0x5f5f87,0x5f5faf,0x5f5fd7,0x5f5fff,
	0x5f8700,0x5f875f,0x5f8787,0x5f87af,0x5f87d7,0x5f87ff,0x5faf00,0x5faf5f,
	0x5faf87,0x5fafaf,0x5fafd7,0x5fafff,0x5fd700,0x5fd75f,0x5fd787,0x5fd7af,
	0x5fd7d7,0x5fd7ff,0x5fff00,0x5fff5f,0x5fff87,0x5fffaf,0x5fffd7,0x5fffff,
	0x870000,0x87005f,0x870087,0x8700af,0x8700d7,0x8700ff,0x875f00,0x875f5f,
	0x875f87,0x875faf,0x875fd7,0x875fff,0x878700,0x87875f,0x878787,0x8787af,
	0x8787d7,0x8787ff,0x87af00,0x87af5f,0x87af87,0x87afaf,0x87afd7,0x87afff,
	0x87d700,0x87d75f,0x87d787,0x87d7af,0x87d7d7,0x87d7ff,0x87ff00,0x87ff5f,
	0x87ff87,0x87ffaf,0x87ffd7,0x87ffff,0xaf0000,0xaf005f,0xaf0087,0xaf00af,
	0xaf00d7,0xaf00ff,0xaf5f00,0xaf5f5f,0xaf5f87,0xaf5faf,0xaf5fd7,0xaf5fff,
	0xaf8700,0xaf875f,0xaf8787,0xaf87af,0xaf87d7,0xaf87ff,0xafaf00,0xafaf5f,
	0xafaf87,0xafafaf,0xafafd7,0xafafff,0xafd700,0xafd75f,0xafd787,0xafd7af,
	0xafd7d7,0xafd7ff,0xafff00,0xafff5f,0xafff87,0xafffaf,0xafffd7,0xafffff,
	0xd70000,0xd7005f,0xd70087,0xd700af,0xd700d7,0xd700ff,0xd75f00,0xd75f5f,
	0xd75f87,0xd75faf,0xd75fd7,0xd75fff,0xd78700,0xd7875f,0xd78787,0xd787af,
	0xd787d7,0xd787ff,0xd7af00,0xd7af5f,0xd7af87,0xd7afaf,0xd7afd7,0xd7afff,
	0xd7d700,0xd7d75f,0xd7d787,0xd7d7af,0xd7d7d7,0xd7d7ff,0xd7ff00,0xd7ff5f,
	0xd7ff87,0xd7ffaf,0xd7ffd7,0xd7ffff,0xff0000,0xff005f,0xff0087,0xff00af,
	0xff00d7,0xff00ff,0xff5f00,0xff5f5f,0xff5f87,0xff5faf,0xff5fd7,0xff5fff,
	0xff8700,0xff875f,0xff8787,0xff87af,0xff87d7,0xff87ff,0xffaf00,0xffaf5f,
	0xffaf87,0xffafaf,0xffafd7,0xffafff,0xffd700,0xffd75f,0xffd787,0xffd7af,
	0xffd7d7,0xffd7ff,0xffff00,0xffff5f,0xffff87,0xffffaf,0xffffd7,0xffffff,
	0x080808,0x121212,0x1c1c1c,0x262626,0x303030,0x3a3a3a,0x444444,0x4e4e4e,
	0x585858,0x626262,0x6c6c6c,0x767676,0x808080,0x8a8a8a,0x949494,0x9e9e9e,
	0xa8a8a8,0xb2b2b2,0xbcbcbc,0xc6c6c6,0xd0d0d0,0xdadada,0xe4e4e4,0xeeeeee};
static uint8_t fg16codes[] = {30, 31, 32, 33, 34, 35, 36, 37,  90,  91,  92,  93,  94,  95,  96,  97};
static uint8_t bg16codes[] = {40, 41, 42, 43, 44, 45, 46, 47, 100, 101, 102, 103, 104, 105, 106, 107};


static size_t findStdColorRGB(size_t palsize, uint32_t rgb) {
	size_t i;
	rgb = rgb&0xFFFFFF;
	if( palsize == 24 ) {
		palsize = 256;
	}
	for( i=0; i<palsize; i++ ) {
		if( standard_palette[i] == rgb ) {
			return i;
		}
	}
	fprintf(stderr,"Failed to encode standard palette color %06X with palette size %lu\n",rgb,palsize);
	exit(1);
}

static ssize_t findStdColor(size_t palsize, uint8_t r, uint8_t g, uint8_t b ) {
	size_t i;
	uint32_t rgb = (r<<16)|(g<<8)|b;
	return findStdColorRGB(palsize,rgb);
}

static void ansiSetStdColor( term_encode_t* enc, uint8_t color_type, size_t color_idx ) {
	if( enc->palsize == 16 ) {
		if( color_type == ENC_FGCOLOR ) {
			fprintf(enc->textfp,"\x1b[%dm",fg16codes[color_idx]);
		} else { //color_type == ENC_BGCOLOR
			fprintf(enc->textfp,"\x1b[%dm",bg16codes[color_idx]);
		}
	} else { // enc->palsize == 256 (or 24, which used 256 color encoding)
		if( color_type == ENC_FGCOLOR ) {
			fprintf(enc->textfp,"\x1b[38;5;%ldm",color_idx);
		} else { //color_type == ENC_BGCOLOR
			fprintf(enc->textfp,"\x1b[48;5;%ldm",color_idx);
		}
	}
}

static void ansiSetColorRGB( term_encode_t* enc, uint8_t color_type, uint32_t rgb ) {
	ssize_t color_idx;
	uint8_t r,g,b;
	if( enc->stdpal ) {
		color_idx = findStdColorRGB(enc->palsize,rgb);
		ansiSetStdColor(enc,color_type,color_idx);
	}
	else {
		r = (rgb>>16)&0xFF;
		g = (rgb>>8)&0xFF;
		b = rgb&0xFF;
		if( color_type == ENC_FGCOLOR ) {
			fprintf(enc->textfp,"\x1b[38;2;%d;%d;%dm",r,g,b);
		}
		else if( color_type == ENC_BGCOLOR ) {
			fprintf(enc->textfp,"\x1b[48;2;%d;%d;%dm",r,g,b);
		}
	}
}

static void ansiSetColor( term_encode_t* enc, uint8_t color_type, uint8_t r, uint8_t g, uint8_t b ) {
	size_t color_idx;
	if( enc->stdpal ) {
		color_idx = findStdColor(enc->palsize,r,g,b);
		ansiSetStdColor(enc,color_type,color_idx);
	}
	else {
		if( color_type == ENC_FGCOLOR ) {
			fprintf(enc->textfp,"\x1b[38;2;%d;%d;%dm",r,g,b);
		}
		else if( color_type == ENC_BGCOLOR ) {
			fprintf(enc->textfp,"\x1b[48;2;%d;%d;%dm",r,g,b);
		}
	}
}

static void binWriteHeader( term_encode_t *enc, size_t width, size_t height ) {
	uint32_t tmp;
	uint8_t color_mode;
	
	fprintf(enc->binaryfp,"newdraw");
	if( width == 0 ) {
		width = enc->width;
	}
	tmp = htonl(width);
	fwrite(&tmp,4,1,enc->binaryfp);
	if( height == 0 ) {
		height = enc->height;
	}
	tmp = htonl(height);
	fwrite(&tmp,4,1,enc->binaryfp);
	if( enc->stdpal ) {
		if( enc->palsize == 16 ) {
			color_mode = 0;
		}
		else if( enc->palsize == 256 ) {
			color_mode = 1;
		}
		else if( enc->palsize == 0 ) {
			color_mode = 3;
		}
	}
	else {
		color_mode = 2;
	}
	fwrite(&color_mode,1,1,enc->binaryfp);
}

static void binWriteCellIndex(term_encode_t *enc,
		uint32_t fg_index, uint32_t bg_index,
		uint8_t reverse, uint8_t blink, uint8_t bold, uint8_t underline,
		uint32_t character) {
	uint32_t tmp;
	uint8_t tmp8;
	if( ! enc->stdpal ) {
		fprintf(stderr,"Attempted to write indexed color to not palette binary file\n");
		exit(1);
	}
	tmp = htonl(fg_index);
	fwrite(&tmp,4,1,enc->binaryfp);
	tmp = htonl(bg_index);
	fwrite(&tmp,4,1,enc->binaryfp);
	fwrite(&reverse,1,1,enc->binaryfp);
	fwrite(&blink,1,1,enc->binaryfp);
	fwrite(&bold,1,1,enc->binaryfp);
	fwrite(&underline,1,1,enc->binaryfp);
	tmp8 = 0;
	fwrite(&tmp8,1,1,enc->binaryfp); //Line Type
	tmp = htonl(character);
	fwrite(&tmp,4,1,enc->binaryfp);
}

static void binWriteCellRGB(term_encode_t *enc, 
		uint32_t fg_rgb, uint32_t bg_rgb, 
		uint8_t reverse, uint8_t blink, uint8_t bold, uint8_t underline,
		uint32_t character) {
	uint32_t tmp;
	uint8_t tmp8;
	size_t fg_idx, bg_idx;
	
	if( enc->stdpal ) {
		if( enc->palsize ) {
			fg_idx = findStdColorRGB(enc->palsize,fg_rgb);
			bg_idx = findStdColorRGB(enc->palsize,bg_rgb);
		}
		else {
			fg_idx = 15;
			bg_idx = 0;
		}
		binWriteCellIndex(enc, fg_idx, bg_idx,
			reverse,blink,bold,underline, character);
	} else {
		tmp = htonl(fg_rgb);
		fwrite(&tmp,4,1,enc->binaryfp);
		tmp = htonl(bg_rgb);
		fwrite(&tmp,4,1,enc->binaryfp);
		fwrite(&reverse,1,1,enc->binaryfp);
		fwrite(&blink,1,1,enc->binaryfp);
		fwrite(&bold,1,1,enc->binaryfp);
		fwrite(&underline,1,1,enc->binaryfp);
		tmp8 = 0;
		fwrite(&tmp8,1,1,enc->binaryfp); //Line Type
		tmp = htonl(character);
		fwrite(&tmp,4,1,enc->binaryfp);
	}
}

static void binWriteCell(term_encode_t* enc,
		uint8_t fg_red, uint8_t fg_green, uint8_t fg_blue,
		uint8_t bg_red, uint8_t bg_green, uint8_t bg_blue,
		uint8_t reverse, uint8_t blink, uint8_t bold, uint8_t underline,
		uint32_t character ) {
	uint32_t fg_rgb;
	uint32_t bg_rgb;
	
	fg_rgb = (fg_red<<16) | (fg_green<<8) | (fg_blue); //ENC_FGCOLOR
	bg_rgb = (bg_red<<16) | (bg_green<<8) | (bg_blue); //ENC_BGCOLOR
	binWriteCellRGB(enc,fg_rgb,bg_rgb,reverse,blink,bold,underline,character);
}

static void binClose(term_encode_t* enc) {
	fclose(enc->binaryfp);
	enc->binaryfp = 0;
}

static uint16_t simple_chars[16] = { 
	0x0020, 0x2588
};
static void ansiEncodeSimple( term_encode_t* enc ) {
	uint8_t *prgb;
	int last_bg_rgb;
	int bg_rgb;
	uint8_t r, g, b;
	size_t x;
	size_t y;
	uint32_t binchar;
	
	uint8_t bw = (enc->stdpal) && (enc->palsize == 0);
	
	if( enc->encbinary ) {
		binWriteHeader(enc,0,0);
	}
	
	if( enc->enctext ) {
		fprintf(enc->textfp,"\x1b[0m");
	}
	for( y=0; y<enc->height; y++ ) {
		last_bg_rgb = -1;
		for( x=0; x<enc->width; x++ ) {
			if( bw ) {
				binchar = simple_chars[enc->rgbpixels[3*(y*enc->width+x)]&1];
			}
			else {
				prgb = &(enc->rgbpixels[3*(y*enc->width+x)]);
				r = *(prgb);
				g = *(++prgb);
				b = *(++prgb);
				bg_rgb = (r<<16)|(g<<8)|(b);
				binchar = simple_chars[0];
			}
			
			if( enc->enctext ) {
				if( !bw ) {
					if( last_bg_rgb != bg_rgb ) {
						ansiSetColorRGB(enc,ENC_BGCOLOR,bg_rgb);
						last_bg_rgb = bg_rgb;
					}
				}
				fprintf(enc->textfp,"%s",utf8_encode(0,binchar));
			}
			if( enc->encbinary ) {
				binWriteCellRGB(enc,0x00FFFFFF,bg_rgb,0,0,0,0,binchar);
			}
		}
		if( enc->enctext ) {
			fprintf(enc->textfp,"\x1b[0m\r\n");
		}
	}
	
	if( enc->encbinary ) {
		binClose(enc);
	}
}

static uint16_t halfheight_chars[16] = { 
	0x0020, 0x2584, 0x2580, 0x2588
};
static void ansiEncodeHalfHeight( term_encode_t* enc ) {
	uint8_t *prgb;
	int last_fg_rgb;
	int last_bg_rgb;
	int fg_rgb;
	int bg_rgb;
	
	int r, g, b;
	size_t x;
	size_t hy,y;
	uint32_t binchar;
	
	uint8_t last_idx;
	uint8_t idx = 1;
	
	uint8_t bw = (enc->stdpal) && (enc->palsize == 0);
	
	if( enc->encbinary ) {
		binWriteHeader(enc,enc->width,enc->height/2);
	}
	
	if( enc->enctext ) {
		fprintf(enc->textfp,"\x1b[0m");
	}
	
	for( hy=0; hy<enc->height/2; hy++ ) {
		y = hy*2;
		last_fg_rgb = -1;
		last_bg_rgb = -1;
		for( x=0; x<enc->width; x++ ) {
			if( bw ) {
				idx = (enc->rgbpixels[3*(y*enc->width+x)]&1);
				idx = (idx<<1) | (enc->rgbpixels[3*((y+1)*enc->width+x)]&1);
				binchar = halfheight_chars[idx];
				fg_rgb = 0xFFFFFF;
				bg_rgb = 0x000000;
			}
			else {
				prgb = &(enc->rgbpixels[3*(y*enc->width+x)]);
				r = *(prgb);
				g = *(++prgb);
				b = *(++prgb);
				bg_rgb = (r<<16)|(g<<8)|(b);
				prgb = &(enc->rgbpixels[3*((y+1)*enc->width+x)]);
				r = *(prgb);
				g = *(++prgb);
				b = *(++prgb);
				fg_rgb = (r<<16)|(g<<8)|(b);
				idx = 1;
				binchar = halfheight_chars[idx];
			}

			if( enc->enctext ) {
				if( !bw ) {
					if( last_fg_rgb != fg_rgb ) {
						ansiSetColorRGB(enc,ENC_FGCOLOR,fg_rgb);
						last_fg_rgb = fg_rgb;
					}
					if( last_bg_rgb != bg_rgb ) {
						ansiSetColorRGB(enc,ENC_BGCOLOR,bg_rgb);
						last_bg_rgb = bg_rgb;
					}
				}
				fprintf(enc->textfp,"%s",utf8_encode(0,binchar));
			}
			if( enc->encbinary ) {
				binWriteCellRGB(enc,fg_rgb,bg_rgb,0,0,0,0,binchar);
			}
		}
		if( enc->enctext ) {
			fprintf(enc->textfp,"\x1b[0m\r\n");
		}
	}
	
	if( enc->encbinary ) {
		binClose(enc);
	}
}

static uint16_t quarter_chars[16] = { 
	0x0020, 0x2597, 0x2596, 0x2584, 0x259D, 0x2590, 0x259E, 0x259F, 
	0x2598, 0x259A, 0x258C, 0x2599, 0x2580, 0x259C, 0x259B, 0x2588 
};
static void ansiEncodeQuarter( term_encode_t* enc ) {
	uint8_t *prgb;
	int last_fg_rgb;
	int last_bg_rgb;
	int fg_rgb;
	int bg_rgb;
	
	int r, g, b;
	size_t hx,x;
	size_t hy,y;
	uint32_t binchar;
	
	uint8_t pal2[6];
	size_t pal2size;
	uint8_t pal4pixels[4];
	uint8_t rgb4pixels[12];
	uint8_t *prgb4;
	
	uint8_t idx;
	
	uint8_t bw = (enc->stdpal) && (enc->palsize == 0);
	
	if( enc->encbinary ) {
		binWriteHeader(enc,enc->width/2,enc->height/2);
	}
	
	if( enc->enctext ) {
		fprintf(enc->textfp,"\x1b[0m");
	}
	
	for( hy=0; hy<enc->height/2; hy++ ) {
		y = hy*2;
		last_fg_rgb = -1;
		last_bg_rgb = -1;
		for( hx=0; hx<enc->width/2; hx++ ) {
			x = hx*2;
			
			if( bw ) {
				idx = (enc->rgbpixels[3*(y*enc->width+x)]&1);
				idx = (idx<<1) | (enc->rgbpixels[3*(y*enc->width+(x+1))]&1);
				idx = (idx<<1) | (enc->rgbpixels[3*((y+1)*enc->width+x)]&1);
				idx = (idx<<1) | (enc->rgbpixels[3*((y+1)*enc->width+(x+1))]&1);
				binchar = quarter_chars[idx];
				fg_rgb = 0xFFFFFF;
				bg_rgb = 0x000000;
			}
			else {
				//rgb x=0 y=0
				prgb = &(enc->rgbpixels[3*(y*enc->width+x)]);
				prgb4 = &(rgb4pixels[0]);
				*(prgb4)   = *(prgb);
				*(++prgb4) = *(++prgb);
				*(++prgb4) = *(++prgb);
				//rgb x=1 y=0
				*(++prgb4) = *(++prgb);
				*(++prgb4) = *(++prgb);
				*(++prgb4) = *(++prgb);
				//rgb x=0 y=1
				prgb = &(enc->rgbpixels[3*((y+1)*enc->width+x)]);
				*(++prgb4)   = *(prgb);
				*(++prgb4) = *(++prgb);
				*(++prgb4) = *(++prgb);
				//rgb x=1 y=1
				*(++prgb4) = *(++prgb);
				*(++prgb4) = *(++prgb);
				*(++prgb4) = *(++prgb);

				//Quatize this block down to 2 colors
				pal2size = 2;
				quant_quantize(pal2,&pal2size,
						pal4pixels, rgb4pixels, 4, 0);
				
				idx = (pal4pixels[0]<<3) | (pal4pixels[1]<<2) | (pal4pixels[2]<<1) | pal4pixels[3];
				binchar = quarter_chars[idx];
				prgb = pal2;
				r = *(prgb);
				g = *(++prgb);
				b = *(++prgb);
				bg_rgb = (r<<16)|(g<<8)|(b);
				if( pal2size > 1 ) {
					r = *(++prgb);
					g = *(++prgb);
					b = *(++prgb);
					fg_rgb = (r<<16)|(g<<8)|(b);
				} else {
					fg_rgb = last_fg_rgb;
				}
			}

			if( enc->enctext ) {
				if( !bw ) {
					if( last_fg_rgb != fg_rgb ) {
						ansiSetColorRGB(enc,ENC_FGCOLOR,fg_rgb);
						last_fg_rgb = fg_rgb;
					}
					if( last_bg_rgb != bg_rgb ) {
						ansiSetColorRGB(enc,ENC_BGCOLOR,bg_rgb);
						last_bg_rgb = bg_rgb;
					}
				}
				fprintf(enc->textfp,"%s",utf8_encode(0,binchar));
			}
			if( enc->encbinary ) {
				binWriteCellRGB(enc,fg_rgb,bg_rgb,0,0,0,0,binchar);
			}
		}
		if( enc->enctext ) {
			fprintf(enc->textfp,"\x1b[0m\r\n");
		}
	}
	
	if( enc->encbinary ) {
		binClose(enc);
	}
}

static uint32_t sextant_chars[64] = {
	0x00020,0x1FB1E,0x1FB0F,0x1FB2D,0x1FB07,0x1FB26,0x1FB16,0x1FB35,
	0x1FB03,0x1FB22,0x1FB13,0x1FB31,0x1FB0B,0x1FB29,0x1FB1A,0x1FB39,
	0x1FB01,0x1FB20,0x1FB11,0x1FB2F,0x1FB09,0x02590,0x1FB18,0x1FB37,
	0x1FB05,0x1FB24,0x1FB14,0x1FB33,0x1FB0D,0x1FB2B,0x1FB1C,0x1FB3B,
	0x1FB00,0x1FB1F,0x1FB10,0x1FB2E,0x1FB08,0x1FB27,0x1FB17,0x1FB36,
	0x1FB04,0x1FB23,0x0258C,0x1FB32,0x1FB0C,0x1FB2A,0x1FB1B,0x1FB3A,
	0x1FB02,0x1FB21,0x1FB12,0x1FB30,0x1FB0A,0x1FB28,0x1FB19,0x1FB38,
	0x1FB06,0x1FB25,0x1FB15,0x1FB34,0x1FB0E,0x1FB2C,0x1FB1D,0x02588
};
static void ansiEncodeSextant( term_encode_t* enc ) {
	uint8_t *prgb;
	int last_fg_rgb;
	int last_bg_rgb;
	int fg_rgb;
	int bg_rgb;
	
	int r, g, b;
	size_t hx,x;
	size_t hy,y;
	uint32_t binchar;
	
	uint8_t pal2[6];
	size_t pal2size;
	uint8_t pal6pixels[6];
	uint8_t rgb6pixels[18];
	uint8_t *prgb6;
	
	uint8_t idx;
	
	uint8_t bw = (enc->stdpal) && (enc->palsize == 0);
	
	if( enc->encbinary ) {
		binWriteHeader(enc,enc->width/2,enc->height/3);
	}
	
	if( enc->enctext ) {
		fprintf(enc->textfp,"\x1b[0m");
	}
	for( hy=0; hy<enc->height/3; hy++ ) {
		y = hy*3;
		last_fg_rgb = -1;
		last_bg_rgb = -1;
		for( hx=0; hx<enc->width/2; hx++ ) {
			x = hx*2;
			
			if( bw ) {
				idx = (enc->rgbpixels[3*(y*enc->width+x)]&1);
				idx = (idx<<1) | (enc->rgbpixels[3*(y*enc->width+(x+1))]&1);
				idx = (idx<<1) | (enc->rgbpixels[3*((y+1)*enc->width+x)]&1);
				idx = (idx<<1) | (enc->rgbpixels[3*((y+1)*enc->width+(x+1))]&1);
				idx = (idx<<1) | (enc->rgbpixels[3*((y+2)*enc->width+x)]&1);
				idx = (idx<<1) | (enc->rgbpixels[3*((y+2)*enc->width+(x+1))]&1);
				binchar = sextant_chars[idx];
				fg_rgb = 0xFFFFFF;
				bg_rgb = 0x000000;
			}
			else {
				//rgb x=0 y=0
				prgb = &(enc->rgbpixels[3*(y*enc->width+x)]);
				prgb6 = &(rgb6pixels[0]);
				*(prgb6)   = *(prgb);
				*(++prgb6) = *(++prgb);
				*(++prgb6) = *(++prgb);
				//rgb x=1 y=0
				*(++prgb6) = *(++prgb);
				*(++prgb6) = *(++prgb);
				*(++prgb6) = *(++prgb);
				//rgb x=0 y=1
				prgb = &(enc->rgbpixels[3*((y+1)*enc->width+x)]);
				*(++prgb6)   = *(prgb);
				*(++prgb6) = *(++prgb);
				*(++prgb6) = *(++prgb);
				//rgb x=1 y=1
				*(++prgb6) = *(++prgb);
				*(++prgb6) = *(++prgb);
				*(++prgb6) = *(++prgb);
				//rgb x=0 y=2
				prgb = &(enc->rgbpixels[3*((y+2)*enc->width+x)]);
				*(++prgb6)   = *(prgb);
				*(++prgb6) = *(++prgb);
				*(++prgb6) = *(++prgb);
				//rgb x=1 y=2
				*(++prgb6) = *(++prgb);
				*(++prgb6) = *(++prgb);
				*(++prgb6) = *(++prgb);

				//Quatize this block down to 2 colors
				pal2size = 2;
				quant_quantize(pal2,&pal2size,
							   pal6pixels, rgb6pixels, 6, 0);
				idx = (pal6pixels[0]<<5) | (pal6pixels[1]<<4) | (pal6pixels[2]<<3) | (pal6pixels[3]<<2) | (pal6pixels[4]<<1) | pal6pixels[5];
				binchar = sextant_chars[idx];
				prgb = pal2;
				r = *(prgb);
				g = *(++prgb);
				b = *(++prgb);
				bg_rgb = (r<<16)|(g<<8)|(b);
				if( pal2size > 1 ) {
					r = *(++prgb);
					g = *(++prgb);
					b = *(++prgb);
					fg_rgb = (r<<16)|(g<<8)|(b);
				} else {
					fg_rgb = last_fg_rgb;
				}
			}

			if( enc->enctext ) {
				if( !bw ) {
					if( last_fg_rgb != fg_rgb ) {
						ansiSetColorRGB(enc,ENC_FGCOLOR,fg_rgb);
						last_fg_rgb = fg_rgb;
					}
					if( last_bg_rgb != bg_rgb ) {
						ansiSetColorRGB(enc,ENC_BGCOLOR,bg_rgb);
						last_bg_rgb = bg_rgb;
					}
				}
				fprintf(enc->textfp,"%s",utf8_encode(0,binchar));
			}
			if( enc->encbinary ) {
				binWriteCellRGB(enc,fg_rgb,bg_rgb,0,0,0,0,binchar);
			}
		}
		if( enc->enctext ) {
			fprintf(enc->textfp,"\x1b[0m\r\n");
		}
	}
	
	if( enc->encbinary ) {
		binClose(enc);
	}
}

static uint16_t braille_chars[256] = {
	0x2800, 0x2880, 0x2840, 0x28C0, 0x2820, 0x28A0, 0x2860, 0x28E0,
	0x2804, 0x2884, 0x2844, 0x28C4, 0x2824, 0x28A4, 0x2864, 0x28E4,	
	0x2810, 0x2890, 0x2850, 0x28D0, 0x2830, 0x28B0, 0x2870, 0x28F0,
	0x2814, 0x2894, 0x2854, 0x28D4, 0x2834, 0x28B4, 0x2874, 0x28F4,
	0x2802, 0x2882, 0x2842, 0x28C2, 0x2822, 0x28A2, 0x2862, 0x28E2,
	0x2806, 0x2886, 0x2846, 0x28C6, 0x2826, 0x28A6, 0x2866, 0x28E6,
	0x2812, 0x2892, 0x2852, 0x28D2, 0x2832, 0x28B2, 0x2872, 0x28F2,
	0x2816, 0x2896, 0x2856, 0x28D6, 0x2836, 0x28B6, 0x2876, 0x28F6,
	0x2808, 0x2888, 0x2848, 0x28C8, 0x2828, 0x28A8, 0x2868, 0x28E8,
	0x280C, 0x288C, 0x284C, 0x28CC, 0x282C, 0x28AC, 0x286C, 0x28EC,
	0x2818, 0x2898, 0x2858, 0x28D8, 0x2838, 0x28B8, 0x2878, 0x28F8,
	0x281C, 0x289C, 0x285C, 0x28DC, 0x283C, 0x28BC, 0x287C, 0x28FC,
	0x280A, 0x288A, 0x284A, 0x28CA, 0x282A, 0x28AA, 0x286A, 0x28EA,
	0x280E, 0x288E, 0x284E, 0x28CE, 0x282E, 0x28AE, 0x286E, 0x28EE,
	0x281A, 0x289A, 0x285A, 0x28DA, 0x283A, 0x28BA, 0x287A, 0x28FA,
	0x281E, 0x289E, 0x285E, 0x28DE, 0x283E, 0x28BE, 0x287E, 0x28FE,
	0x2801, 0x2881, 0x2841, 0x28C1, 0x2821, 0x28A1, 0x2861, 0x28E1,
	0x2805, 0x2885, 0x2845, 0x28C5, 0x2825, 0x28A5, 0x2865, 0x28E5,
	0x2811, 0x2891, 0x2851, 0x28D1, 0x2831, 0x28B1, 0x2871, 0x28F1,
	0x2815, 0x2895, 0x2855, 0x28D5, 0x2835, 0x28B5, 0x2875, 0x28F5,
	0x2803, 0x2883, 0x2843, 0x28C3, 0x2823, 0x28A3, 0x2863, 0x28E3,
	0x2807, 0x2887, 0x2847, 0x28C7, 0x2827, 0x28A7, 0x2867, 0x28E7,
	0x2813, 0x2893, 0x2853, 0x28D3, 0x2833, 0x28B3, 0x2873, 0x28F3,
	0x2817, 0x2897, 0x2857, 0x28D7, 0x2837, 0x28B7, 0x2877, 0x28F7,
	0x2809, 0x2889, 0x2849, 0x28C9, 0x2829, 0x28A9, 0x2869, 0x28E9,
	0x280D, 0x288D, 0x284D, 0x28CD, 0x282D, 0x28AD, 0x286D, 0x28ED,
	0x2819, 0x2899, 0x2859, 0x28D9, 0x2839, 0x28B9, 0x2879, 0x28F9,
	0x281D, 0x289D, 0x285D, 0x28DD, 0x283D, 0x28BD, 0x287D, 0x28FD,
	0x280B, 0x288B, 0x284B, 0x28CB, 0x282B, 0x28AB, 0x286B, 0x28EB,
	0x280F, 0x288F, 0x284F, 0x28CF, 0x282F, 0x28AF, 0x286F, 0x28EF,
	0x281B, 0x289B, 0x285B, 0x28DB, 0x283B, 0x28BB, 0x287B, 0x28FB,
	0x281F, 0x289F, 0x285F, 0x28DF, 0x283F, 0x28BF, 0x287F, 0x28FF,
};
static void ansiEncodeBraille( term_encode_t* enc ) {
	uint8_t *prgb;
	int last_rgb;
	int rgb;

	int r, g, b;
	size_t hx,x;
	size_t hy,y;
	uint32_t binchar;
	
	uint8_t *bwpixels;
	uint8_t idx;
	uint8_t bw = (enc->stdpal) && (enc->palsize == 0);
	
	bwpixels = (uint8_t*)malloc(sizeof(uint8_t)*enc->width*enc->height);
	if( bwpixels == 0 ) {
		fprintf(enc->textfp,"Failed allocate space for black and white pixels\n");
	}
	quant_bw(bwpixels,enc->rgbpixels,enc->width*enc->height,0);
	
	if( enc->encbinary ) {
		binWriteHeader(enc,enc->width/2,enc->height/3);
	}
	
	if( enc->enctext ) {
		fprintf(enc->textfp,"\x1b[0m");
		if( ! bw ) {
			ansiSetColorRGB(enc,ENC_BGCOLOR,0);
		}
	}
	for( hy=0; hy<enc->height/4; hy++ ) {
		y = hy*4;
		last_rgb = -1;
		for( hx=0; hx<enc->width/2; hx++ ) {
			x = hx*2;
			
			idx = (bwpixels[y*enc->width+x]&1);
			idx = (idx<<1) | (bwpixels[y*enc->width+(x+1)]&1);
			idx = (idx<<1) | (bwpixels[(y+1)*enc->width+x]&1);
			idx = (idx<<1) | (bwpixels[(y+1)*enc->width+(x+1)]&1);
			idx = (idx<<1) | (bwpixels[(y+2)*enc->width+x]&1);
			idx = (idx<<1) | (bwpixels[(y+2)*enc->width+(x+1)]&1);
			idx = (idx<<1) | (bwpixels[(y+3)*enc->width+x]&1);
			idx = (idx<<1) | (bwpixels[(y+3)*enc->width+(x+1)]&1);
			binchar = braille_chars[idx];
			
			if( !bw ) {
				prgb = &(enc->rgbpixels[3*(y*enc->width+x)]);
				r = *(prgb);
				g = *(++prgb);
				b = *(++prgb);
				if( !enc->palsize ) {
					r = ( r + *(++prgb));
					g = ( g + *(++prgb));
					b = ( b + *(++prgb));
					prgb = &(enc->rgbpixels[3*((y+1)*enc->width+x)]);
					r = ( r + *(prgb));
					g = ( g + *(++prgb));
					b = ( b + *(++prgb));
					r = ( r + *(++prgb));
					g = ( g + *(++prgb));
					b = ( b + *(++prgb));
					prgb = &(enc->rgbpixels[3*((y+2)*enc->width+x)]);
					r = ( r + *(prgb));
					g = ( g + *(++prgb));
					b = ( b + *(++prgb));
					r = ( r + *(++prgb));
					g = ( g + *(++prgb));
					b = ( b + *(++prgb));
					prgb = &(enc->rgbpixels[3*((y+3)*enc->width+x)]);
					r = ( r + *(prgb));
					g = ( g + *(++prgb));
					b = ( b + *(++prgb));
					r = ( r + *(++prgb));
					g = ( g + *(++prgb));
					b = ( b + *(++prgb));
					r = r / 8;
					g = g / 8;
					b = b / 8;
				}
				rgb = (r<<16)|(g<<8)|(b);
			}

			if( enc->enctext ) {
				if( !bw ) {
					if( last_rgb != rgb ) {
						ansiSetColorRGB(enc,ENC_FGCOLOR,rgb);
						last_rgb = rgb;
					}
				}
				fprintf(enc->textfp,"%s",utf8_encode(0,binchar));
			}
			if( enc->encbinary ) {
				binWriteCellRGB(enc,rgb,0,0,0,0,0,binchar);
			}
		}
		if( enc->enctext ) {
			fprintf(enc->textfp,"\x1b[0m\r\n");
		}
	}
	
	if( enc->encbinary ) {
		binClose(enc);
	}
	
	free(bwpixels);
}
	
#ifdef USE_AALIB
static aa_context* aaRender( term_encode_t* enc, size_t* char_width, size_t* char_height, int supported) {
	aa_context *aa;
	struct aa_hardware_params hwparams;
	aa_renderparams rparams; 
 	aa_palette palette;
	uint8_t  *prgb;
	int r,g,b;
	size_t i, x, y;
	size_t cwidth = (enc->width/2)+(enc->width%2);
	size_t cheight = (enc->height/2)+(enc->height%2);
	
	memset(&hwparams,0,sizeof(hwparams));
	hwparams.font = &aa_font16;
	if( supported == 0 ) {
		hwparams.supported = AA_NORMAL_MASK;
	} else {
		hwparams.supported = supported;
	}
	hwparams.width = cwidth;
	hwparams.height = cheight;
	
	memset(&rparams,0,sizeof(rparams));
	rparams.gamma = 1.0;
	
	aa = aa_init(&mem_d,&hwparams,0);
	if( aa == 0 ) {
		fprintf(stderr,"Failed to initialize aalib\n");
		return 0;
	}
	
	if( enc->palsize ) {
		for( y=0; y<enc->height; y++ ) {
			for( x=0; x<enc->width; x++ ) {
				aa_putpixel(aa,x,y,enc->palpixels[y*enc->width+x]);
			}
		}
		memset(palette,0,sizeof(palette));
		for( i=0; i<enc->palsize; i++ ) {
			aa_setpalette(palette,i,
				enc->palette[(3*i)],
				enc->palette[(3*i)+1],
				enc->palette[(3*i)+2]);
		}
		aa_renderpalette(aa, palette, &rparams, 0, 0, cwidth, cheight);
	} else {
		for( y=0; y<enc->height; y++ ) {
			for( x=0; x<enc->width; x++ ) {
				prgb = &(enc->rgbpixels[3*(y*enc->width+x)]);
				r = *(prgb);
				g = *(++prgb);
				b = *(++prgb);
				aa_putpixel(aa,x,y, (((r)*30+(g)*59+(b)*11)>>8) );
			}
		}
		aa_render(aa, &rparams, 0, 0, cwidth, cheight);
	}
	if( char_width ) {
		*char_width = cwidth;
	}
	if( char_height ) {
		*char_height = cheight;
	}
	return aa;
}

static int asciiEncode( term_encode_t* enc, int extended) {
	size_t x,y;
	uint8_t attr, last_attr;
	uint8_t reverse, bold;
	uint16_t c;
	char* utf8c;
	size_t char_width;
	size_t char_height;
	aa_context *aa;
	int supported = AA_NORMAL_MASK|AA_BOLD_MASK|AA_REVERSE_MASK;
	
	if( extended ) {
		supported |= AA_EXTENDED;
	}
	aa = aaRender(enc,&char_width,&char_height,supported);
	if( aa == 0 ) { return 1; }
	
	if( enc->encbinary ) {
		enc->stdpal = 1;
		enc->palsize = 0;
		binWriteHeader(enc,char_width,char_height);
	}
	
	for( y=0; y<char_height; y++ ) {
	last_attr = AA_NORMAL;
		for( x=0; x<char_width; x++ ) {
			attr = aa->attrbuffer[y*char_width+x];
			c = cp437[aa->textbuffer[y*char_width+x]];
			utf8c = utf8_encode(0,c);
			reverse = (attr == AA_REVERSE);
			bold = (attr == AA_BOLD);
			if( enc->enctext ) {
				if( attr != last_attr ) {
					fprintf(enc->textfp,"\x1b[0m");
					if( reverse ) {
						fprintf(enc->textfp,"\x1b[7m");
					}
					else if( bold ) {
						fprintf(enc->textfp,"\x1b[1m");
					}
					last_attr = attr;
				}
				fprintf(enc->textfp,"%s",utf8c);
			}
			if( enc->encbinary ) {
				binWriteCellRGB(enc,0x00FFFFFF,0x000000,
					reverse,0,bold,0, c);
			}
		}
		if( enc->enctext ) {
			fprintf(enc->textfp,"\x1b[0m\r\n");
		}
	}
	fflush(stdout);
	
	if( enc->encbinary ) {
		binClose(enc);
	}
	return 0;
}

static int asciiColorEncode( term_encode_t* enc, uint8_t color_type, int extended ) {
	uint8_t *prgb;
	uint32_t rgb;
	uint32_t last_rgb;
	uint32_t r, g, b;
	int fg = 0xFF;
	size_t hx,x;
	size_t hy,y;
	uint8_t attr, last_attr;
	uint8_t reverse, bold;
	uint16_t c;
	char *utf8c;
	size_t char_width;
	
	uint8_t bw = (enc->stdpal) && (enc->palsize == 0);
	
	aa_context *aa;
	int supported = AA_NORMAL_MASK;
	
	if( extended ) {
		supported |= AA_EXTENDED;
	}
	aa = aaRender(enc,&char_width,0,supported);
	if( aa == 0 ) { return 1; }
	
	if( enc->encbinary ) {
		binWriteHeader(enc,enc->width/2,enc->height/2);
	}
	
	if( enc->enctext ) {
		fprintf(enc->textfp,"\x1b[0m");
	}
	for( hy=0; hy<enc->height/2; hy++ ) {
		y = hy*2;
		last_attr = AA_NORMAL;
		last_rgb = -1;
		for( hx=0; hx<enc->width/2; hx++ ) {
			x = hx*2;
			if( !bw ) {
				prgb = &(enc->rgbpixels[3*(y*enc->width+x)]);
				r = *(prgb);
				g = *(++prgb);
				b = *(++prgb);
				if( !enc->palsize ) {
					r = ( r + *(++prgb));
					g = ( g + *(++prgb));
					b = ( b + *(++prgb));
					prgb = &(enc->rgbpixels[3*((y+1)*enc->width+x)]);
					r = ( r + *(prgb));
					g = ( g + *(++prgb));
					b = ( b + *(++prgb));
					r = ( r + *(++prgb));
					g = ( g + *(++prgb));
					b = ( b + *(++prgb));
					r = r / 4;
					g = g / 4;
					b = b / 4;
				}
				rgb = (r<<16)|(g<<8)|(b);
			}
			attr = aa->attrbuffer[hy*char_width+hx];
			c = cp437[aa->textbuffer[hy*char_width+hx]];
			utf8c = utf8_encode(0,c);
			reverse = (attr == AA_REVERSE);
			bold = (attr == AA_BOLD);
			if( enc->enctext ) {
				if( !bw ) {
					if( last_rgb != rgb ) {
						fprintf(enc->textfp,"\x1b[0m");
						if( color_type == ENC_FGCOLOR ) {
							ansiSetColor(enc,ENC_BGCOLOR,0,0,0);
							ansiSetColor(enc,ENC_FGCOLOR,r,g,b);
						}
						else if( color_type == ENC_BGCOLOR ) {
							ansiSetColorRGB(enc,ENC_BGCOLOR,rgb);
							if( (r*0.299 + g*0.587 + b*0.114) > 150 ) { //Theory Limit 186
								fg = 0;
							} else {
								fg = 0xFF;
							}
							ansiSetColor(enc,ENC_FGCOLOR,fg,fg,fg);
						}
					}
					
					if( reverse ) {
						fprintf(enc->textfp,"\x1b[7m");
					}
					else if( bold ) {
						fprintf(enc->textfp,"\x1b[1m");
					}
					
					last_rgb = rgb;
					last_attr = attr;
				}
				fprintf(enc->textfp,"%s",utf8c);
			}
			if( enc->encbinary ) {
				if( color_type == ENC_FGCOLOR ) {
					binWriteCellRGB(enc,rgb,0,
					reverse,0,bold,0, c);
				}
				else if( color_type == ENC_BGCOLOR ) {
				binWriteCell(enc,fg,fg,fg,r,g,b,
					reverse,0,bold,0, c);
				}
			}
		}
		if( enc->enctext ) {
			fprintf(enc->textfp,"\x1b[0m\r\n");
		}
	}
	
	if( enc->encbinary ) {
		binClose(enc);
	}
	return 0;
}
#endif //USE_AALIB

#ifdef USE_LIBCACA
caca_canvas_t* cacaRender( term_encode_t* enc,
		size_t* char_width, size_t* char_height, 
		char* charset, char* dither_algorithm ) {
	int error = 1;
	caca_canvas_t *canvas = 0;
	caca_dither_t *dither = 0;
	size_t i;
	uint8_t *rgb;
	uint32_t *cacapixels;
	size_t cwidth;
	size_t cheight;
	
	cwidth = enc->width;
	if( char_width ) {
		*char_width = cwidth;
	}
	cheight = enc->height/2;
	if( char_height ) {
		*char_height = cheight;
	}
	
	cacapixels = (uint32_t*)malloc(sizeof(uint32_t)*enc->width*enc->height);
	if( cacapixels == 0 ) {
		fprintf(stderr,"Failed to allocate caca pixels\n");
		goto cacaRenderEnd;
	}
	
	canvas = caca_create_canvas(cwidth,cheight);
	if( canvas ==0 ) {
		fprintf(stderr,"Failed to crate caca canvas\n");
		goto cacaRenderEnd;
	}

	//Create a new array with alpha channel
	for( i=0; i<enc->width*enc->height; i++ ) {
		rgb = &(enc->rgbpixels[3*i]);
		cacapixels[i] = 0xFF000000 | (*(rgb) << 16) | (*(rgb+1) << 8) | *(rgb+2);
		//cacapixels[i] = ntohl(cacapixels[i]);
	}
	dither = caca_create_dither(32,enc->width,enc->height,enc->width*sizeof(uint32_t),
		0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
	if( dither == 0 ) {
		fprintf(stderr,"Failed to create caca dither\n");
		goto cacaRenderEnd;
	}
	if( caca_set_dither_color(dither,"full16") ) {
		fprintf(stderr,"Failed to set caca dither color\n");
		goto cacaRenderEnd;
	}
	if( caca_set_dither_charset(dither,charset) ) {
		fprintf(stderr,"Failed to set caca character set to \"%s\"",charset);
		goto cacaRenderEnd;
	}
	if( caca_set_dither_algorithm(dither,dither_algorithm) ) {
		fprintf(stderr,"Failed to set caca dither algorithm to \"%s\"",dither_algorithm);
		goto cacaRenderEnd;
	}
	caca_dither_bitmap(canvas,0,0,cwidth,cheight,
		dither,cacapixels);
	
	error = 0;
	cacaRenderEnd:
	if( dither ) {
		caca_free_dither(dither);
	}
	if( error && canvas ) {
		caca_free_canvas(canvas);
		canvas = 0;
	}
	return canvas;
}

static int cacaEncode( term_encode_t* enc, uint8_t use_blocks ) {
	caca_canvas_t *canvas;
	const uint32_t *cacachars;
	const uint32_t *cacaattrs;
	uint32_t lastattr;
	uint32_t currattr;
	uint32_t currchar;
	uint8_t blink,bold,underline;
	uint8_t color, fg, bg;
	char* utf8c;
	size_t x,y;
	size_t char_width;
	size_t char_height;

	if( use_blocks ) {
		canvas = cacaRender(enc,&char_width,&char_height,"blocks","none");
	}
	else {
		canvas = cacaRender(enc,&char_width,&char_height,"ascii","none");
	}
	if( canvas == 0 ) {
		return 1;
	}
	
	if( enc->encbinary ) {
		enc->stdpal = 1;
		enc->palsize = 16;
		binWriteHeader(enc,enc->width,enc->height);
	}
	
	//libcaca drivers want to control the entire terminal, and destroy the
	//rendering when the display is destroyed.  This is not the way the
	//other renderers operate, so we use the libcaca canvas and dither
	//routines, but then render it ourself.
	cacachars = caca_get_canvas_chars(canvas);
	cacaattrs = caca_get_canvas_attrs(canvas);
	for( y=0; y<char_height; y++ ) {
		lastattr = 0;
		for( x=0; x<char_width; x++ ) {
			currchar = cacachars[y*char_width+x];
			utf8c = utf8_encode(0,currchar);
			currattr = cacaattrs[y*char_width+x];
			fg = caca_attr_to_ansi_fg(currattr);
			bg = caca_attr_to_ansi_bg(currattr);
			if( currattr & CACA_BOLD ) { bold=1; }
			else { bold=0; }
			if( currattr & CACA_BLINK ) { blink=1; }
			else { blink=0; }
			if( currattr & CACA_UNDERLINE ) { underline=1; }
			else { underline=0; }
			if( enc->enctext ) {
				if( currattr != lastattr ) {
					fprintf(enc->textfp,"\x1b[0m");
					ansiSetStdColor(enc,ENC_FGCOLOR,fg);
					ansiSetStdColor(enc,ENC_BGCOLOR,bg);
					if( bold ) {
						fprintf(enc->textfp,"\x1b[1m");
					}
					//Ignoring Italics - it's not consistantly supported
					if( underline ) {
						fprintf(enc->textfp,"\x1b[4m");
					}
					if( blink ) {
						fprintf(enc->textfp,"\x1b[5m");
					}
					lastattr = currattr;
				}
				fprintf(enc->textfp,"%s",utf8c);
			}
			if( enc->encbinary ) {
				binWriteCellIndex(enc,fg,bg,
					0,bold,blink,underline, currchar);
			}
		}
		if( enc->enctext ) {
			fprintf(enc->textfp,"\x1b[0m\r\n");
		}
	}
	fflush(stdout);
	
	if( enc->encbinary ) {
		binClose(enc);
	}

	caca_free_canvas(canvas);
	return 0;
}
#endif //USE_LIBCACA

#if USE_LIBSIXEL
static int sixelEncode( term_encode_t* enc ) {
	SIXELSTATUS result = SIXEL_OK;
	sixel_allocator_t *allocator;
	sixel_encoder_t* encoder;
	
	result = sixel_allocator_new(&allocator,0,0,0,0);
	if( result != SIXEL_OK ) {
		fprintf(stderr,"Failed to create sixel allocator\n");
		return 1;
	}
	
	result = sixel_encoder_new(&encoder,allocator);
	if( result != SIXEL_OK ) {
		fprintf(stderr,"Failed to create encoded\n");
		return 1;
	}

	result = sixel_encoder_encode_bytes(
		encoder,
		enc->rgbpixels,
		enc->width,
		enc->height,
		PIXELFORMAT_RGB888,0,256);
	if( result != SIXEL_OK ) {
		fprintf(stderr,"Failed to encode image\n");
		return 1;
	}
	sixel_encoder_unref(encoder);
	sixel_allocator_unref(allocator);
	return 0;
}
#endif

//pixels_per_col = number of pixels per character column in the renderer
//pixel_ratio    = renderer pixel ratio (width/height);
static int prepImage( term_encode_t* enc, float pixels_per_col, float pixel_ratio ) {
	uint8_t *dstrgb;
	uint8_t *srcrgb;
	size_t i, x, y;
	size_t orgcolors,genpalsize;
	uint8_t *imgpixels= enc->imgpixels;
	uint8_t *alloctmp;
	size_t imgwidth;
	size_t imgheight;
	float imgratio;

	//Crop Image
	if( enc->crop.w && enc->crop.h ) {
		if( enc->crop.y >= imgheight || enc->crop.x >= imgwidth ) {
			fprintf(stderr,"Failed to crop image because rectange is out of bounds\n");
			return 1;
		}
		if( enc->crop.y+enc->crop.h > enc->imgheight ) {
			imgheight = enc->imgheight - enc->crop.y;
		} else {
			imgheight = enc->crop.h;
		}
		if( enc->crop.x+enc->crop.w > enc->imgwidth ) {
			imgwidth = enc->imgwidth - enc->crop.x;
		} else {
			imgwidth = enc->crop.w;
		}
		imgpixels = enc->imgpixels;
		
		//Crop the image by moving all of the wanted pixels to the
		//top/left of the original images and change the reported
		//image size.
		for( y=0; y<imgheight; y++ ) {
			for( x=0; x<imgwidth; x++ ) {
				srcrgb = &(imgpixels[3*((y+enc->crop.y)*enc->imgwidth+(x+enc->crop.x))]);
				dstrgb = &(imgpixels[3*(y*imgwidth+x)]);
				*(dstrgb) = *(srcrgb);
				*(++dstrgb) = *(++srcrgb);
				*(++dstrgb) = *(++srcrgb);
			}
		}
		enc->imgwidth = imgwidth;
		enc->imgheight = imgheight;
	}
	
	//Resize image so that pixel width matches the  target character width (win_width).
	//The pixel height is calculated to maintain the image ratio.  Things to consider:
	// 1) Pixels are square
	// 2) Terminal characters are rectangles (twice as height as they are wide)
	// 3) Renderers are responsible for "squaring" the resulting image (or not)
	//    through their selection of characters.  The pixel_ratio argument should
	//    specify how well the renderer will do this.
	imgpixels = enc->imgpixels;
	imgwidth = enc->imgwidth;
	imgheight = enc->imgheight;
	//Set the terminal width (characters) for the encoder
	if( enc->win_width == 0 ) {
		enc->win_width = imgwidth;
	}
	//Set the render width and height (pixels) for the encoder 
	imgratio = (float)imgheight / (float)imgwidth;
	enc->width = enc->win_width*pixels_per_col;
	enc->height = enc->width*imgratio*pixel_ratio;
	
	//Resize input image
	if( imgwidth != enc->width || imgheight != enc->height ) {
		imgpixels = (uint8_t*)malloc(sizeof(uint8_t)*3*enc->width*enc->height);
		if( imgpixels == 0 ) {
			fprintf(stderr,"Failed to allocate RGB pixels for resize: %f %ld %ld %ld\n",imgratio,enc->win_width,enc->width,enc->height);
			return 1;
		}
		if( ! stbir_resize_uint8(enc->imgpixels,enc->imgwidth,enc->imgheight,0,imgpixels,enc->width,enc->height,0,3) ) {
			fprintf(stderr,"Failed to resize image with ratio: %f\n",imgratio);
			return 1;
		}
		imgwidth = enc->width;
		imgheight = enc->height;
		#ifdef DEBUG
		fprintf(stderr,"Resized image from %lu / %lu to %lu / %lu\n",enc->imgwidth,enc->imgheight,imgwidth,imgheight);
		#endif
	}
	
	//Perform edge detection
	if( enc->edge != ENC_EDGE_NONE ) {
		if( enc->edge == ENC_EDGE_SCALE ) {
			edge_scale( imgpixels, enc->color_rgb, enc->invert, 0, imgpixels, imgwidth, imgheight );
		}
		else if( enc->edge == ENC_EDGE_LINE ) {
			edge_scale( imgpixels, enc->color_rgb, enc->invert, EDGE_DEFAULT_THRESHOLD, imgpixels, imgwidth, imgheight ); 
		}
		else if( enc->edge == ENC_EDGE_GLOW ) {
			edge_highlight( imgpixels, enc->color_rgb, 0, 1, imgpixels, imgwidth, imgheight );
		}
		else if( enc->edge == ENC_EDGE_HIGHLIGHT ) {
			edge_highlight( imgpixels, enc->color_rgb, EDGE_DEFAULT_THRESHOLD, 0, imgpixels, imgwidth, imgheight );
		}
	}
	
	//Black and White
	if( enc->stdpal && !enc->palsize ) {
		//allocate bwpixels
		alloctmp = (uint8_t*)malloc(sizeof(uint8_t)*enc->width*enc->height);
		if( alloctmp == 0 ) {
			fprintf(enc->textfp,"Failed allocate space for black and white pixels\n");
			return 1;
		}
		quant_bw(alloctmp,imgpixels,enc->width*enc->height,1);
		free(alloctmp);
	}
	//Paletteize
	else if( enc->palsize ) {
		//allocate palette
		alloctmp  = (uint8_t*)realloc(enc->palette,sizeof(uint8_t)*3*enc->palsize);
		if( alloctmp == 0 ) {
			fprintf(enc->textfp,"Failed allocate space for palette\n");
			return 1;
		}
		enc->palette = alloctmp;

		//allcode indexed (palettized) version of pixels
		alloctmp =  (uint8_t*)realloc(enc->palpixels,sizeof(uint8_t)*enc->width*enc->height);
		if( alloctmp == 0 ) {
			fprintf(stderr,"Failed to allocate palette pixels\n");
			return 1;
		}
		enc->palpixels = alloctmp;
		
		if( enc->stdpal ) {
			if( enc->palsize == 24 ) {
				dstrgb = &(enc->palette[0]);
				*(dstrgb) = (standard_palette[0]>>16)&0xFF;
				*(++dstrgb) = (standard_palette[0]>>8)&0xFF;
				*(++dstrgb) = (standard_palette[0])&0xFF;
				for( i=1; i<23; i++ ) {
					dstrgb = &(enc->palette[3*i]);
					*(dstrgb) = (standard_palette[231+i]>>16)&0xFF;
					*(++dstrgb) = (standard_palette[231+i]>>8)&0xFF;
					*(++dstrgb) = (standard_palette[231+i])&0xFF;
				}
				dstrgb = &(enc->palette[3*23]);
				*(dstrgb) = (standard_palette[15]>>16)&0xFF;
				*(++dstrgb) = (standard_palette[15]>>8)&0xFF;
				*(++dstrgb) = (standard_palette[15])&0xFF;
			}
			else {
				for( i=0; i<enc->palsize; i++ ) {
					dstrgb = &(enc->palette[3*i]);
					*(dstrgb) = (standard_palette[i]>>16)&0xFF;
					*(++dstrgb) = (standard_palette[i]>>8)&0xFF;
					*(++dstrgb) = (standard_palette[i])&0xFF;
				}
			}
			#ifdef USE_QUANTPNM
			if( ! enc->dither ) 
			#endif //USE_QUANTPNM
			{
				//Apply the palette for non-dither quantizer
				//Dither quanizer will apply the palette below
				quant_apply_palette(enc->palette, enc->palsize,
					enc->palpixels, imgpixels, enc->width*enc->height,
					1);
			}
			
		}
		else {
			//Quantize down to the "optimal" palette of specified palette size
			genpalsize = enc->palsize;
			#ifdef USE_QUANTPNM
			if( enc->dither ) {
				//quant_pnm_make_palette creates the palette, but it must be still be applied
				enc->palette = quant_pnm_make_palette(imgpixels, enc->width*enc->height,
					enc->palsize,&genpalsize,0,QUANT_LARGE_AUTO,QUANT_REP_AUTO,QUANT_QUALITY_HIGH);
			} else 
			#endif //USE_QUANTPNM
			{
				//quant_quantize will apply the palette as it creates it
				quant_quantize(enc->palette, &(genpalsize),
					enc->palpixels, imgpixels, enc->width*enc->height,1);
			}
			#ifdef DEBUG
			fprintf(stderr,"Changing terminal encoder palette size from %lu to %lu\n",enc->palsize,genpalsize);
			#endif
			enc->palsize = genpalsize;
		}
		#ifdef USE_QUANTPNM
		if( enc->dither ) {
			//Dither quantizer needs a call to apply the the palette reguardless
			//of whether a standard palette is used or not.
			quant_pnm_apply_palette(enc->palpixels, imgpixels, enc->width, enc->height,
				enc->palette, enc->palsize, QUANT_DIFFUSE_AUTO,1,0,1,0,&genpalsize);
			for( i=0; i<enc->width*enc->height; i++ ) {
				srcrgb = &(enc->palette[3*enc->palpixels[i]]);
				dstrgb = &(imgpixels[3*i]);
				*(dstrgb) = *(srcrgb);
				*(++dstrgb) = *(++srcrgb);
				*(++dstrgb) = *(++srcrgb);
			}
		}
		#endif //USE_QUANTPNM
	}
	//else True Color
	
	enc->rgbpixels = imgpixels;
	return 0;
}

void term_encode_init(term_encode_t* enc) {
	memset(enc,0,sizeof(term_encode_t));
}

void term_encode_destroy(term_encode_t* enc) {
	if( enc->rgbpixels && enc->rgbpixels != enc->imgpixels ) {
		free(enc->rgbpixels);
		enc->rgbpixels = 0;
	}
	if( enc->imgpixels ) {
		free(enc->imgpixels);
		enc->imgpixels = 0;
	}
	if( enc->palpixels ) {
		free(enc->palpixels);
		enc->palpixels = 0;
	}
	if( enc->palette ) {
		free(enc->palette);
		enc->palette = 0;
	}
}

//Use ioctl/TIOCGWINSZ to get terminal size
int term_encode_detect_win_width(term_encode_t* enc) {
	struct winsize ws;
	if( ioctl(0,TIOCGWINSZ,&ws) ) { return 1; }
	enc->win_width = ws.ws_col;
	return 0;
}

int term_encode(term_encode_t* enc) {
	if( (enc->stdpal && enc->reqpalsize != 0 && enc->reqpalsize != 16 && enc->reqpalsize != 256 && enc->reqpalsize != 24) ||
			enc->reqpalsize > 256 ) {
		enc->palsize = 0;
		fprintf(stderr,"Invalid palette size\n");
		return 1;
	}
	enc->palsize = enc->reqpalsize;
	
	if( enc->renderer && enc->textfp == 0 ){
		enc->textfp = stdout;
	}
	
	if( enc->clearterm ) {
		fprintf(enc->textfp,"\x1b[2J\x1b[H");
	}
	
	if( enc->renderer == ENC_RENDER_NONE ) {
	}
	#ifdef USE_LIBSIXEL
	else if( enc->renderer == ENC_RENDER_SIXEL ) {
		if( prepImage(enc,1,1.0) ) { return 1; }
		sixelEncode(enc);
	}
	#endif //USE_LIBSIXEL
	else if( enc->renderer == ENC_RENDER_SIMPLE ) {
		//pixel_ratio manually fine tuned based on Dejavu San Monospace
		if( prepImage(enc,1,0.48) ) { return 1; }
		ansiEncodeSimple( enc );
	}
	else if( enc->renderer == ENC_RENDER_HALF ) {
		//pixel_ratio manually fine tuned based on Dejavu San Monospace
		if( prepImage(enc,1.0,0.97) ) { return 1; }
		ansiEncodeHalfHeight( enc );
	}
	else if( enc->renderer == ENC_RENDER_QUARTER ) {
		//pixel_ratio manually fine tuned based on Dejavu San Monospace
		if( prepImage(enc,2.0,0.48) ) { return 1; }
		ansiEncodeQuarter( enc );
	}
	else if( enc->renderer == ENC_RENDER_SEXTANT ) {
		//pixel_ratio manually fine tuned based on Dejavu San Monospace
		if( prepImage(enc,2.0,0.72) ) { return 1; }
		ansiEncodeSextant( enc );
	}
	else if( enc->renderer == ENC_RENDER_BRAILLE ) {
		//pixel_ratio manually fine tuned based on Dejavu San Monospace
		if( prepImage(enc,2.0,0.95) ) { return 1; }
		ansiEncodeBraille( enc );
	}
	#ifdef USE_AALIB
	else if( enc->renderer == ENC_RENDER_AA ) {
		if( prepImage(enc,2.0,0.5) ) { return 1; }
		if( asciiEncode(enc,0) ) { return 1; }
	}
	else if( enc->renderer == ENC_RENDER_AAEXT ) {
		if( prepImage(enc,2.0,0.5) ) { return 1; }
		if( asciiEncode(enc,1) ) { return 1; }
	}
	else if( enc->renderer == ENC_RENDER_AAFG ) {
		if( prepImage(enc,2.0,0.5) ) { return 1; }
		if( asciiColorEncode(enc,ENC_FGCOLOR,0) ) { return 1; }
	}
	else if( enc->renderer == ENC_RENDER_AAFGEXT ) {
		if( prepImage(enc,2.0,0.5) ) { return 1; }
		if( asciiColorEncode(enc,ENC_FGCOLOR,1) ) { return 1; }
	}
	else if( enc->renderer == ENC_RENDER_AABG ) {
		if( prepImage(enc,2.0,0.5) ) { return 1; }
		if( asciiColorEncode(enc,ENC_BGCOLOR,0) ) { return 1; }
	}
	else if( enc->renderer == ENC_RENDER_AABGEXT ) {
		if( prepImage(enc,2.0,0.5) ) { return 1; }
		if( asciiColorEncode(enc,ENC_BGCOLOR,1) ) { return 1; }
	}
	#endif //USE_AALIB
	#ifdef USE_LIBCACA
	else if( enc->renderer == ENC_RENDER_CACA ) {
		if( prepImage(enc,1.0,1.0) ) { return 1; }
		if( cacaEncode(enc,0) ) { return 1; }
	}
	else if( enc->renderer == ENC_RENDER_CACABLK ) {
		if( prepImage(enc,1.0,1.0) ) { return 1; }
		if( cacaEncode(enc,1) ) { return 1; }
	}
	#endif //USE_LIBCACA
	else {
		fprintf(stderr,"Renderer not implemented.");
		return 1;
	}
	return 0;
}
