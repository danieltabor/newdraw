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

void apple2( uint8_t *dst_pixels, uint8_t *src_pixels, size_t width, size_t height, int bw, uint32_t fg );

#ifdef APPLE2_UNREALISTIC
//Allows a full horizontal pixel resolution.
//The normal apple2 filter averages pixel pairs, and then two black&white pixels are generated from each pair.
//Similarly two identical (NTSC artifact) color pixels are generated for each pair, thus halfing the effective
//horizontal resolution in color.  This function processes each indvidual src pixel, and considers a 7 pixels per 
//palette selection.
void apple2_full( uint8_t *dst_pixels, uint8_t *src_pixels, size_t width, size_t height );
#endif //APPLE2_UNREALISTIC

#ifdef APPLE2_IMPLEMENTATION

static uint32_t apple2_palette[2][4] = {
	{0x000000,0x62c000,0xad44ff,0xffffff},
	{0x000000,0xdc681f,0x48a1e3,0xffffff},
};

static uint32_t apply_palette( uint8_t *dst, uint8_t *dstidx, uint8_t *src, uint32_t *pal ) {
	uint32_t min_dist;
	size_t i, p, c;
	int channel_dist;
	uint32_t color_dist;
	uint32_t total_dist = 0;
	
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
				c = p;
			}
		}
		total_dist = total_dist + min_dist;
		if( dst ) {
			dst[3*i]   = (pal[c]>>16)&0xFF;
			dst[3*i+1] = (pal[c]>>8)&0xFF;
			dst[3*i+2] = pal[c]&0xFF;
		}
		if( dstidx ) {
			dstidx[i] = c;
		}
	}
	return total_dist/7;
}

void apple2( uint8_t *dst_pixels, uint8_t *src_pixels, size_t width, size_t height, int bw, uint32_t fg ) {
	size_t y, x, i;
	uint8_t chunk_pixels[21];
	uint8_t *pal_idx;
	uint8_t *pal_pixels;
	uint8_t pal0_pixels[21];
	uint8_t pal1_pixels[21];
	uint8_t pal0_idx[7];
	uint8_t pal1_idx[7];
	uint32_t pal0_dist;
	uint32_t pal1_dist;
	uint8_t r,g,b;
	
	for( y=0; y<height; y++ ) {
		for( x=0; x<width; x=x+14 ) {
			for( i=0; i<14; i=i+2 ) {
				if( x+i < width ) {
					r = src_pixels[3*(y*width+x+i)];
					g = src_pixels[3*(y*width+x+i)+1];
					b = src_pixels[3*(y*width+x+i)+2];
					if( x+i+1 < width ) {
						r = (r + src_pixels[3*(y*width+x+i+1)]) / 2;
						g = (g + src_pixels[3*(y*width+x+i+1)+1]) / 2;
						b = (b + src_pixels[3*(y*width+x+i+1)+2]) / 2;
					}
				}
				else {
					r = 0;
					g = 0;
					b = 0;
				}
				chunk_pixels[3*(i/2)] = r;
				chunk_pixels[3*(i/2)+1] = g;
				chunk_pixels[3*(i/2)+2] = b;
			}
			pal0_dist = apply_palette( pal0_pixels, pal0_idx, chunk_pixels, apple2_palette[0] );
			pal1_dist = apply_palette( pal1_pixels, pal1_idx, chunk_pixels, apple2_palette[1] );
			if( pal0_dist < pal1_dist ) {
				pal_idx    = pal0_idx;
				pal_pixels = pal0_pixels;
			}
			else {
				pal_idx    = pal1_idx;
				pal_pixels = pal1_pixels;
			}
			if( bw ) {
				for( i=0; i<14; i=i+2 ) {
					if( x+i < width ) {
						if( pal_idx[i/2] & 2 ) {
							dst_pixels[3*(y*width+x+i)]   = (fg>>16)&0xff;
							dst_pixels[3*(y*width+x+i)+1] = (fg>>8)&0xff;
							dst_pixels[3*(y*width+x+i)+2] = fg&0xff;
						}
						else {
							dst_pixels[3*(y*width+x+i)]   = 0;
							dst_pixels[3*(y*width+x+i)+1] = 0;
							dst_pixels[3*(y*width+x+i)+2] = 0;
						}
						if( x+i+1 < width ) {
							if( pal_idx[i/2] & 1 ) {
								dst_pixels[3*(y*width+x+i+1)]   = (fg>>16)&0xff;
								dst_pixels[3*(y*width+x+i+1)+1] = (fg>>8)&0xff;
								dst_pixels[3*(y*width+x+i+1)+2] = fg&0xff;
							}
							else {
								dst_pixels[3*(y*width+x+i+1)]   = 0;
								dst_pixels[3*(y*width+x+i+1)+1] = 0;
								dst_pixels[3*(y*width+x+i+1)+2] = 0;
							}
						}
					}
				}
			}
			else {
				for( i=0; i<14; i=i+2 ) {
					if( x+i < width ) {
						dst_pixels[3*(y*width+x+i)]   = pal_pixels[3*(i/2)];
						dst_pixels[3*(y*width+x+i)+1] = pal_pixels[3*(i/2)+1];
						dst_pixels[3*(y*width+x+i)+2] = pal_pixels[3*(i/2)+2];
						if( x+i+1 < width ) {
							dst_pixels[3*(y*width+x+i+1)]   = pal_pixels[3*(i/2)];
							dst_pixels[3*(y*width+x+i+1)+1] = pal_pixels[3*(i/2)+1];
							dst_pixels[3*(y*width+x+i+1)+2] = pal_pixels[3*(i/2)+2];
						}
					}
				}
			}
		}
	}
}

#ifdef APPLE2_UNREALISTIC
void apple2_full( uint8_t *dst_pixels, uint8_t *src_pixels, size_t width, size_t height ) {
	size_t y, x, i;
	uint8_t chunk_pixels[21];
	uint8_t *pal_pixels;
	uint8_t pal0_pixels[21];
	uint8_t pal1_pixels[21];
	uint32_t pal0_dist;
	uint32_t pal1_dist;
	
	for( y=0; y<height; y++ ) {
		for( x=0; x<width; x=x+7 ) {
			for( i=0; i<7; i++ ) {
				if( x+i < width ) {
					memcpy(chunk_pixels+3*i, src_pixels+3*(y*width+x+i), 3);
				}
				else {
					memset(chunk_pixels+3*i,0,3);
				}
			}
			pal0_dist = apply_palette( pal0_pixels, 0, chunk_pixels, apple2_palette[0] );
			pal1_dist = apply_palette( pal1_pixels, 0, chunk_pixels, apple2_palette[1] );
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
#endif //APPLE2_UNREALISTIC

#endif //APPLE2_IMPLEMENTATION
#endif //__APPLE2_H__
