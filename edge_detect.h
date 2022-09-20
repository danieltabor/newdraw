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
#ifndef __EDGE_DETECT_H__
#define __EDGE_DETECT_H__

#include <stdint.h>

#define EDGE_DEFAULT_THRESHOLD 32

void edge_detect( uint8_t *edge, uint8_t threshold, 
		uint8_t* rgb_image_pixels, size_t width, size_t height );

int edge_scale( uint8_t *rgb_edge_pixels, uint32_t fg, uint8_t invert, uint8_t threshold, 
		uint8_t *rgb_image_pixels, size_t width, size_t height );

int edge_highlight( uint8_t *rgb_edge_pixels, uint32_t fg, uint8_t threshold, uint8_t mix,
		uint8_t *rgb_image_pixels, size_t width, size_t height );

void edge_free();

#endif //__EDGE_DETECT_H__

#ifdef EDGE_DETECT_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SQUARE( x ) ((double)(x)*(double)(x))
#define DR( buf, idx ) ((double)buf[3*(idx)])
#define DG( buf, idx ) ((double)buf[3*(idx)+1])
#define DB( buf, idx ) ((double)buf[3*(idx)+2])
#define cdist(buf0, idx0, buf1, idx1) (sqrt( SQUARE( DR(buf0,idx0)-DR(buf1,idx1) ) + SQUARE( DG(buf0,idx0)-DG(buf1,idx1) ) + SQUARE( DB(buf0,idx0)-DB(buf1,idx1) ) ))

#define MAX_DISTANCE 441.673

uint8_t *g_scratch = 0;

void edge_detect( uint8_t *edge, uint8_t threshold, 
		uint8_t* rgb_image_pixels, size_t width, size_t height ) {
	size_t i,x,y;
	double horiz_distance;
	double vert_distance;
	uint8_t distance;
	uint8_t max_distance = 0;
	float ratio;
	
	memset(edge,0,width*height);
	
	for( y=0; y<height-1; y++ ) {
		for( x=0; x<width-1; x++ ) {
			horiz_distance = cdist( rgb_image_pixels, y*width+x, rgb_image_pixels, y    *width+ x+1 );
			vert_distance  = cdist( rgb_image_pixels, y*width+x, rgb_image_pixels, (y+1)*width+ x   );
			
			if( horiz_distance > vert_distance ) {
				distance = (uint8_t)(255 * (horiz_distance / (double)MAX_DISTANCE));
			} else {
				distance = (uint8_t)(255 * (vert_distance / (double)MAX_DISTANCE));
			}
			if( distance > max_distance ) {
				max_distance = distance;
			}
			edge[y*width+x] = distance;
		}
	}
	if( max_distance ) {
		ratio = 255.0/(float)max_distance;
	} else {
		ratio = 1;
	}
	for( i=0; i<width*height; i++ ) {
		distance = edge[i]*ratio;
		if( threshold == 0 ) {
			edge[i] = distance;
		} else if( distance >= threshold ) {
			edge[i] = 0xFF;
		} else {
			edge[i] = 0;
		}
	}
}

int edge_scale( uint8_t *rgb_edge_pixels, uint32_t rgb, uint8_t invert, uint8_t threshold,
		uint8_t *rgb_image_pixels, size_t width, size_t height ) {
	size_t i;
	uint8_t *dstrgb;
	uint8_t *edge;
	uint8_t cred = (rgb>>16)&0xFF;
	uint8_t cgreen = (rgb>>8)&0xFF;
	uint8_t cblue = (rgb)&0xFF;
	int red, green, blue;
	float ratio;
	edge = (uint8_t*)realloc(g_scratch,width*height);
	if( edge == 0 ) {
		fprintf(stderr,"Failed to allocate buffer for edge detection.\n");
		return 1;
	}
	edge_detect(edge, threshold, rgb_image_pixels, width, height);
	for( i=0; i<width*height;i++ ) {
		dstrgb = &(rgb_edge_pixels[3*i]);
		ratio = (float)edge[i]/255.0;
		if( invert ) {
			red = cred+0xFF*ratio;
			if( red > 0xFF ) { red = 0xFF; }
			green = cgreen+0xFF*ratio;
			if( green > 0xFF ) { green = 0xFF; }
			blue = cblue+0xFF*ratio;
			if( blue > 0xFF ) { blue = 0xFF; }
			*dstrgb = red;
			*(++dstrgb) = green;
			*(++dstrgb) = blue;
		}
		else {
			*dstrgb = cred*ratio;
			*(++dstrgb) = cgreen*ratio;
			*(++dstrgb) = cblue*ratio;
		}
	}
	return 0;
}

int edge_highlight( uint8_t *rgb_edge_pixels, uint32_t fg, uint8_t threshold, uint8_t mix,
		uint8_t *rgb_image_pixels, size_t width, size_t height ) {
	size_t i;
	uint8_t *dstrgb;
	uint8_t *srcrgb;
	uint8_t *edge;
	uint8_t fg_red = (fg>>16)&0xFF;
	uint8_t fg_green = (fg>>8)&0xFF;
	uint8_t fg_blue = (fg)&0xFF;
	uint32_t red, green, blue;
	float ratio;
	edge = (uint8_t*)realloc(g_scratch,width*height);
	if( edge == 0 ) {
		fprintf(stderr,"Failed to allocate buffer for edge detection.\n");
		return 1;
	}
	if( mix ) { mix = 1; } //mix need to be precisely 0 or 1 for the math later
	edge_detect(edge, threshold, rgb_image_pixels, width, height);
	for( i=0; i<width*height;i++ ) {
		srcrgb = &(rgb_image_pixels[3*i]);
		dstrgb = &(rgb_edge_pixels[3*i]);
		if( edge[i] ) {
			ratio = (float)edge[i]/255.0;
			red = *srcrgb * mix + fg_red * ratio;
			if( red > 0xFF ) { red = 0xFF;}
			green = *(++srcrgb) * mix + fg_green * ratio;
			if( green > 0xFF ) { green = 0xFF;}
			blue = *(++srcrgb) * mix + fg_blue * ratio;
			if( blue  > 0xFF ) { blue = 0xFF;}
			*dstrgb = red;
			*(++dstrgb) = green;
			*(++dstrgb) = blue;
		} else {
			*dstrgb = *(srcrgb);
			*(++dstrgb) = *(++srcrgb);
			*(++dstrgb) = *(++srcrgb);
		}
	}
	return 0;
}

void edge_free() {
	if( g_scratch ) {
		free(g_scratch);
		g_scratch = 0;
	}
}
	
#endif //EDGE_DETECT_IMPLEMENTATION