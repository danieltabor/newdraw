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
#ifndef __APPLE2_H__
#define __APPLE2_H__

#include <stdint.h>

void apple2( uint8_t *dst_pixels, uint8_t *src_pixels, size_t width, size_t height, int scanline );

#ifdef APPLE2_IMPLEMENTATION

static uint32_t apple2_palette[2][4] = {
	{0x000000,0x62c000,0xad44ff,0xffffff},
	{0x000000,0xdc681f,0x48a1e3,0xffffff},
};

static uint32_t apply_palette( uint8_t *dst, uint8_t *src, uint32_t *pal ) {
	uint32_t min_dist;
	size_t i, p;
	int channel_dist;
	uint32_t color_dist;
	uint32_t total_dist = 0;
	uint32_t color;
	
	for( i=0; i<7; i++ ) {
		min_dist = 0xFFFFFFFF;
		for( p=0; p<4; p++ ) {
			channel_dist = ((pal[p]>>16)&0xFF) - src[3*i];
			channel_dist = channel_dist * channel_dist;
			color_dist   = channel_dist;
			channel_dist = ((pal[p]>>8)&0xFF) - src[3*i+1];
			channel_dist = channel_dist * channel_dist;
			color_dist   = color_dist+channel_dist;
			channel_dist = (pal[p]&0xFF) - src[3*i+2];
			channel_dist = channel_dist * channel_dist;
			color_dist   = color_dist+channel_dist;
			if( color_dist < min_dist ) {
				min_dist = color_dist;
				color = pal[p];
			}
		}
		total_dist = total_dist + min_dist;
		dst[3*i]   = (color>>16)&0xFF;
		dst[3*i+1] = (color>>8)&0xFF;
		dst[3*i+2] = color&0xFF;
	}
	return total_dist/7;
}

void apple2( uint8_t *dst_pixels, uint8_t *src_pixels, size_t width, size_t height, int scanline ) {
	size_t y, x, i;
	uint8_t chunk_pixels[21];
	uint8_t *pal_pixels;
	uint8_t pal0_pixels[21];
	uint8_t pal1_pixels[21];
	uint32_t pal0_dist;
	uint32_t pal1_dist;
	
	for( y=0; y<height; y++ ) {
		if( scanline && y&1 ) { continue; }
		for( x=0; x<width; x=x+7 ) {
			for( i=0; i<7; i++ ) {
				if( x+i < width ) {
					memcpy(chunk_pixels+3*i, src_pixels+3*(y*width+x+i), 3);
				}
				else {
					memset(chunk_pixels+3*i,0,3);
				}
			}
			pal0_dist = apply_palette( pal0_pixels, chunk_pixels, apple2_palette[0] );
			pal1_dist = apply_palette( pal1_pixels, chunk_pixels, apple2_palette[1] );
			if( pal0_dist < pal1_dist ) {
				pal_pixels = pal0_pixels;
			}
			else {
				pal_pixels = pal1_pixels;
			}
			for( i=0; i<7; i++ ) {
				if( x+i < width ) {
					memcpy(dst_pixels+3*(y*width+x+i), pal_pixels+3*i ,3);
				}
			}
		}
	}
}

#endif //APPLE2_IMPLEMENTATION
#endif //__APPLE2_H__
