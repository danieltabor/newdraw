/*
 * This file is based upon the quantizer from libsixel, which in turn in based on
 * the quantizer from pnmcolormap.  The associated license blocks are below.
 *
 * Copyright (c) 2014-2016 Hayaki Saito
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * mediancut algorithm implementation is imported from pnmcolormap.c
 * in netpbm library.
 * http://netpbm.sourceforge.net/
 *
 * *******************************************************************************
 *				  original license block of pnmcolormap.c
 * *******************************************************************************
 *
 *   Derived from ppmquant, originally by Jef Poskanzer.
 *
 *   Copyright (C) 1989, 1991 by Jef Poskanzer.
 *   Copyright (C) 2001 by Bryan Henderson.
 *
 *   Permission to use, copy, modify, and distribute this software and its
 *   documentation for any purpose and without fee is hereby granted, provided
 *   that the above copyright notice appear in all copies and that both that
 *   copyright notice and this permission notice appear in supporting
 *   documentation.  This software is provided "as is" without express or
 *   implied warranty.
 *
 * ******************************************************************************
 *
 * Copyright (c) 2014-2018 Hayaki Saito
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 */

#ifndef __QUANTPNM_H__
#define __QUANTPNM_H__

#include <stdint.h>

/* method for finding the largest dimension for splitting,
 * and sorting by that component */
#define QUANT_LARGE_AUTO   0x0   /* choose automatically the method for finding the largest
									dimension */
#define QUANT_LARGE_NORM   0x1   /* simply comparing the range in RGB space */
#define QUANT_LARGE_LUM	0x2   /* transforming into luminosities before the comparison */

/* method for choosing a color from the box */
#define QUANT_REP_AUTO			0x0  /* choose automatically the method for selecting
										  representative color from each box */
