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
#ifndef __QUANT_H__
#define __QUANT_H__

#include <stdint.h>

void quant_quantize(
	uint8_t *palette,
	size_t *palsize,
	uint8_t *palpixels,
	uint8_t *rgbpixels,
	size_t pixelslen,
	uint8_t syncrgb );

/* apply color palette into specified pixel buffers */
void quant_apply_palette(
	uint8_t *palette,
	size_t palsize,
	uint8_t *palpixes,
	uint8_t *rgbpixels,
	size_t pixelslen,
	uint8_t syncrgb );


//
//
////   end header file   /////////////////////////////////////////////////////
#endif // __QUANT_H__

#ifdef QUANT_IMPLEMENTATION

#include <math.h>

#define SQUARE( x ) ((double)(x)*(double)(x))
#define DR( buf, idx ) ((double)buf[3*(idx)])
#define DG( buf, idx ) ((double)buf[3*(idx)+1])
#define DB( buf, idx ) ((double)buf[3*(idx)+2])
#define DY( buf, idx ) (0.2126*DR((buf),(idx)) + 0.7152*DG((buf),(idx)) + 0.0722*DB((buf),(idx)))
#define cdist(buf0, idx0, buf1, idx1) (sqrt( SQUARE( DR(buf0,idx0)-DR(buf1,idx1) ) + SQUARE( DG(buf0,idx0)-DG(buf1,idx1) ) + SQUARE( DB(buf0,idx0)-DB(buf1,idx1) ) ))
#define rgbassign( dst,didx, src, sidx )  { dst[3*(didx)+0] = src[3*(sidx)+0]; dst[3*(didx)+1] = src[3*(sidx)+1]; dst[3*(didx)+2] = src[3*(sidx)+2]; }

static void reduce_palette(
	uint8_t *palette,
	size_t *palsize,
	uint8_t *palpixels,
	size_t palpixelslen,
	size_t distance) {
	size_t q, p, r;
	size_t i;
	
	for( q=0; q<(*palsize); q++ ) {
		for( p=q+1; p<(*palsize); ) {
			if( cdist(palette,q,palette,p) <= distance ) {
				for( r=p+1; r<(*palsize); r++ ) {
					rgbassign(palette,(r-1),palette,r);
				}
				(*palsize)--;
				for( i=0; i<palpixelslen; i++ ) {
					if( palpixels[i] == p ) { 
							palpixels[i] = q; 
					} else if( palpixels[i] > p ) {
						palpixels[i]--;
					}
				}
			} else {
				p++;
			}
		}
	}
}


void quant_quantize(
	uint8_t *palette,
	size_t *palsize,
	uint8_t *palpixels,
	uint8_t *rgbpixels,
	size_t pixelslen,
	uint8_t syncrgb ) {
	
	size_t distance = 0;
	size_t i, p;
	size_t cpalsize = 0;
	
	for( i=0; i<pixelslen; i++ ) {
		while( distance < 0xFFFFFF ) {
			//Try to find an existing color that matches this pixel
			for( p=0; p<cpalsize; p++ ) {
				if( cdist(rgbpixels,i,palette,p) <= distance ) {
					//Found a suitable color on the palette
					break;
				}
			}
			if( p < cpalsize ) {
				//Assign the pixel to the found pallette color
				palpixels[i] = p;
				break;
			}
			//If an existing color could not be found...
			else { //p == cpalsize
				//Try to add the color to the end of the palette
				if( cpalsize < *palsize ) {
					rgbassign(palette,p,rgbpixels,i);
					cpalsize++;
					palpixels[i] = p;
					break;
				}
				//Or reduce the palette and try again
				else {
					distance++;
					#ifdef DEBUG
					printf("Reduction: distance(%lu) palsize(%lu) ",distance,cpalsize);
					#endif
					reduce_palette(palette,&cpalsize,
						palpixels,i,distance);
					#ifdef DEBUG
					printf("   new palsize(%lu)\n",cpalsize);
					#endif
				}
			}
		}
	}
	if( syncrgb ) {
		for( i=0; i<pixelslen; i++ ) {
			rgbassign(rgbpixels,i,palette,(palpixels[i]));
		}
	}
	*palsize = cpalsize;
}

void quant_apply_palette(
	uint8_t *palette,
	size_t palsize,
	uint8_t *palpixels,
	uint8_t *rgbpixels,
	size_t pixelslen,
	uint8_t syncrgb ) {
	
	size_t i, p;
	size_t distance;
	size_t cutoff_distance;
	size_t min_distance;
	size_t min_color;
	
	cutoff_distance = 0xFFFFFFFF;
	for( i=0; i<palsize; i++ ) {
		for( p=i+1; p<palsize; p++ ) {
			distance = cdist(palette,i,palette,p);
			if( distance && distance < cutoff_distance ) {
				cutoff_distance = distance;
			}
		}
	}
	cutoff_distance = cutoff_distance / 2;
	
	for( i=0; i<pixelslen; i++ ) {
		min_distance = 0xFFFFFF;
		min_color = 0;
		for( p=0; p<palsize; p++ ) {
			distance = cdist(rgbpixels,i,palette,p);
			if( distance < min_distance ) {
				min_distance = distance;
				min_color = p;
				if( distance <= cutoff_distance ) {
					break;
				}
			}
		}
		palpixels[i] = min_color;
		if( syncrgb ) {
			rgbassign(rgbpixels,i,palette,min_color);
		}
	}
}

#endif //QUANT_IMPLEMENTATION