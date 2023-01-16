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
#ifndef __TERM_ENCODE_H__
#define __TERM_ENCODE_H__

#include <stdint.h>

/////////////
// Renderers
/////////////
#define ENC_RENDER_NONE       0
#define ENC_RENDER_SIXEL      1
#define ENC_RENDER_SIMPLE     2
#define ENC_RENDER_DOUBLE     3
#define ENC_RENDER_HALF       4
#define ENC_RENDER_QUARTER    5
#define ENC_RENDER_SEXTANT    6
#define ENC_RENDER_AA         7
#define ENC_RENDER_AAEXT      8
#define ENC_RENDER_AADL       9
#define ENC_RENDER_AADLEXT   10
#define ENC_RENDER_AAFG      11
#define ENC_RENDER_AAFGEXT   12
#define ENC_RENDER_AABG      13
#define ENC_RENDER_AABGEXT   14
#define ENC_RENDER_CACA      15
#define ENC_RENDER_CACABLK   16

//////////////////////////////
// Edge Decection Processing
//////////////////////////////
#define ENC_EDGE_NONE      0
#define ENC_EDGE_SCALE     1
#define ENC_EDGE_LINE      2
#define ENC_EDGE_GLOW      3
#define ENC_EDGE_HIGHLIGHT 4

typedef struct {
	size_t x;
	size_t y;
	size_t w;
	size_t h;
} crop_rect_t;

typedef struct {
	///////////////////////////
	// Encoder input arguments
	///////////////////////////
	
	//Input image data.
	//Pixels are RGB format
	//imgpixels may be modified during encode and will
	//be freed during destory if non-null
	uint8_t* imgpixels;
	size_t   imgwidth;
	size_t   imgheight;
	
	//Target terminal width
	//For sixel target pixel width
	size_t win_width;
	
	//Renderer
	//One of ENC_RENDER_*
	uint8_t renderer;
	
	//1 - Output encoded text to textfp
	//0 - Do not output encoded text
	uint8_t enctext; 
	
	//1 - Encode binary file for newdraw to binaryfp
	//0 - Do not enocde binary file
	uint8_t encbinary;
	
	//true  - Include termianl clear in text
	//false - Do not include terminal clear
	uint8_t clearterm;
	
	#ifdef USE_QUANTPNM
	//true  - Use quantizer derived from pnmcolormap.c
	//false - Use quantizer without dither
	uint8_t dither;
	#endif
	
	//true  - Use a standard palette
	//false - Use an optimal palette
	uint8_t stdpal;
	
	//Requested palette size
	//0 - Use 24-bit true color
	//1-256 - Optimal palette size
	//        Actual calculated size may be less
	//If stdpal is true, then must be 16 or 256 or 24
	//    A standard palette of 24 is 24 color gray scale 
	//    part of the standard 256 color pallette
	size_t reqpalsize;
	
	//Actual calculated palette
	size_t palsize;
	
	//Optional image crop
	//This occurs before resizing and other processing
	crop_rect_t crop;
	
	//Optional edge processing
	//One of ENC_EDGE_*
	uint8_t edge;
	
	//Color value to use for any process that requires
	//a color (ie edge dection).  
	//  Format: 0x--RRGGBB
	//invert=0 makes color foreground with a black background
	//invert=1 makes color background with a white foreground
	uint32_t color_rgb;
	uint8_t invert;
	
	//File for text output
	//Only used if enctext is true
	FILE* textfp;
	
	//file for binary output
	//Only used if encbinary is true
	FILE* binaryfp;
	
	///////////////////////////////
	// Internal Encoder Variables
	///////////////////////////////
	//RGB pixels
	uint8_t* rgbpixels;
	//Palette indexes for RGB pixels
	uint8_t* palpixels;
	//Palette used by palpixels
	//Length is palsize
	uint8_t* palette;
	//Size of rgbpixel/palpixels
	size_t width;
	size_t height;
} term_encode_t;

void term_encode_init(term_encode_t* enc);
void term_encode_destroy(term_encode_t* enc);
int term_encode_detect_win_width(term_encode_t* enc);
int term_encode(term_encode_t* enc);

#endif //__TERM_ENCODE_H__