#define QUANT_REP_CENTER_BOX	  0x1  /* choose the center of the box */
#define QUANT_REP_AVERAGE_COLORS  0x2  /* choose the average all the color
										  in the box (specified in Heckbert's paper) */
#define QUANT_REP_AVERAGE_PIXELS  0x3  /* choose the average all the pixels in the box */

/* method for diffusing */
#define QUANT_DIFFUSE_AUTO		0x0  /* choose diffusion type automatically */
#define QUANT_DIFFUSE_NONE		0x1  /* don't diffuse */
#define QUANT_DIFFUSE_ATKINSON	0x2  /* diffuse with Bill Atkinson's method */
#define QUANT_DIFFUSE_FS		  0x3  /* diffuse with Floyd-Steinberg method */
#define QUANT_DIFFUSE_JAJUNI	  0x4  /* diffuse with Jarvis, Judice & Ninke method */
#define QUANT_DIFFUSE_STUCKI	  0x5  /* diffuse with Stucki's method */
#define QUANT_DIFFUSE_BURKES	  0x6  /* diffuse with Burkes' method */
#define QUANT_DIFFUSE_A_DITHER	0x7  /* positionally stable arithmetic dither */
#define QUANT_DIFFUSE_X_DITHER	0x8  /* positionally stable arithmetic xor based dither */

/* quality modes */
#define QUANT_QUALITY_AUTO		0x0  /* choose quality mode automatically */
#define QUANT_QUALITY_HIGH		0x1  /* high quality palette construction */
#define QUANT_QUALITY_LOW		 0x2  /* low quality palette construction */
#define QUANT_QUALITY_FULL		0x3  /* full quality palette construction */
#define QUANT_QUALITY_HIGHCOLOR   0x4  /* high color */
	

/* choose colors using median-cut method */
uint8_t *
quant_pnm_make_palette(
	uint8_t /* in */  *data,  /* data for sampling */
	size_t  /* in */  length, /* data size */
	size_t  /* in */  reqcolors,
	size_t  /* in */  *ncolors,
	size_t  /* in */  *origcolors,
	uint8_t /* in */  methodForLargest,
	uint8_t /* in */  methodForRep,
	uint8_t /* in */  qualityMode);


/* apply color palette into specified pixel buffers */
void
quant_pnm_apply_palette(
	uint8_t  /* out */ *result,
	uint8_t  /* in */  *data,
	size_t   /* in */  width,
	size_t   /* in */  height,
	uint8_t  /* in */  *palette,
	size_t   /* in */  reqcolor,
	uint8_t  /* in */  methodForDiffuse,
	uint8_t  /* in */  foptimize,
	uint8_t  /* in */  foptimize_palette,
	int      /* in */  complexion,
	uint16_t /* in */  *cachetable,
	size_t  /* in */  *ncolors);

//
//
////   end header file   /////////////////////////////////////////////////////
#endif // __QUANTPNM_H__

#ifdef QUANT_IMPLEMENTATION

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <inttypes.h>

/*****************************************************************************
 *
 * quantization
 *
 *****************************************************************************/
	
typedef struct box* boxVector;
struct box {
	unsigned int ind;
	unsigned int colors;
	unsigned int sum;
};

typedef unsigned long sample;
typedef sample * tuple;

struct tupleint {
	/* An ordered pair of a tuple value and an integer, such as you
	   would find in a tuple table or tuple hash.
	   Note that this is a variable length structure.
	*/
	unsigned int value;
	sample tuple[1];
	/* This is actually a variable size array -- its size is the
	   depth of the tuple in question.  Some compilers do not let us
	   declare a variable length array.
	*/
};
typedef struct tupleint ** tupletable;

typedef struct {
	size_t size;
	tupletable table;
} tupletable2;

static unsigned int compareplanePlane;
	/* This is a parameter to compareplane().  We use this global variable
	   so that compareplane() can be called by qsort(), to compare two
	   tuples.  qsort() doesn't pass any arguments except the two tuples.
	*/
static int
compareplane(const void * const arg1,
			 const void * const arg2)
{
	int lhs, rhs;

	typedef const struct tupleint * const * const sortarg;
	sortarg comparandPP  = (sortarg) arg1;
	sortarg comparatorPP = (sortarg) arg2;
	lhs = (int)(*comparandPP)->tuple[compareplanePlane];
	rhs = (int)(*comparatorPP)->tuple[compareplanePlane];

	return lhs - rhs;
}


static int
sumcompare(const void * const b1, const void * const b2)
{
	return (int)((boxVector)b2)->sum - (int)((boxVector)b1)->sum;
}


static tupletable
alloctupletable(
	unsigned int const  /* in */  depth,
	unsigned int const  /* in */  size)
{
	enum { message_buffer_size = 256 };
	char message[message_buffer_size];
	int nwrite;
	unsigned int mainTableSize;
	unsigned int tupleIntSize;
	unsigned int allocSize;
	void * pool;
	tupletable tbl;
	unsigned int i;

	if (UINT_MAX / sizeof(struct tupleint) < size) {
		fprintf(stderr,"size %u is too big for arithmetic",size);
		exit(1);;
	}

	mainTableSize = size * sizeof(struct tupleint *);
	tupleIntSize = sizeof(struct tupleint) - sizeof(sample)
		+ depth * sizeof(sample);

	/* To save the enormous amount of time it could take to allocate
	   each individual tuple, we do a trick here and allocate everything
	   as a single malloc block and suballocate internally.
	*/
	if ((UINT_MAX - mainTableSize) / tupleIntSize < size) {
		fprintf(stderr,"size %u is too big for arithmetic",size);
		exit(1);
	}

	allocSize = mainTableSize + size * tupleIntSize;

	pool = malloc(allocSize);
	if (pool == NULL) {
		fprintf(stderr,"unable to allocate %u bytes for a %u-entry tuple table",allocSize, size);
		exit(1);
	}
	tbl = (tupletable) pool;

	for (i = 0; i < size; ++i)
		tbl[i] = (struct tupleint *)
			((char*)pool + mainTableSize + i * tupleIntSize);

	return tbl;
}


/*
** Here is the fun part, the median-cut colormap generator.  This is based
** on Paul Heckbert's paper "Color Image Quantization for Frame Buffer
** Display", SIGGRAPH '82 Proceedings, page 297.
*/

static tupletable2
newColorMap(unsigned int const newcolors, unsigned int const depth)
{
	tupletable2 colormap;
	unsigned int i;

	colormap.size = 0;
	colormap.table = alloctupletable(depth, newcolors);
	if  (colormap.table) {
		for (i = 0; i < newcolors; ++i) {
			unsigned int plane;
			for (plane = 0; plane < depth; ++plane)
				colormap.table[i]->tuple[plane] = 0;
		}
		colormap.size = newcolors;
	}

	return colormap;
}


static boxVector
newBoxVector(
	unsigned int const  /* in */ colors,
	unsigned int const  /* in */ sum,
	unsigned int const  /* in */ newcolors)
{
	boxVector bv;

	bv = (boxVector)malloc(sizeof(struct box) * (size_t)newcolors);
	if (bv == NULL) {
		fprintf(stderr, "out of memory allocating box vector table\n");
		exit(1);
	}

	/* Set up the initial box. */
	bv[0].ind = 0;
	bv[0].colors = colors;
	bv[0].sum = sum;

	return bv;
}


static void
findBoxBoundaries(tupletable2  const colorfreqtable,
				  unsigned int const depth,
				  unsigned int const boxStart,
				  unsigned int const boxSize,
				  sample			 minval[],
				  sample			 maxval[])
{
/*----------------------------------------------------------------------------
  Go through the box finding the minimum and maximum of each
  component - the boundaries of the box.
-----------------------------------------------------------------------------*/
	unsigned int plane;
	unsigned int i;

	for (plane = 0; plane < depth; ++plane) {
		minval[plane] = colorfreqtable.table[boxStart]->tuple[plane];
		maxval[plane] = minval[plane];
	}

	for (i = 1; i < boxSize; ++i) {
		for (plane = 0; plane < depth; ++plane) {
			sample const v = colorfreqtable.table[boxStart + i]->tuple[plane];
			if (v < minval[plane]) minval[plane] = v;
			if (v > maxval[plane]) maxval[plane] = v;
		}
	}
}



static unsigned int
largestByNorm(sample minval[], sample maxval[], unsigned int const depth)
{

	unsigned int largestDimension;
	unsigned int plane;
	sample largestSpreadSoFar;

	largestSpreadSoFar = 0;
	largestDimension = 0;
	for (plane = 0; plane < depth; ++plane) {
		sample const spread = maxval[plane]-minval[plane];
		if (spread > largestSpreadSoFar) {
			largestDimension = plane;
			largestSpreadSoFar = spread;
		}
	}
	return largestDimension;
}



static unsigned int
largestByLuminosity(sample minval[], sample maxval[], unsigned int const depth)
{
/*----------------------------------------------------------------------------
   This subroutine presumes that the tuple type is either
   BLACKANDWHITE, GRAYSCALE, or RGB (which implies pamP->depth is 1 or 3).
   To save time, we don't actually check it.
-----------------------------------------------------------------------------*/
	unsigned int retval;

	double lumin_factor[3] = {0.2989, 0.5866, 0.1145};

	if (depth == 1) {
		retval = 0;
	} else {
		/* An RGB tuple */
		unsigned int largestDimension;
		unsigned int plane;
		double largestSpreadSoFar;

		largestSpreadSoFar = 0.0;
		largestDimension = 0;

		for (plane = 0; plane < 3; ++plane) {
			double const spread =
				lumin_factor[plane] * (maxval[plane]-minval[plane]);
			if (spread > largestSpreadSoFar) {
				largestDimension = plane;
				largestSpreadSoFar = spread;
			}
		}
		retval = largestDimension;
	}
	return retval;
}



static void
centerBox(unsigned int const boxStart,
		  unsigned int const boxSize,
		  tupletable2  const colorfreqtable,
		  unsigned int const depth,
		  tuple		const newTuple)
{

	unsigned int plane;
	sample minval, maxval;
	unsigned int i;

	for (plane = 0; plane < depth; ++plane) {
		minval = maxval = colorfreqtable.table[boxStart]->tuple[plane];

		for (i = 1; i < boxSize; ++i) {
			sample v = colorfreqtable.table[boxStart + i]->tuple[plane];
			minval = minval < v ? minval: v;
			maxval = maxval > v ? maxval: v;
		}
		newTuple[plane] = (minval + maxval) / 2;
	}
}



static void
averageColors(unsigned int const boxStart,
			  unsigned int const boxSize,
			  tupletable2  const colorfreqtable,
			  unsigned int const depth,
			  tuple		const newTuple)
{
	unsigned int plane;
	sample sum;
	unsigned int i;

	for (plane = 0; plane < depth; ++plane) {
		sum = 0;

		for (i = 0; i < boxSize; ++i) {
			sum += colorfreqtable.table[boxStart + i]->tuple[plane];
		}

		newTuple[plane] = sum / boxSize;
	}
}



static void
averagePixels(unsigned int const boxStart,
			  unsigned int const boxSize,
			  tupletable2 const colorfreqtable,
			  unsigned int const depth,
			  tuple const newTuple)
{

	unsigned int n;
		/* Number of tuples represented by the box */
	unsigned int plane;
	unsigned int i;

	/* Count the tuples in question */
	n = 0;  /* initial value */
	for (i = 0; i < boxSize; ++i) {
		n += (unsigned int)colorfreqtable.table[boxStart + i]->value;
	}

	for (plane = 0; plane < depth; ++plane) {
		sample sum;

		sum = 0;

		for (i = 0; i < boxSize; ++i) {
			sum += colorfreqtable.table[boxStart + i]->tuple[plane]
				* (unsigned int)colorfreqtable.table[boxStart + i]->value;
		}

		newTuple[plane] = sum / n;
	}
}



static tupletable2
colormapFromBv(unsigned int const newcolors,
			   boxVector const bv,
			   unsigned int const boxes,
			   tupletable2 const colorfreqtable,
			   unsigned int const depth,
			   int const methodForRep)
{
	/*
	** Ok, we've got enough boxes.  Now choose a representative color for
	** each box.  There are a number of possible ways to make this choice.
	** One would be to choose the center of the box; this ignores any structure
	** within the boxes.  Another method would be to average all the colors in
	** the box - this is the method specified in Heckbert's paper.  A third
	** method is to average all the pixels in the box.
	*/
	tupletable2 colormap;
	unsigned int bi;

	colormap = newColorMap(newcolors, depth);
	if (!colormap.size) {
		return colormap;
	}

	for (bi = 0; bi < boxes; ++bi) {
		switch (methodForRep) {
		case QUANT_REP_CENTER_BOX:
			centerBox(bv[bi].ind, bv[bi].colors,
					  colorfreqtable, depth,
					  colormap.table[bi]->tuple);
			break;
		case QUANT_REP_AVERAGE_COLORS:
			averageColors(bv[bi].ind, bv[bi].colors,
						  colorfreqtable, depth,
						  colormap.table[bi]->tuple);
			break;
		case QUANT_REP_AVERAGE_PIXELS:
			averagePixels(bv[bi].ind, bv[bi].colors,
						  colorfreqtable, depth,
						  colormap.table[bi]->tuple);
			break;
		default:
			fprintf(stderr, "Internal error: invalid value of methodForRep: %d\n",methodForRep);
			exit(1);
		}
	}
	return colormap;
}


static int
splitBox(boxVector const bv,
		 unsigned int *const boxesP,
		 unsigned int const bi,
		 tupletable2 const colorfreqtable,
		 unsigned int const depth,
		 int const methodForLargest)
{
/*----------------------------------------------------------------------------
   Split Box 'bi' in the box vector bv (so that bv contains one more box
   than it did as input).  Split it so that each new box represents about
   half of the pixels in the distribution given by 'colorfreqtable' for
   the colors in the original box, but with distinct colors in each of the
   two new boxes.

   Assume the box contains at least two colors.
-----------------------------------------------------------------------------*/
	unsigned int const boxStart = bv[bi].ind;
	unsigned int const boxSize  = bv[bi].colors;
	unsigned int const sm	   = bv[bi].sum;

	enum { max_depth= 16 };
	sample minval[max_depth];
	sample maxval[max_depth];

	/* assert(max_depth >= depth); */

	unsigned int largestDimension;
		/* number of the plane with the largest spread */
	unsigned int medianIndex;
	unsigned int lowersum;
		/* Number of pixels whose value is "less than" the median */

	findBoxBoundaries(colorfreqtable, depth, boxStart, boxSize,
					  minval, maxval);

	/* Find the largest dimension, and sort by that component.  I have
	   included two methods for determining the "largest" dimension;
	   first by simply comparing the range in RGB space, and second by
	   transforming into luminosities before the comparison.
	*/
	switch (methodForLargest) {
	case QUANT_LARGE_NORM:
		largestDimension = largestByNorm(minval, maxval, depth);
		break;
	case QUANT_LARGE_LUM:
		largestDimension = largestByLuminosity(minval, maxval, depth);
		break;
	default:
		fprintf(stderr,"Internal error: invalid value of methodForLargest.");
		return 1;
	}

	/* TODO: I think this sort should go after creating a box,
	   not before splitting.  Because you need the sort to use
	   the QUANT_REP_CENTER_BOX method of choosing a color to
	   represent the final boxes
	*/

	/* Set the gross global variable 'compareplanePlane' as a
	   parameter to compareplane(), which is called by qsort().
	*/
	compareplanePlane = largestDimension;
	qsort((char*) &colorfreqtable.table[boxStart], boxSize,
		  sizeof(colorfreqtable.table[boxStart]),
		  compareplane);

	{
		/* Now find the median based on the counts, so that about half
		   the pixels (not colors, pixels) are in each subdivision.  */

		unsigned int i;

		lowersum = colorfreqtable.table[boxStart]->value; /* initial value */
		for (i = 1; i < boxSize - 1 && lowersum < sm / 2; ++i) {
			lowersum += colorfreqtable.table[boxStart + i]->value;
		}
		medianIndex = i;
	}
	/* Split the box, and sort to bring the biggest boxes to the top.  */

	bv[bi].colors = medianIndex;
	bv[bi].sum = lowersum;
	bv[*boxesP].ind = boxStart + medianIndex;
	bv[*boxesP].colors = boxSize - medianIndex;
	bv[*boxesP].sum = sm - lowersum;
	++(*boxesP);
	qsort((char*) bv, *boxesP, sizeof(struct box), sumcompare);

	return 0;
}



static void
mediancut(tupletable2 const colorfreqtable,
		  unsigned int const depth,
		  unsigned int const newcolors,
		  int const methodForLargest,
		  int const methodForRep,
		  tupletable2 *const colormapP)
{
/*----------------------------------------------------------------------------
   Compute a set of only 'newcolors' colors that best represent an
   image whose pixels are summarized by the histogram
   'colorfreqtable'.  Each tuple in that table has depth 'depth'.
   colorfreqtable.table[i] tells the number of pixels in the subject image
   have a particular color.

   As a side effect, sort 'colorfreqtable'.
-----------------------------------------------------------------------------*/
	boxVector bv;
	unsigned int bi;
	unsigned int boxes;
	int multicolorBoxesExist;
	unsigned int i;
	unsigned int sum;

	sum = 0;

	for (i = 0; i < colorfreqtable.size; ++i) {
		sum += colorfreqtable.table[i]->value;
	}

	/* There is at least one box that contains at least 2 colors; ergo,
	   there is more splitting we can do.  */
	bv = newBoxVector(colorfreqtable.size, sum, newcolors);
	if (bv == NULL) {
		fprintf(stderr,"Failed to create box vector\n");
		exit(1);
	}
	boxes = 1;
	multicolorBoxesExist = (colorfreqtable.size > 1);

	/* Main loop: split boxes until we have enough. */
	while (boxes < newcolors && multicolorBoxesExist) {
		/* Find the first splittable box. */
		for (bi = 0; bi < boxes && bv[bi].colors < 2; ++bi)
			;
		if (bi >= boxes) {
			multicolorBoxesExist = 0;
		} else {
			if( splitBox(bv, &boxes, bi,
							  colorfreqtable, depth,
							  methodForLargest) ) {
				fprintf(stderr,"Failed to split box vector\n");
				exit(1);
			}
		}
	}
	*colormapP = colormapFromBv(newcolors, bv, boxes,
								colorfreqtable, depth,
								methodForRep);

	free(bv);
}


static unsigned int
computeHash(uint8_t const *data, unsigned int const depth)
{
	unsigned int hash = 0;
	unsigned int n;

	for (n = 0; n < depth; n++) {
		hash |= (unsigned int)(data[depth - 1 - n] >> 3) << n * 5;
	}

	return hash;
}


static void
computeHistogram(uint8_t const	/* in */  *data,
				 unsigned int		   /* in */  length,
				 unsigned long const	/* in */  depth,
				 tupletable2 * const	/* out */ colorfreqtableP,
				 int const			  /* in */  qualityMode)
{
	typedef unsigned short unit_t;
	unsigned int i, n;
	unit_t *histogram = NULL;
	unit_t *refmap = NULL;
	unit_t *ref;
	unit_t *it;
	unsigned int bucket_index;
	unsigned int step;
	unsigned int max_sample;

	switch (qualityMode) {
	case QUANT_QUALITY_LOW:
		max_sample = 18383;
		step = length / depth / max_sample * depth;
		break;
	case QUANT_QUALITY_HIGH:
		max_sample = 18383;
		step = length / depth / max_sample * depth;
		break;
	case QUANT_QUALITY_FULL:
	default:
		max_sample = 4003079;
		step = length / depth / max_sample * depth;
		break;
	}

	if (length < max_sample * depth) {
		step = 6 * depth;
	}

	if (step <= 0) {
		step = depth;
	}

	#ifdef DEBUG
	fprintf(stderr, "making histogram...\n");
	#endif

	histogram = (unit_t *)calloc( (size_t)(1 << depth * 5), sizeof(unit_t));
	if (histogram == NULL) {
		fprintf(stderr,"unable to allocate memory for histogram.");
		exit(1);
	}
	it = ref = refmap
		= (unsigned short *)malloc((size_t)(1 << depth * 5) * sizeof(unit_t));
	if (!it) {
		fprintf(stderr,"unable to allocate memory for lookup table.");
		exit(1);
	}

	for (i = 0; i < length; i += step) {
		bucket_index = computeHash(data + i, 3);
		if (histogram[bucket_index] == 0) {
			*ref++ = bucket_index;
		}
		if (histogram[bucket_index] < (unsigned int)(1 << sizeof(unsigned short) * 8) - 1) {
			histogram[bucket_index]++;
		}
	}

	colorfreqtableP->size = (unsigned int)(ref - refmap);

	colorfreqtableP->table = alloctupletable(depth, (unsigned int)(ref - refmap));

	for (i = 0; i < colorfreqtableP->size; ++i) {
		if (histogram[refmap[i]] > 0) {
			colorfreqtableP->table[i]->value = histogram[refmap[i]];
			for (n = 0; n < depth; n++) {
				colorfreqtableP->table[i]->tuple[depth - 1 - n]
					= (sample)((*it >> n * 5 & 0x1f) << 3);
			}
		}
		it++;
	}

	#ifdef DEBUG
	fprintf(stderr, "%lu colors found\n", colorfreqtableP->size);
	#endif
	
	free(refmap);
	free(histogram);
}


static void
computeColorMapFromInput(uint8_t const *data,
						 unsigned int const length,
						 unsigned int const depth,
						 unsigned int const reqColors,
						 int const methodForLargest,
						 int const methodForRep,
						 int const qualityMode,
						 tupletable2 * const colormapP,
						 size_t *origcolors)
{
/*----------------------------------------------------------------------------
   Produce a colormap containing the best colors to represent the
   image stream in file 'ifP'.  Figure it out using the median cut
   technique.

   The colormap will have 'reqcolors' or fewer colors in it, unless
   'allcolors' is true, in which case it will have all the colors that
   are in the input.

   The colormap has the same maxval as the input.

   Put the colormap in newly allocated storage as a tupletable2
   and return its address as *colormapP.  Return the number of colors in
   it as *colorsP and its maxval as *colormapMaxvalP.

   Return the characteristics of the input file as
   *formatP and *freqPamP.  (This information is not really
   relevant to our colormap mission; just a fringe benefit).
-----------------------------------------------------------------------------*/
	tupletable2 colorfreqtable = {0, NULL};
	unsigned int i;
	unsigned int n;

	computeHistogram(data, length, depth,
							  &colorfreqtable, qualityMode);
	if (origcolors) {
		*origcolors = colorfreqtable.size;
	}

	if (colorfreqtable.size <= reqColors) {
		#ifdef DEBUG
		fprintf(stderr,
					"Image already has few enough colors (<=%d).  "
					"Keeping same colors.\n", reqColors);
		#endif
		/* *colormapP = colorfreqtable; */
		colormapP->size = colorfreqtable.size;
		colormapP->table = alloctupletable(depth, colorfreqtable.size);
		for (i = 0; i < colorfreqtable.size; ++i) {
			colormapP->table[i]->value = colorfreqtable.table[i]->value;
			for (n = 0; n < depth; ++n) {
				colormapP->table[i]->tuple[n] = colorfreqtable.table[i]->tuple[n];
			}
		}
	} else {
		#ifdef DEBUG
		fprintf(stderr, "choosing %d colors...\n", reqColors);
		#endif
		mediancut(colorfreqtable, depth, reqColors,
						   methodForLargest, methodForRep, colormapP);
		#ifdef DEBUG
		fprintf(stderr, "%lu colors are choosed.\n", colorfreqtable.size);
		#endif
	}

	free(colorfreqtable.table);
}


/* diffuse error energy to surround pixels */
static void
error_diffuse(uint8_t /* in */	*data,	  /* base address of pixel buffer */
			  int		   /* in */	pos,		/* address of the destination pixel */
			  int		   /* in */	depth,	  /* color depth in bytes */
			  int		   /* in */	error,	  /* error energy */
			  int		   /* in */	numerator,  /* numerator of diffusion coefficient */
			  int		   /* in */	denominator /* denominator of diffusion coefficient */)
{
	int c;

	data += pos * depth;

	c = *data + error * numerator / denominator;
	if (c < 0) {
		c = 0;
	}
	if (c >= 1 << 8) {
		c = (1 << 8) - 1;
	}
	*data = (uint8_t)c;
}


static void
diffuse_none(uint8_t *data, int width, int height,
			 int x, int y, int depth, int error)
{
	/* unused */ (void) data;
	/* unused */ (void) width;
	/* unused */ (void) height;
	/* unused */ (void) x;
	/* unused */ (void) y;
	/* unused */ (void) depth;
	/* unused */ (void) error;
}


static void
diffuse_fs(uint8_t *data, int width, int height,
		   int x, int y, int depth, int error)
{
	int pos;

	pos = y * width + x;

	/* Floyd Steinberg Method
	 *		  curr	7/16
	 *  3/16	5/48	1/16
	 */
	if (x < width - 1 && y < height - 1) {
		/* add error to the right cell */
		error_diffuse(data, pos + width * 0 + 1, depth, error, 7, 16);
		/* add error to the left-bottom cell */
		error_diffuse(data, pos + width * 1 - 1, depth, error, 3, 16);
		/* add error to the bottom cell */
		error_diffuse(data, pos + width * 1 + 0, depth, error, 5, 16);
		/* add error to the right-bottom cell */
		error_diffuse(data, pos + width * 1 + 1, depth, error, 1, 16);
	}
}


static void
diffuse_atkinson(uint8_t *data, int width, int height,
				 int x, int y, int depth, int error)
{
	int pos;

	pos = y * width + x;

	/* Atkinson's Method
	 *		  curr	1/8	1/8
	 *   1/8	 1/8	1/8
	 *		   1/8
	 */
	if (y < height - 2) {
		/* add error to the right cell */
		error_diffuse(data, pos + width * 0 + 1, depth, error, 1, 8);
		/* add error to the 2th right cell */
		error_diffuse(data, pos + width * 0 + 2, depth, error, 1, 8);
		/* add error to the left-bottom cell */
		error_diffuse(data, pos + width * 1 - 1, depth, error, 1, 8);
		/* add error to the bottom cell */
		error_diffuse(data, pos + width * 1 + 0, depth, error, 1, 8);
		/* add error to the right-bottom cell */
		error_diffuse(data, pos + width * 1 + 1, depth, error, 1, 8);
		/* add error to the 2th bottom cell */
		error_diffuse(data, pos + width * 2 + 0, depth, error, 1, 8);
	}
}


static void
diffuse_jajuni(uint8_t *data, int width, int height,
			   int x, int y, int depth, int error)
{
	int pos;

	pos = y * width + x;

	/* Jarvis, Judice & Ninke Method
	 *				  curr	7/48	5/48
	 *  3/48	5/48	7/48	5/48	3/48
	 *  1/48	3/48	5/48	3/48	1/48
	 */
	if (pos < (height - 2) * width - 2) {
		error_diffuse(data, pos + width * 0 + 1, depth, error, 7, 48);
		error_diffuse(data, pos + width * 0 + 2, depth, error, 5, 48);
		error_diffuse(data, pos + width * 1 - 2, depth, error, 3, 48);
		error_diffuse(data, pos + width * 1 - 1, depth, error, 5, 48);
		error_diffuse(data, pos + width * 1 + 0, depth, error, 7, 48);
		error_diffuse(data, pos + width * 1 + 1, depth, error, 5, 48);
		error_diffuse(data, pos + width * 1 + 2, depth, error, 3, 48);
		error_diffuse(data, pos + width * 2 - 2, depth, error, 1, 48);
		error_diffuse(data, pos + width * 2 - 1, depth, error, 3, 48);
		error_diffuse(data, pos + width * 2 + 0, depth, error, 5, 48);
		error_diffuse(data, pos + width * 2 + 1, depth, error, 3, 48);
		error_diffuse(data, pos + width * 2 + 2, depth, error, 1, 48);
	}
}


static void
diffuse_stucki(uint8_t *data, int width, int height,
			   int x, int y, int depth, int error)
{
	int pos;

	pos = y * width + x;

	/* Stucki's Method
	 *				  curr	8/48	4/48
	 *  2/48	4/48	8/48	4/48	2/48
	 *  1/48	2/48	4/48	2/48	1/48
	 */
	if (pos < (height - 2) * width - 2) {
		error_diffuse(data, pos + width * 0 + 1, depth, error, 1, 6);
		error_diffuse(data, pos + width * 0 + 2, depth, error, 1, 12);
		error_diffuse(data, pos + width * 1 - 2, depth, error, 1, 24);
		error_diffuse(data, pos + width * 1 - 1, depth, error, 1, 12);
		error_diffuse(data, pos + width * 1 + 0, depth, error, 1, 6);
		error_diffuse(data, pos + width * 1 + 1, depth, error, 1, 12);
		error_diffuse(data, pos + width * 1 + 2, depth, error, 1, 24);
		error_diffuse(data, pos + width * 2 - 2, depth, error, 1, 48);
		error_diffuse(data, pos + width * 2 - 1, depth, error, 1, 24);
		error_diffuse(data, pos + width * 2 + 0, depth, error, 1, 12);
		error_diffuse(data, pos + width * 2 + 1, depth, error, 1, 24);
		error_diffuse(data, pos + width * 2 + 2, depth, error, 1, 48);
	}
}


static void
diffuse_burkes(uint8_t *data, int width, int height,
			   int x, int y, int depth, int error)
{
	int pos;

	pos = y * width + x;

	/* Burkes' Method
	 *				  curr	4/16	2/16
	 *  1/16	2/16	4/16	2/16	1/16
	 */
	if (pos < (height - 1) * width - 2) {
		error_diffuse(data, pos + width * 0 + 1, depth, error, 1, 4);
		error_diffuse(data, pos + width * 0 + 2, depth, error, 1, 8);
		error_diffuse(data, pos + width * 1 - 2, depth, error, 1, 16);
		error_diffuse(data, pos + width * 1 - 1, depth, error, 1, 8);
		error_diffuse(data, pos + width * 1 + 0, depth, error, 1, 4);
		error_diffuse(data, pos + width * 1 + 1, depth, error, 1, 8);
		error_diffuse(data, pos + width * 1 + 2, depth, error, 1, 16);
	}
}

static float
mask_a (int x, int y, int c)
{
	return ((((x + c * 67) + y * 236) * 119) & 255 ) / 128.0 - 1.0;
}

static float
mask_x (int x, int y, int c)
{
	return ((((x + c * 29) ^ y* 149) * 1234) & 511 ) / 256.0 - 1.0;
}

/* lookup closest color from palette with "normal" strategy */
static int
lookup_normal(uint8_t const * const pixel,
			  int const depth,
			  uint8_t const * const palette,
			  int const reqcolor,
			  unsigned short * const cachetable,
			  int const complexion)
{
	int result;
	int diff;
	int r;
	int i;
	int n;
	int distant;
	
	result = (-1);
	diff = INT_MAX;

	/* don't use cachetable in 'normal' strategy */
	(void) cachetable;

	for (i = 0; i < reqcolor; i++) {
		distant = 0;
		r = pixel[0] - palette[i * depth + 0];
		distant += r * r * complexion;
		for (n = 1; n < depth; ++n) {
			r = pixel[n] - palette[i * depth + n];
			distant += r * r;
		}
		if (distant < diff) {
			diff = distant;
			result = i;
		}
	}

	return result;
}


/* lookup closest color from palette with "fast" strategy */
static int
lookup_fast(uint8_t const * const pixel,
			int const depth,
			uint8_t const * const palette,
			int const reqcolor,
			unsigned short * const cachetable,
			int const complexion)
{
	int result;
	unsigned int hash;
	int diff;
	int cache;
	int i;
	int distant;

	/* don't use depth in 'fast' strategy because it's always 3 */
	(void) depth;

	result = (-1);
	diff = INT_MAX;
	hash = computeHash(pixel, 3);

	cache = cachetable[hash];
	if (cache) {  /* fast lookup */
		return cache - 1;
	}
	/* collision */
	for (i = 0; i < reqcolor; i++) {
		distant = 0;
#if 0
		for (n = 0; n < 3; ++n) {
			r = pixel[n] - palette[i * 3 + n];
			distant += r * r;
		}
#elif 1  /* complexion correction */
		distant = (pixel[0] - palette[i * 3 + 0]) * (pixel[0] - palette[i * 3 + 0]) * complexion
				+ (pixel[1] - palette[i * 3 + 1]) * (pixel[1] - palette[i * 3 + 1])
				+ (pixel[2] - palette[i * 3 + 2]) * (pixel[2] - palette[i * 3 + 2])
				;
#endif
		if (distant < diff) {
			diff = distant;
			result = i;
		}
	}
	cachetable[hash] = result + 1;

	return result;
}


static int
lookup_mono_darkbg(uint8_t const * const pixel,
				   int const depth,
				   uint8_t const * const palette,
				   int const reqcolor,
				   unsigned short * const cachetable,
				   int const complexion)
{
	int n;
	int distant;

	/* unused */ (void) palette;
	/* unused */ (void) cachetable;
	/* unused */ (void) complexion;

	distant = 0;
	for (n = 0; n < depth; ++n) {
		distant += pixel[n];
	}
	return distant >= 128 * reqcolor ? 1: 0;
}


static int
lookup_mono_lightbg(uint8_t const * const pixel,
					int const depth,
					uint8_t const * const palette,
					int const reqcolor,
					unsigned short * const cachetable,
					int const complexion)
{
	int n;
	int distant;

	/* unused */ (void) palette;
	/* unused */ (void) cachetable;
	/* unused */ (void) complexion;

	distant = 0;
	for (n = 0; n < depth; ++n) {
		distant += pixel[n];
	}
	return distant < 128 * reqcolor ? 1: 0;
}


/* choose colors using median-cut method */
uint8_t *
quant_pnm_make_palette(
	uint8_t /* in */  *data,  /* data for sampling */
	size_t  /* in */  length, /* data size */
	size_t  /* in */  reqcolors,
	size_t  /* in */  *ncolors,
	size_t  /* in */  *origcolors,
	uint8_t /* in */  methodForLargest,
	uint8_t /* in */  methodForRep,
	uint8_t /* in */  qualityMode)
{
	unsigned int i;
	unsigned int n;
	int ret;
	tupletable2 colormap;
	unsigned int depth = 3;
	uint8_t *palette;

	if( methodForLargest == QUANT_LARGE_AUTO ) {
		methodForLargest = QUANT_LARGE_NORM;
	}
	if( methodForRep == QUANT_REP_AUTO ) {
		methodForRep = QUANT_REP_CENTER_BOX;
	}
	if( qualityMode == QUANT_QUALITY_AUTO ) {
		if( reqcolors <= 8 ) {
			qualityMode = QUANT_QUALITY_HIGH;
		} else {
			qualityMode = QUANT_QUALITY_LOW;
		}
	}
	
	computeColorMapFromInput(data, length, depth,
								   reqcolors, methodForLargest,
								   methodForRep, qualityMode,
								   &colormap, origcolors);
	*ncolors = colormap.size;
	#ifdef DEBUG
	fprintf(stderr, "tupletable size: %zu\n", *ncolors);
	#endif
	palette = (uint8_t *)malloc(*ncolors * depth);
	if( palette == 0 ) {
		fprintf(stderr,"Failed to allocate palette\n");
		exit(1);
	}
	for (i = 0; i < *ncolors; i++) {
		for (n = 0; n < depth; ++n) {
			(palette)[i * depth + n] = colormap.table[i]->tuple[n];
		}
	}

	free(colormap.table);
	return palette;
}


/* apply color palette into specified pixel buffers */
void
quant_pnm_apply_palette(
	uint8_t  /* out */ *result,
	uint8_t  /* in */  *data,
	size_t   /* in */  width,
	size_t   /* in */  height,
	uint8_t  /* in */  *palette,
	size_t   /* in */  reqcolor,
	uint8_t  /* in */  methodForDiffuse,
	uint8_t  /* in */  foptimize,
	uint8_t  /* in */  foptimize_palette,
	int      /* in */  complexion,
	uint16_t /* in */  *cachetable,
	size_t  /* in */  *ncolors)
{
	typedef int component_t;
	unsigned int depth = 3;
	int pos, n, x, y, sum1, sum2;
	component_t offset;
	int color_index;
	unsigned short *indextable;
	uint8_t new_palette[3*256];
	unsigned short migration_map[256];
	float (*f_mask) (int x, int y, int c) = NULL;
	void (*f_diffuse)(uint8_t *data, int width, int height,
					  int x, int y, int depth, int offset);
	int (*f_lookup)(uint8_t const * const pixel,
					int const depth,
					uint8_t const * const palette,
					int const reqcolor,
					unsigned short * const cachetable,
					int const complexion);

	if( methodForDiffuse == QUANT_DIFFUSE_AUTO ) {
		if( reqcolor > 16 ) {
			methodForDiffuse = QUANT_DIFFUSE_FS;
		} else {
			methodForDiffuse = QUANT_DIFFUSE_ATKINSON;
		}
	}
	
	/* check bad reqcolor */
	if (reqcolor < 1) {
		fprintf(stderr,"quant_apply_palette: a bad argument is detected, reqcolor < 0.");
		exit(1);
	}

	switch (methodForDiffuse) {
	case QUANT_DIFFUSE_NONE:
		f_diffuse = diffuse_none;
		break;
	case QUANT_DIFFUSE_ATKINSON:
		f_diffuse = diffuse_atkinson;
		break;
	case QUANT_DIFFUSE_FS:
		f_diffuse = diffuse_fs;
		break;
	case QUANT_DIFFUSE_JAJUNI:
		f_diffuse = diffuse_jajuni;
		break;
	case QUANT_DIFFUSE_STUCKI:
		f_diffuse = diffuse_stucki;
		break;
	case QUANT_DIFFUSE_BURKES:
		f_diffuse = diffuse_burkes;
		break;
	case QUANT_DIFFUSE_A_DITHER:
		f_diffuse = diffuse_none;
		f_mask = mask_a;
		break;
	case QUANT_DIFFUSE_X_DITHER:
		f_diffuse = diffuse_none;
		f_mask = mask_x;
		break;
	default:
		#ifdef DEBUG
		fprintf(stderr, "Internal error: invalid value of"
							" methodForDiffuse: %d\n",
					methodForDiffuse);
		#endif
		f_diffuse = diffuse_none;
		break;
	}

	f_lookup = NULL;
	if (reqcolor == 2) {
		sum1 = 0;
		sum2 = 0;
		for (n = 0; n < depth; ++n) {
			sum1 += palette[n];
		}
		for (n = depth; n < depth + depth; ++n) {
			sum2 += palette[n];
		}
		if (sum1 == 0 && sum2 == 255 * 3) {
			f_lookup = lookup_mono_darkbg;
		} else if (sum1 == 255 * 3 && sum2 == 0) {
			f_lookup = lookup_mono_lightbg;
		}
	}
	if (f_lookup == NULL) {
		if (foptimize && depth == 3) {
			f_lookup = lookup_fast;
		} else {
			f_lookup = lookup_normal;
		}
	}

	indextable = cachetable;
	if (cachetable == NULL && f_lookup == lookup_fast) {
		indextable = (unsigned short *)calloc(
															  (size_t)(1 << depth * 5),
															  sizeof(unsigned short));
		if (!indextable) {
			fprintf(stderr, "Unable to allocate memory for indextable.\n");
			exit(1);
		}
	}

	if (foptimize_palette) {
		*ncolors = 0;

		memset(new_palette, 0x00, sizeof(256 * depth));
		memset(migration_map, 0x00, sizeof(migration_map));

		if (f_mask) {
			for (y = 0; y < height; ++y) {
				for (x = 0; x < width; ++x) {
					uint8_t copy[depth];
					int d;
					int val;

					pos = y * width + x;
					for (d = 0; d < depth; d ++) {
						val = data[pos * depth + d] + f_mask(x, y, d) * 32;
						copy[d] = val < 0 ? 0 : val > 255 ? 255 : val;
					}
					color_index = f_lookup(copy, depth,
										   palette, reqcolor, indextable, complexion);
					if (migration_map[color_index] == 0) {
						result[pos] = *ncolors;
						for (n = 0; n < depth; ++n) {
							new_palette[*ncolors * depth + n] = palette[color_index * depth + n];
						}
						++*ncolors;
						migration_map[color_index] = *ncolors;
					} else {
						result[pos] = migration_map[color_index] - 1;
					}
				}
			}
			memcpy(palette, new_palette, (size_t)(*ncolors * depth));
		} else {
			for (y = 0; y < height; ++y) {
				for (x = 0; x < width; ++x) {
					pos = y * width + x;
					color_index = f_lookup(data + (pos * depth), depth,
										   palette, reqcolor, indextable, complexion);
					if (migration_map[color_index] == 0) {
						result[pos] = *ncolors;
						for (n = 0; n < depth; ++n) {
							new_palette[*ncolors * depth + n] = palette[color_index * depth + n];
						}
						++*ncolors;
						migration_map[color_index] = *ncolors;
					} else {
						result[pos] = migration_map[color_index] - 1;
					}
					for (n = 0; n < depth; ++n) {
						offset = data[pos * depth + n] - palette[color_index * depth + n];
						f_diffuse(data + n, width, height, x, y, depth, offset);
					}
				}
			}
			memcpy(palette, new_palette, (size_t)(*ncolors * depth));
		}
	} else {
		if (f_mask) {
			for (y = 0; y < height; ++y) {
				for (x = 0; x < width; ++x) {
					uint8_t copy[depth];
					int d;
					int val;

					pos = y * width + x;
					for (d = 0; d < depth; d ++) {
						val = data[pos * depth + d] + f_mask(x, y, d) * 32;
						copy[d] = val < 0 ? 0 : val > 255 ? 255 : val;
					}
					result[pos] = f_lookup(copy, depth,
										   palette, reqcolor, indextable, complexion);
				}
			}
		} else {
			for (y = 0; y < height; ++y) {
				for (x = 0; x < width; ++x) {
					pos = y * width + x;
					color_index = f_lookup(data + (pos * depth), depth,
										   palette, reqcolor, indextable, complexion);
					result[pos] = color_index;
					for (n = 0; n < depth; ++n) {
						offset = data[pos * depth + n] - palette[color_index * depth + n];
						f_diffuse(data + n, width, height, x, y, depth, offset);
					}
				}
			}
		}
		*ncolors = reqcolor;
	}

	if (cachetable == NULL) {
		free(indextable);
	}
}

#endif //QUANT_IMPLEMENTATION
