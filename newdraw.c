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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <term.h>
#include <errno.h>
#include <arpa/inet.h>

#define UTF8_IMPLEMENTATION
#define UTF8_EXT_CHARACTER_SETS
#include "utf8.h"

#define DIRNL 0x0A
#define DIRUP 0x41
#define DIRDN 0x42
#define DIRRT 0x43
#define DIRLT 0x44

#define CM_16   0
#define CM_256  1
#define CM_TRUE 2

#ifdef DEBUG
uint8_t lastinput[8];
size_t lastinput_len;
#endif

//linechars[up][right][down][left][this]
uint16_t linechars[3][3][3][3][2] = {
	{ //up=0
		{ //right=0
			{ //down=0
				//up=0 right=0 down=0 left=0
				{ 0x2500, 0x2550 },
				//up=0 right=0 down=0 left=1
				{ 0x2500, 0x2550 },
				//up=0 right=0 down=0 left=2
				{ 0x2500, 0x2550 },
			},
			{ //down=1
				//up=0 right=0 down=1 left=0
				{ 0x2502, 0x2551 },
				//up=0 right=0 down=1 left=1
				{ 0x2510, 0x2557 },
				//up=0 right=0 down=1 left=2
				{ 0x2510, 0x2555 },
			},
			{ //down=2
				//up=0 right=0 down=2 left=0
				{ 0x2502, 0x2551 },
				//up=0 right=0 down=2 left=1
				{ 0x2510, 0x2556 },
				//up=0 right=0 down=2 left=2
				{ 0x2510, 0x2557 },
			},
		},
		{ //right=1
			{ //down=0
				//up=0 right=1 down=0 left=0
				{ 0x2500, 0x2550 },
				//up=0 right=1 down=0 left=1
				{ 0x2500, 0x2550 },
				//up=0 right=1 down=0 left=2
				{ 0x2500, 0x2550 },
			},
			{ //down=1
				//up=0 right=1 down=1 left=0
				{ 0x250C, 0x2554 },
				//up=0 right=1 down=1 left=1
				{ 0x252C, 0x2564 },
				//up=0 right=1 down=1 left=2
				{ 0x252C, 0x2564 },
			},
			{ //down=2
				//up=0 right=1 down=2 left=0
				{ 0x250C, 0x2553 },
				//up=0 right=1 down=2 left=1
				{ 0x252C, 0x2565 },
				//up=0 right=1 down=2 left=2
				{ 0x2565, 0x2566 },
			},
		},
		{ //right=2
			{ //down=0
				//up=0 right=2 down=0 left=0
				{ 0x2500, 0x2550 },
				//up=0 right=2 down=0 left=1
				{ 0x2500, 0x2550 },
				//up=0 right=2 down=0 left=2
				{ 0x2500, 0x2550 },
			},
			{ //down=1
				//up=0 right=2 down=1 left=0
				{ 0x250C, 0x2552 },
				//up=0 right=2 down=1 left=1
				{ 0x252C, 0x2564 },
				//up=0 right=2 down=1 left=2
				{ 0x252C, 0x2564 },
			},
			{ //down=2
				//up=0 right=2 down=2 left=0
				{ 0x250C, 0x2554 },
				//up=0 right=2 down=2 left=1
				{ 0x2565, 0x2566 },
				//up=0 right=2 down=2 left=2
				{ 0x252C, 0x2566 },
			},
		},
	},
	{ //up=1
		{ //right=0
			{ //down=0
				//up=1 right=0 down=0 left=0
				{ 0x2502, 0x2551 },
				//up=1 right=0 down=0 left=1
				{ 0x2518, 0x255D },
				//up=1 right=0 down=0 left=2
				{ 0x2518, 0x255B },
			},
			{ //down=1
				//up=1 right=0 down=1 left=0
				{ 0x2502, 0x2551 },
				//up=1 right=0 down=1 left=1
				{ 0x2524, 0x2563 },
				//up=1 right=0 down=1 left=2
				{ 0x2524, 0x2561 },
			},
			{ //down=2
				//up=1 right=0 down=2 left=0
				{ 0x2502, 0x2551 },
				//up=1 right=0 down=2 left=1
				{ 0x2524, 0x2562 },
				//up=1 right=0 down=2 left=2
				{ 0x2561, 0x2563 },
			},
		},
		{ //right=1
			{ //down=0
				//up=1 right=1 down=0 left=0
				{ 0x2514, 0x255A },
				//up=1 right=1 down=0 left=1
				{ 0x2534, 0x2567 },
				//up=1 right=1 down=0 left=2
				{ 0x2534, 0x2567 },
			},
			{ //down=1
				//up=1 right=1 down=1 left=0
				{ 0x251C, 0x2560 },
				//up=1 right=1 down=1 left=1
				{ 0x253C, 0x256C },
				//up=1 right=1 down=1 left=2
				{ 0x253C, 0x256C },
			},
			{ //down=2
				//up=1 right=1 down=1 left=0
				{ 0x251C, 0x2560 },
				//up=1 right=1 down=1 left=1
				{ 0x253C, 0x256C },
				//up=1 right=1 down=1 left=2
				{ 0x253C, 0x256C },
			},
		},
		{ //right=2
			{ //down=0
				//up=1 right=2 down=0 left=0
				{ 0x2514, 0x2558 },
				//up=1 right=2 down=0 left=1
				{ 0x2534, 0x2567 },
				//up=1 right=2 down=0 left=2
				{ 0x2534, 0x2567 },
			},
			{ //down=1
				//up=1 right=2 down=1 left=0
				{ 0x251C, 0x255E },
				//up=1 right=2 down=1 left=1
				{ 0x253C, 0x256A },
				//up=1 right=2 down=1 left=2
				{ 0x253C, 0x256A },
			},
			{ //down=2
				//up=1 right=2 down=2 left=0
				{ 0x255E, 0x2560 },
				//up=1 right=2 down=2 left=1
				{ 0x253C, 0x256C },
				//up=1 right=2 down=2 left=2
				{ 0x256A, 0x256C },
			},
		},
	},
	{ //up=2
		{ //right=0
			{ //down=0
				//up=2 right=0 down=0 left=0
				{ 0x2502, 0x2551 },
				//up=2 right=0 down=0 left=1
				{ 0x2518, 0x255C },
				//up=2 right=0 down=0 left=2
				{ 0x2518, 0x255D },
			},
			{ //down=1
				//up=2 right=0 down=1 left=0
				{ 0x2502, 0x2551 },
				//up=2 right=0 down=1 left=1
				{ 0x2524, 0x2562 },
				//up=2 right=0 down=1 left=2
				{ 0x2561, 0x2563 },
			},
			{ //down=2
				//up=2 right=0 down=2 left=0
				{ 0x2502, 0x2551 },
				//up=2 right=0 down=2 left=1
				{ 0x2524, 0x2562 },
				//up=2 right=0 down=2 left=2
				{ 0x2525, 0x2563 },
			},
		},
		{ //right=1
			{ //down=0
				//up=2 right=1 down=0 left=0
				{ 0x2514, 0x2559 },
				//up=2 right=1 down=0 left=1
				{ 0x2534, 0x2568 },
				//up=2 right=1 down=0 left=2
				{ 0x2568, 0x2569 },
			},
			{ //down=1
				//up=2 right=1 down=1 left=0
				{ 0x251C, 0x255F },
				//up=2 right=1 down=1 left=1
				{ 0x253C, 0x256C },
				//up=2 right=1 down=1 left=2
				{ 0x253C, 0x256C },
			},
			{ //down=2
				//up=2 right=1 down=2 left=0
				{ 0x251C, 0x255F },
				//up=2 right=1 down=2 left=1
				{ 0x253C, 0x256B },
				//up=2 right=1 down=2 left=2
				{ 0x253C, 0x256C },
			},
		},
		{ //right=2
			{ //down=0
				//up=2 right=2 down=0 left=0
				{ 0x2514, 0x255A },
				//up=2 right=2 down=0 left=1
				{ 0x2568, 0x2569 },
				//up=2 right=2 down=0 left=2
				{ 0x2534, 0x2569 },
			},
			{ //down=1
				//up=2 right=2 down=1 left=0
				{ 0x255E, 0x2560 },
				//up=2 right=2 down=1 left=1
				{ 0x253C, 0x256C },
				//up=2 right=2 down=1 left=2
				{ 0x256A, 0x256C },
			},
			{ //down=2
				//up=2 right=2 down=2 left=0
				{ 0x251C, 0x2560 },
				//up=2 right=2 down=2 left=1
				{ 0x253C, 0x256C },
				//up=2 right=2 down=2 left=2
				{ 0x253C, 0x256C },
			},
		},
	},
};

uint8_t fgcolors[] = {30, 31, 32, 33, 34, 35, 36, 37,  90,  91,  92,  93,  94,  95,  96,  97};
uint8_t bgcolors[] = {40, 41, 42, 43, 44, 45, 46, 47, 100, 101, 102, 103, 104, 105, 106, 107};
uint8_t color_mode = CM_16;
uint32_t codepage = 0x00002500;
uint32_t *charset = 0;
size_t   charsetlen = 0;

size_t win_width;
size_t win_height;
uint8_t esc = 0;
uint32_t clear_fgcolor;
uint32_t fgcolor;
uint32_t clear_bgcolor;
uint32_t bgcolor;
uint8_t reverse = 0;
uint8_t blink = 0;
uint8_t bold = 0;
uint8_t underline = 0;
uint8_t insert = 0;
uint8_t save_input = 0;
char save_path[256];
uint8_t save_path_len = 0;
uint8_t export_input = 0;
char export_path[256];
uint8_t export_spaces = 1;
uint8_t export_clear = 1;
uint8_t export_path_len = 0;
uint8_t fgcolor_input = 0;
uint8_t bgcolor_input = 0;
uint8_t codepage_input = 0;

typedef struct {
	uint32_t fgcolor;
	uint32_t bgcolor;
	uint8_t reverse;
	uint8_t blink;
	uint8_t bold;
	uint8_t underline;
	uint8_t line;
	uint32_t character;
} cell_t;

cell_t *canvas = 0;
size_t canvas_width = 80;
size_t canvas_height = 22;
size_t scroll_x = 0;
size_t scroll_y = 0;
size_t cursor_x = 0;
size_t cursor_y = 0;
ssize_t copy_start_x = -1;
ssize_t copy_start_y = -1;
ssize_t copy_end_x   = -1;
ssize_t copy_end_y   = -1;

void clear_canvas() {
	size_t x;
	size_t y;
	cell_t *cell;
	clear_fgcolor = fgcolor;
	clear_bgcolor = bgcolor;
	for( y=0; y<canvas_height; y++ ) {
		for( x=0; x<canvas_width; x++ ) {
			cell = &(canvas[ y*canvas_width+x ]);
			cell->fgcolor = fgcolor;
			cell->bgcolor = bgcolor;
			cell->reverse = 0;
			cell->blink = 0;
			cell->bold = 0;
			cell->underline = 0;
			cell->line = 0;
			cell->character = 0;
		}
	}
}

void init_canvas() {
	canvas = (cell_t*)realloc(canvas,canvas_width*canvas_height*sizeof(cell_t));
	if( canvas == 0 ) {
		printf("Failed to allocate canvas\n");
		exit(1);
	}
	if( color_mode == CM_TRUE ) {
		fgcolor = 0x00FFFFFF;
		bgcolor = 0x00000000;
	} else {
		fgcolor = 15;
		bgcolor = 0;
	}
	clear_canvas();
}

int move_cursor(uint8_t dir) {
	if( dir == DIRNL ) {
		cursor_x = 0;
		scroll_x = 0;
		if( cursor_y < canvas_height-1 ) {
			cursor_y++;
			while( scroll_y + win_height-3 < cursor_y ) {
				scroll_y++;
			}
		}
		return 1;
	}
	else if( dir == DIRUP ) {
		if( cursor_y > 0 ) {
			cursor_y--;
			while( scroll_y > cursor_y ) {
				scroll_y--;
			}
			return 1;
		}
	}
	else if( dir == DIRDN ) {
		if( cursor_y < canvas_height-1 ) {
			cursor_y++;
			while( scroll_y + win_height-3 < cursor_y ) {
				scroll_y++;
			}
			return 1;
		}
	}
	else if( dir == DIRLT ) {
		if( cursor_x > 0 ) {
			cursor_x--;
			while( scroll_x > cursor_x ) {
				scroll_x--;
			}
			return 1;
		}
		else { //Wrap around
			if( cursor_y > 0 ) {
				cursor_x = canvas_width-1;
				cursor_y--;
				while( scroll_x + win_width <= cursor_x ) {
					scroll_x++;
				}
				while( scroll_y > cursor_y ) {
					scroll_y--;
				}
				return 1;
			}
		}
	}
	else if( dir == DIRRT ) {
		if( cursor_x < canvas_width-1 ) {
			cursor_x++;
			while( scroll_x + win_width <= cursor_x ) {
				scroll_x++;
			}
			return 1;
		}
		else { //Wrap around
			if( cursor_y < canvas_height-1 ) {
				cursor_x = 0;
				scroll_x = 0;
				cursor_y++;
				while( scroll_y + win_height-3 < cursor_y ) {
					scroll_y++;
				}
				return 1;
			}
		}
	}
	return 0;
}

void set_line_character(size_t x, size_t y) {
	uint8_t up    = 0;
	uint8_t right = 0;
	uint8_t down  = 0;
	uint8_t left  = 0;
	uint8_t this  = 0;
	this = canvas[(y)*canvas_width + x].line;
	if( this ) {
		if( y ) { 
			up = canvas[(y-1)*canvas_width + x].line; 
		}
		if( x < canvas_width-1 ) { 
			right = canvas[(y)*canvas_width + x + 1].line; 
		}
		if( y < canvas_height-1 ) {
			down = canvas[(y+1)*canvas_width + x].line; 
		}
		if( x ) { 
			left = canvas[(y)*canvas_width + x - 1].line; 
		}
		canvas[(y)*canvas_width + x].character = linechars[up][right][down][left][this-1];
	}
}

void update_line(size_t x, size_t y) {
	set_line_character(x,y);
	if( y ) {
		set_line_character(x,y-1);
	}
	if( x < canvas_width-1 ) {
		set_line_character(x+1,y);
	}
	if( y < canvas_height-1 ) {
		set_line_character(x,y+1);
	}
	if( x ) {
		set_line_character(x-1,y);
	}
}

void draw_line() {
	cell_t *cell = &(canvas[(cursor_y)*canvas_width + cursor_x]);
	cell->fgcolor = fgcolor;
	cell->bgcolor = bgcolor;
	cell->reverse = reverse;
	cell->blink = blink;
	cell->bold = bold;
	cell->underline = underline;
	cell->line = (cell->line + 1) % 3;
	cell->character = 0;
	update_line(cursor_x,cursor_y);
}

void delete_cell() {
	size_t i = (cursor_y)*canvas_width + cursor_x; //Cursor Position
	size_t j = (cursor_y+1)*canvas_width - 1; //End of line
	cell_t *cell;
	if( insert ) {
		while( i < j ) {
			canvas[i] = canvas[i+1];
			i++;
		}
	}
	cell = &(canvas[i]);
	cell->fgcolor = clear_fgcolor;
	cell->bgcolor = clear_bgcolor;
	cell->reverse = 0;
	cell->blink = 0;
	cell->bold = 0;
	cell->underline = 0;
	cell->character = 0;
	if( cell->line ) {
		cell->line = 0;
		update_line(cursor_x,cursor_y);
	}
}

void insert_cell(uint32_t c) {
	size_t i = (cursor_y)*canvas_width + cursor_x; //Cursor Position
	size_t j = (cursor_y+1)*canvas_width - 1; //End of line
	cell_t *cell = &(canvas[i]);
	if( insert ) {
		while( j > i ) {
			canvas[j] = canvas[j-1];
			j--;
		}
	}
	cell->fgcolor = fgcolor;
	cell->bgcolor = bgcolor;
	cell->reverse = reverse;
	cell->blink = blink;
	cell->bold = bold;
	cell->underline = underline;
	cell->character = c;
	if( cell->line ) {
		cell->line = 0;
		update_line(cursor_x,cursor_y);
	}
	move_cursor(DIRRT);
}

void apply_character_attributes() {
	cell_t *cell = &(canvas[(cursor_y)*canvas_width + cursor_x]);
	cell->fgcolor = fgcolor;
	cell->bgcolor = bgcolor;
	cell->reverse = reverse;
	cell->blink = blink;
	cell->bold = bold;
	cell->underline = underline;
}

void grab_character_attributes() {
	cell_t *cell = &(canvas[(cursor_y)*canvas_width + cursor_x]);
	fgcolor = cell->fgcolor;
	bgcolor = cell->bgcolor;
	reverse = cell->reverse;
	blink = cell->blink;
	bold = cell->bold;
	underline = cell->underline;
}

void paste_block() {
	size_t srcx;
	size_t srcy;
	size_t dstx;
	size_t dsty;
	if( copy_start_x == -1 || copy_start_y == -1 ||
		  copy_end_x == -1 || copy_end_y == -1 ) {
		return;
	}
	for( srcy=copy_start_y, dsty=cursor_y; srcy<=copy_end_y && dsty<canvas_height; srcy++, dsty++ ) {
		for( srcx=copy_start_x, dstx=cursor_x; srcx<=copy_end_x && dstx<canvas_width; srcx++, dstx++ ) {
			canvas[dsty*canvas_width + dstx] = canvas[srcy*canvas_width + srcx];
		}
	}
}

void save() {
	size_t i;
	FILE* fp;
	uint32_t tmp;
	fp = fopen(save_path,"wb");
	if( fp == 0 ) {
		printf("Failed to save file\n");
		exit(1);
	}
	fprintf(fp,"newdraw");
	tmp = htonl(canvas_width);
	fwrite(&tmp,4,1,fp);
	tmp = htonl(canvas_height);
	fwrite(&tmp,4,1,fp);
	fwrite(&color_mode,1,1,fp);
	for( i=0; i<canvas_width*canvas_height; i++ ) {
		tmp = htonl(canvas[i].fgcolor);
		fwrite(&tmp,4,1,fp);
		tmp = htonl(canvas[i].bgcolor);
		fwrite(&tmp,4,1,fp);
		fwrite(&(canvas[i].reverse),1,1,fp);
		fwrite(&(canvas[i].blink),1,1,fp);
		fwrite(&(canvas[i].bold),1,1,fp);
		fwrite(&(canvas[i].underline),1,1,fp);
		fwrite(&(canvas[i].line),1,1,fp);
		tmp = htonl(canvas[i].character);
		fwrite(&tmp,4,1,fp);
	}
}

void load(char* path, int resize) {
	size_t x;
	size_t y;
	uint32_t tmp;
	FILE* fp;
	uint32_t file_width;
	uint32_t file_height;
	cell_t *cell;
	char magic[7];
	
	fp = fopen(path,"rb");
	if( fp == 0 ) {
		printf("Failed to load file - could not open path\n");
		exit(1);
	}
	fread(magic,7,1,fp);
	if( strncmp(magic,"newdraw",7) !=  0 ){
		printf("Failed to load file - not in newdraw binary format\n");
		exit(1);
	}
	tmp = 0;
	fread(&tmp,4,1,fp);
	file_width = ntohl(tmp);
	tmp = 0;
	fread(&tmp,4,1,fp);
	file_height = ntohl(tmp);
	if( canvas_width == 0 || canvas_height == 0 ) {
		printf("Failed to load file - bad width and/or height\n");
		exit(1);
	}
	if( !resize ) {
		canvas_width = file_width;
		canvas_height = file_height;
	}
	fread(&color_mode,1,1,fp);
	init_canvas();
	for( y=0; y<file_height; y++ ) {
		for( x=0; x<file_width; x++ ) {
			if( y<canvas_height && x<canvas_width ) {
				//Read the cell into the canvas
				cell = &(canvas[y*canvas_width+x]);
				tmp = 0;
				if( fread(&tmp,4,1,fp) != 1 ) { goto load_done; }
				cell->fgcolor = ntohl(tmp);
				if( fread(&tmp,4,1,fp) != 1 ) { goto load_done; }
				cell->bgcolor = ntohl(tmp);
				if( fread(&(cell->reverse),1,1,fp) != 1 ) { goto load_done; }
				if( fread(&(cell->blink),1,1,fp) != 1 ) { goto load_done; }
				if( fread(&(cell->bold),1,1,fp) != 1 ) { goto load_done; }
				if( fread(&(cell->underline),1,1,fp) != 1 ) { goto load_done; }
				if( fread(&(cell->line),1,1,fp) != 1 ) { goto load_done; }
				tmp = 0;
				if( fread(&tmp,4,1,fp) != 1 ) { goto load_done; }
				cell->character = ntohl(tmp);
				printf("cell->character: 0x%08X\n",cell->character);
			}
			else {
				//The cell falls outside the canvas, so skip it
				if( fseek(fp,17,SEEK_CUR) < 0 ) { goto load_done; }
			}
		}
	}
	load_done:
	return;
}

void export() {
	size_t canvas_x;
	size_t canvas_y;
	char* c;
	cell_t *cell;
	uint32_t scr_fgcolor;
	uint32_t scr_bgcolor;
	uint8_t scr_reverse;
	uint8_t scr_blink;
	uint8_t scr_bold;
	uint8_t scr_underline;
	uint8_t reset;
	uint8_t move_cursor = 0;
	FILE* fp = fopen(export_path,"wb");
	if( fp == 0 ) {
		printf("Failed to export file\n");
		exit(1);
	}
	if( export_clear ) {
		fprintf(fp,"\x1b[2J\x1b[H");
	}
	fprintf(fp,"\x1b[0m");
	reset = 1;
	for( canvas_y=0; canvas_y<canvas_height; canvas_y++ ) {
		for( canvas_x=0; canvas_x<canvas_width; canvas_x++ ) {
			cell = &(canvas[canvas_y*canvas_width+canvas_x]);
			if( !export_spaces && cell->character == 0 ) {
				if( ! reset ) {
					fprintf(fp,"\x1b[0m");
					reset = 1;
				}
				move_cursor = 1;
			} else {
				if( reset ||
						scr_fgcolor != cell->fgcolor ||
						scr_bgcolor != cell->bgcolor ||
						scr_reverse != cell->reverse ||
						scr_blink != cell->blink ||
						scr_bold != cell->bold ||
						scr_underline != cell->underline ) {
					if( ! reset ) {
						fprintf(fp,"\x1b[0m");
					}
					reset = 0;
					if( move_cursor ) {
						fprintf(fp,"\x1b[%ld;%ldH",canvas_y+1, canvas_x+1);
						move_cursor = 0;
					}
					scr_fgcolor = cell->fgcolor;
					scr_bgcolor = cell->bgcolor;
					if( color_mode == CM_16 ) {
						fprintf(fp,"\x1b[%d;%dm",fgcolors[scr_fgcolor],bgcolors[scr_bgcolor]);
					} else if( color_mode == CM_256 ) {
						fprintf(fp,"\x1b[38;5;%dm",scr_fgcolor);
						fprintf(fp,"\x1b[48;5;%dm",scr_bgcolor);
					} else if( color_mode == CM_TRUE ) {
						fprintf(fp,"\x1b[38;2;%d;%d;%dm",(scr_fgcolor>>16)&0xFF,(scr_fgcolor>>8)&0xFF,scr_fgcolor&0xFF);
						fprintf(fp,"\x1b[48;2;%d;%d;%dm",(scr_bgcolor>>16)&0xFF,(scr_bgcolor>>8)&0xFF,scr_bgcolor&0xFF);
					}
					scr_reverse = cell->reverse;
					if( scr_reverse ) { fprintf(fp,"\x1b[7m"); }
					scr_blink = cell->blink;
					if(scr_blink ) { fprintf(fp,"\x1b[5m"); }
					scr_bold = cell->bold;
					if(scr_bold ) { fprintf(fp,"\x1b[1m"); }
					scr_underline = cell->underline;
					if( scr_underline ) { fprintf(fp,"\x1b[4m"); }
				}
				if( cell->character == 0 ) {
					c = " ";
				} else {
					c = utf8_encode(0,cell->character);
				}
				fprintf(fp,"%s",c);
			}
		}
		if( ! move_cursor ) {
			if( ! reset ) {
				fprintf(fp,"\x1b[0m");
				reset = 1;
			}
			fprintf(fp,"\r\n");
		}
	}
	if( ! reset ) {
		fprintf(fp,"\x1b[0m");
	}
	fclose(fp);
}

void render() {
	size_t canvas_x;
	size_t canvas_y;
	size_t term_x;
	size_t term_y;
	char* c;
	size_t i;
	cell_t *cell;
	uint32_t scr_fgcolor;
	uint32_t scr_bgcolor;
	uint8_t scr_reverse;
	uint8_t scr_blink;
	uint8_t scr_bold;
	uint8_t scr_underline;
	uint8_t scr_line;
	uint8_t reset;
	uint8_t move_cursor = 0;
	
	printf("\x1b[2J\x1b[H\x1b[0m");
	reset = 1;
	for( canvas_y=scroll_y, term_y=0; canvas_y<canvas_height && term_y<win_height-2; canvas_y++, term_y++ ) {
		for( canvas_x=scroll_x, term_x=0; canvas_x<canvas_width && term_x<win_width; canvas_x++, term_x++ ) {
			cell = &(canvas[canvas_y*canvas_width+canvas_x]);
			if( !export_spaces && cell->character == 0 ) {
				if( ! reset ) {
					printf("\x1b[0m");
					reset = 1;
				}
				move_cursor = 1;
			} else {
				if( reset ||
						scr_fgcolor != cell->fgcolor ||
						scr_bgcolor != cell->bgcolor ||
						scr_reverse != cell->reverse ||
						scr_blink != cell->blink ||
						scr_bold != cell->bold ||
						scr_underline != cell->underline ) {
					if( ! reset ) {
						printf("\x1b[0m");
					}
					reset = 0;
					if( move_cursor ) {
						printf("\x1b[%ld;%ldH",canvas_y-scroll_y+1, canvas_x-scroll_x+1);
						move_cursor = 0;
					}
					scr_fgcolor = cell->fgcolor;
					scr_bgcolor = cell->bgcolor;
					if( color_mode == CM_16 ) {
						printf("\x1b[%d;%dm",fgcolors[scr_fgcolor],bgcolors[scr_bgcolor]);
					} else if( color_mode == CM_256 ) {
						printf("\x1b[38;5;%dm",scr_fgcolor);
						printf("\x1b[48;5;%dm",scr_bgcolor);
					} else if( color_mode == CM_TRUE ) {
						printf("\x1b[38;2;%d;%d;%dm",(scr_fgcolor>>16)&0xFF,(scr_fgcolor>>8)&0xFF,scr_fgcolor&0xFF);
						printf("\x1b[48;2;%d;%d;%dm",(scr_bgcolor>>16)&0xFF,(scr_bgcolor>>8)&0xFF,scr_bgcolor&0xFF);
					}
					scr_reverse = cell->reverse;
					if( scr_reverse ) { printf("\x1b[7m"); }
					scr_blink = cell->blink;
					if(scr_blink ) { printf("\x1b[5m"); }
					scr_bold = cell->bold;
					if(scr_bold ) { printf("\x1b[1m"); }
					scr_underline = cell->underline;
					if( scr_underline ) { printf("\x1b[4m"); }
				}
				if( cell->character == 0 ) {
					c = " ";
				} else {
					c = utf8_encode(0,cell->character);
				}
				printf("%s",c);
			}
		}
		if( ! move_cursor ) {
			if( ! reset ) {
				printf("\x1b[0m");
				reset = 1;
			}
			printf("\r\n");
		}
	}
	if( ! reset ) {
		printf("\x1b[0m");
		reset = 1;
	}
	
	//Remember the attributes of the cell currently under the cursor
	cell = &(canvas[cursor_y*canvas_width+cursor_x]);
	scr_fgcolor = cell->fgcolor;
	scr_bgcolor = cell->bgcolor;
	scr_reverse = cell->reverse;
	scr_blink = cell->blink;
	scr_bold = cell->bold;
	scr_underline = cell->underline;
	scr_line = cell->line;
	
	#ifdef DEBUG
	printf("\x1b[%ld;0H",win_height-2);
	printf("In");
		for( i=0; i < lastinput_len; i++ ) {
		printf("%02X",lastinput[i]);
	}
	#endif
	
	//Move cursor to bottom of the terminal
	printf("\x1b[%ld;0H",win_height-1);
	if( esc ) {
		printf("\x1b[7mEsc\x1b[0m");
	}
	else {
		printf("Esc");
	}
	
	if( save_input ) {
		printf(" Save:%s",save_path);
	} else if( export_input ) {
		printf(" Export:%s",export_path);
	} else if( fgcolor_input ) {
		printf(" FG ");
		if( fgcolor_input == 1 ) { 
			printf("\x1b[7mR:\x1b[0m"); 
		} else {
			printf("R:");
		}
		printf("%d ",(fgcolor>>16)&0XFF);
		if( fgcolor_input == 2 ) {
			printf("\x1b[7mG:\x1b[0m"); 
		} else {
			printf("G:");
		}
		printf("%d ",(fgcolor>>8)&0XFF);
		if( fgcolor_input == 3 ) {
			printf("\x1b[7mB:\x1b[0m"); 
		} else {
			printf("B:");
		}
		printf("%d ",fgcolor&0XFF);
	} else if( bgcolor_input ) {
		printf(" BG ");
		if( bgcolor_input == 1 ) { 
			printf("\x1b[7mR:\x1b[0m"); 
		} else {
			printf("R:");
		}
		printf("%d ",(bgcolor>>16)&0XFF);
		if( bgcolor_input == 2 ) {
			printf("\x1b[7mG:\x1b[0m"); 
		} else {
			printf("G:");
		}
		printf("%d ",(bgcolor>>8)&0XFF);
		if( bgcolor_input == 3 ) {
			printf("\x1b[7mB:\x1b[0m"); 
		} else {
			printf("B:");
		}
		printf("%d ",bgcolor&0XFF);
	} else if( codepage_input ) {
		printf(" Codepage:%X",codepage);
	} else {
		printf(" +/-");
		printf(" 1 2 3 4 5 6 7 8  ");

		//Insert
		if( insert ) { printf("\x1b[7m"); }
		printf("\x1b[4mI\x1b[24mns\x1b[0m ");
		//Underline
		if( underline ) { printf("\x1b[7m"); }
		if( scr_underline ) { printf("\x1b[1m"); }
		printf("\x1b[4mU\x1b[24mndrln\x1b[0m ");
		//Blink
		if( blink ) { printf("\x1b[7m"); }
		if( scr_blink ) { printf("\x1b[1m"); }
		printf("Blin\x1b[4mk\x1b[0m ");
		//Bold
		if( bold ) { printf("\x1b[7m"); }
		if( scr_bold ) { printf("\x1b[1m"); }
		printf("Bol\x1b[4md\x1b[0m ");
		//Reverse
		if( reverse ) { printf("\x1b[7m"); }
		if( scr_reverse ) { printf("\x1b[1m"); }
		printf("\x1b[4mR\x1b[24mev\x1b[0m ");
		// FG/BG
		if( color_mode == CM_16 ) {
			printf("\x1b[0m\x1b[%d;%dm\x1b[4mF\x1b[24mG(%02d)/\x1b[4mB\x1b[24mG(%02d)\x1b[0m ",fgcolors[fgcolor],bgcolors[bgcolor],fgcolor,bgcolor);
		} else if( color_mode == CM_256 ) {
			printf("\x1b[0m\x1b[38;5;%dm\x1b[48;5;%dm\x1b[4mF\x1b[24mG(%02d)/\x1b[4mB\x1b[24mG(%02d)\x1b[0m ",fgcolor,bgcolor,fgcolor,bgcolor);
		} else if( color_mode == CM_TRUE ) {
			printf("\x1b[0m\x1b[38;2;%d;%d;%dm\x1b[48;2;%d;%d;%dm\x1b[4mF\x1b[24mG/\x1b[4mB\x1b[24mG\x1b[0m ",
						 (fgcolor>>16)&0xFF,(fgcolor>>8)&0xFF,fgcolor&0xFF,(bgcolor>>16)&0xFF,(bgcolor>>8)&0xFF,bgcolor&0xFF);
		}
		//Apply
		printf("\x1b[4mA\x1b[0mpply ");
		//Grab
		printf("\x1b[4mG\x1b[0mrab ");
		
		
		//Codpage
		printf("\r\n %06X",codepage&0xFFFFFF);
		for( i=0; i<8; i++ ) {
			if( charset ) {
				printf(" %s",utf8_encode(0,charset[(codepage+i)%charsetlen]));
			}
			else {
				printf(" %s",utf8_encode(0,codepage+i));
			}
		}
		//Quit
		printf("  \x1b[4mQ\x1b[0muit ");
		//Clear
		printf("Cl\x1b[4me\x1b[24mar\x1b[0m ");
		//Save
		printf("\x1b[4mS\x1b[24mave\x1b[0m ");
		//Export
		printf("E\x1b[4mx\x1b[24mport\x1b[0m ");
		//Copy
		if( copy_start_x != -1 && copy_end_x == -1 ) { printf("\x1b[7m"); }
		printf("\x1b[4mC\x1b[24mopy\x1b[0m ");
		//Paste
		if( copy_start_x != -1 && copy_end_x != -1 ) { printf("\x1b[7m"); }
		printf("\x1b[4mP\x1b[24maste\x1b[0m ");
		//Draw Line 
		printf("\x1b[4mL\x1b[24mine\x1b[0m(%d) ",(scr_line+1)%3);
		// Pos
		printf("Pos(%ld,%ld)",cursor_x,cursor_y);
		
		//Position Cursor
		printf("\x1b[%ld;%ldH",cursor_y-scroll_y+1, cursor_x-scroll_x+1);
	}
	fflush(stdout);
}

void termSetup() {
	int flags;
	struct termios tios;
	struct winsize ws;
	TTY tty;
	
	//Use fcntl to make stdin NON-BLOCKING
	if( (flags = fcntl(STDIN_FILENO, F_GETFL, 0)) < 0 ) {
		fprintf(stderr,"Could not get fnctl flags: %s\n",strerror(errno));
		exit(1);
	}
	flags |= O_NONBLOCK;
	if( fcntl(STDIN_FILENO, F_SETFL, flags ) < 0 ) { 
		fprintf(stderr,"Could not set fnctl flags: %s\n",strerror(errno));
		exit(1);
	}
	
	//Use ioctl/TCGETS to disable echo and line buffering (icannon)
	if( GET_TTY(STDIN_FILENO,&tty) < 0 ) {
		fprintf(stderr,"Could not get termios flags: %s\n",strerror(errno));
		exit(1);
	}
	tty.c_lflag &= ~ICANON;
	tty.c_lflag &= ~ECHO;
	if( SET_TTY(STDIN_FILENO,&tty) < 0 ) {
		fprintf(stderr,"Could not set termios flags: %s\n",strerror(errno));
		exit(1);
	}
	
	//Use ioctl/TIOCGWINSZ to get terminal size
	ioctl(STDIN_FILENO,TIOCGWINSZ,&ws);
	win_width = ws.ws_col;
	win_height = ws.ws_row;
}

void termSetupReset() {
	int flags;
	TTY tty;
	
	printf("\x1b[2J\x1b[0;0H\x1b[0m");
	
	//Use fcntl to make stdin BLOCKING
	if( (flags = fcntl(STDIN_FILENO, F_GETFL, 0)) < 0 ) {
		fprintf(stderr,"Could not get fnctl flags: %s\n",strerror(errno));
		exit(1);
	}
	flags &= ~O_NONBLOCK;
	if( fcntl(STDIN_FILENO, F_SETFL, flags ) < 0 ) { 
		fprintf(stderr,"Could not set fnctl flags: %s\n",strerror(errno));
		exit(1);
	}
	
	//Use ioctl/TCGETS to enable echo and line buffering (icannon)
	if( GET_TTY(STDIN_FILENO,&tty) < 0 ) {
		fprintf(stderr,"Could not get termios flags: %s\n",strerror(errno));
		exit(1);
	}
	tty.c_lflag |= ICANON;
	tty.c_lflag |= ECHO;
	if( SET_TTY(STDIN_FILENO,&tty) < 0 ) {
		fprintf(stderr,"Could not set termios flags: %s\n",strerror(errno));
		exit(1);
	}
}

void usage(char* cmd) {
	printf("Usage:\n");
	printf("%s [-h] [-s width height] [-m 16|256|true] [-c codepage(hex)]\n",cmd);
	printf("      [-dos | -pet | -trs] [-es] [-nec] binpath\n");
	printf("\n");
	printf("  -h  : Print usage message\n");
	printf("  -m  : Select color mode -\n");
	printf("           16 color standard palette\n");
	printf("          256 color standard palette\n");
	printf("         true color - 24 bit\n");
	printf("  -dos: Use DOS character set (codepage 437)\n");
	printf("  -pet: Use PETSCI character set\n");
	printf("  -trs: Use TRS-80 character set\n");
	printf("        Experts are still in UTF-8\n");
	printf("  -c  : Starting special character code page\n");
	printf("  -es : DISABLE export of blank characters as spaces\n");
	printf("  -ec : DISABLE export start with terminal clear\n");
	printf("\n");
	exit(1);
}

int running = 1;
void sigint_handler(int signalId) {
	running = 0;
}

int main(int argc, char** argv) {
	size_t i;
	uint8_t input[8];
	ssize_t inputlen;
	uint8_t size_specified = 0;
	
	save_path[0] = 0;
	export_path[0] = 0;
	
	i=1;
	while( i < argc ) {
		if( strcmp("-h",argv[i]) == 0 ) {
			usage(argv[0]);
		}
		else if( strcmp("-s",argv[i]) == 0 ) {
			if( i >= argc-2 ) {
				usage(argv[0]);
			}
			canvas_width = strtoul(argv[++i],0,10);
			canvas_height = strtoul(argv[++i],0,10);
			if( canvas_width == 0 || canvas_height == 0 ) {
				usage(argv[0]);
			}
			size_specified = 1;
		}
		else if( strcmp("-c",argv[i]) == 0 ) {
			if( i >= argc-1 ) {
				usage(argv[0]);
			}
			codepage = strtoul(argv[++i],0,16);
		}
		else if( strcmp("-m",argv[i]) == 0 ) {
			if( i >= argc-1 ) {
				usage(argv[0]);
			}
			i++;
			if( strcmp("16",argv[i]) ==  0 ) {
				color_mode = CM_16;
			}
			else if( strcmp("256",argv[i]) == 0 ) {
				color_mode = CM_256;
			}
			else if( strcasecmp("true",argv[i]) == 0 ) {
				color_mode = CM_TRUE;
			}
			else {
				usage(argv[0]);
			}
		}
		else if( strcmp("-dos",argv[i]) == 0 ) {
			if( charset ) {
				usage(argv[0]);
			}
			charset = cp437;
			charsetlen = 256;
		}
		else if( strcmp("-pet",argv[i]) == 0 ) {
			if( charset ) {
				usage(argv[0]);
			}
			charset = petscii;
			charsetlen = 512;
		}
		else if( strcmp("-trs",argv[i]) == 0 ) {
			if( charset ) {
				usage(argv[0]);
			}
			charset = trs80;
			charsetlen = 192;
		}
		else if( strcmp("-es",argv[i]) == 0 ) {
			export_spaces = 0;
		}
		else if( strcmp("-ec",argv[i]) == 0 ) {
			export_clear = 0;
		}
		else {
			if( strlen(save_path) ) {
				usage(argv[0]);
			}
			strncpy(save_path,argv[i],255);
			save_path_len = strlen(save_path);
			save_path[save_path_len] = 0;
		}
		i++;
	}
	
	if( charset ) {
		codepage = codepage%charsetlen;
	}
	
	init_canvas();
	
	if( save_path_len != 0 ) {
		load(save_path,size_specified);
	}
	
	termSetup();
	
	signal(SIGINT, sigint_handler);
	
	render();
	fflush(stdout);
	while( running ) {
		memset(input,0,sizeof(input));
		inputlen = read(STDIN_FILENO,&input,sizeof(input));
		#ifdef DEBUG
		if( inputlen > 0 ) {
			memcpy(lastinput,input,sizeof(input));
			lastinput_len = inputlen;
		}
		#endif
		if( inputlen < 0 ) {
			if( errno != EAGAIN ) {
				running = 0;
			}
			continue;
		}
		//Save File Input
		if( save_input || export_input ) {
			if( input[0] == 0x1b ) {
				esc = 0;
				save_input = 0;
				export_input = 0;
			}
			else if( input[0] == 0x0A ) {
				if( save_input ) {
					save();
					save_input = 0;
				} else if( export_input ) {
					export();
					export_input = 0;
				}
			}
			else {
				uint8_t *len;
				char* path;
				if( save_input ) {
					path = save_path;
					len = &save_path_len;
				} else { //if( export_input )
					path = export_path;
					len = &export_path_len;
				}
				if( input[0] > 0x1f && input[0] < 0x7f ) {
					if( *len < 255 ) {
						path[*len] = input[0];
						*len = *len + 1;
						path[*len] = 0;
					}
				}
				else if( input[0] == 0x7F ) {
					if( *len ) {
						*len = *len - 1;
						path[*len] = 0;
					}
				}
			}
		}
		//True Color Input
		else if( fgcolor_input || bgcolor_input ) {
			uint8_t* color_input;
			uint32_t* color;
			uint8_t start_input;
			uint32_t value;
			if( fgcolor_input ) {
				color_input = &fgcolor_input;
				color = &fgcolor;
			} else {
				color_input = &bgcolor_input;
				color = &bgcolor;
			}
			start_input = *color_input;
			if( start_input == 1 ) {
				value = (*color>>16)&0xFF;
			}
			else if( start_input == 2 ) {
				value = (*color>>8)&0xFF;
			} else {
				value = (*color)&0xFF;
			}
			if( inputlen == 1 ) {
				if( input[0] >= 0x30 && input[0] <= 0x39 ) {
					value = ((value * 10) + (input[0]-0x30))&0xFF;
				}
				else if( input[0] == 0x0A ) {
					*color_input = *color_input+1;
					if( *color_input >= 4 ) {
						*color_input = 0;
					}
				}
				else if( input[0] == 0x7F ) {
					if( value ) { 
						value = value / 10; 
					}
				}
			}
			if( start_input == 1 ) {
				*color = (*color&0x0000FFFF) | (value<<16);
			}
			else if( start_input == 2 ) {
				*color = (*color&0x00FF00FF) | (value<<8);
			} else {
				*color = (*color&0x00FFFF00) | (value);
			}
		}
		//Specify Codepage Manually
		else if( codepage_input  ) {
			if( inputlen == 1 ) {
				if( input[0] >= 0x30 && input[0] <= 0x39 ) {
					codepage = ((codepage * 16) + (input[0]-0x30));
				}
				else if( input[0] >= 0x41 && input[0] <= 0x46 ) {
					codepage = ((codepage * 16 ) + (10 + input[0]-0x41));
				}
				else if( input[0] >= 0x61 && input[0] <= 0x66 ) {
					codepage = ((codepage * 16 ) + (10 + input[0]-0x61));
				}
				else if( input[0] == 0x0A ) {
					if( charset ) {
						codepage = codepage % charsetlen;
					}
					codepage_input = 0;
				}
				else if( input[0] == 0x7F ) {
					if( codepage ) { 
						codepage = codepage / 16; 
					}
				}
			}
		}
		//Regular Input
		else if( inputlen == 1 ) {
			if( input[0] == 0x0A ) {
				move_cursor(DIRNL);
			}
			else if( input[0] == 0x1b ) {
				esc = 1 - esc;
			}
			else if( input[0] > 0x1f && input[0] < 0x7f ) {
				if( esc ) {
					if( input[0] == 'q' || input[0] == 'Q' ) {
						running = 0;
					}
					else if( input[0] == 'r' || input[0] == 'R' ) {
					reverse = 1-reverse;
					}
					else if( input[0] == 'k' || input[0] == 'K' ) {
						blink = 1 - blink;
					}
					else if( input[0] == 'd' || input[0] == 'D' ) {
						bold = 1 - bold;
					}
					else if( input[0] == 'u' || input[0] == 'U' ) {
						underline = 1 - underline;
					}
					else if( input[0] == 'i' || input[0] == 'I' ) {
						insert = 1 - insert;
					}
					else if( input[0] == 'e' || input[0] == 'E' ) {
						clear_canvas();
					}
					else if( input[0] == 'l' || input[0] == 'L' ) {
						draw_line();
					}
					else if( input[0] == '-' || input[0] == '_' ) {
						codepage = codepage-8;
						if( charset ) {
							codepage = codepage % charsetlen;
						}
					}
					else if( input[0] == '/' || input[0] == '?' ) {
						codepage_input = 1;
					}
					else if( input[0] == '=' || input[0] == '+' ) {
						codepage = codepage+8;
						if( charset ) {
							codepage = codepage % charsetlen;
						}
					}
					else if( input[0] == 'f' ) { //Foreground color up
						if( color_mode == CM_16 ) { fgcolor = (fgcolor+1)%16; }
						else if( color_mode == CM_256 ) { fgcolor = (fgcolor+1)%256; }
						else if( color_mode == CM_TRUE ) { fgcolor_input = 1; }
					}
					else if( input[0] == 'F' ) { //Foreground color down
						if( color_mode == CM_16 ) { fgcolor = (fgcolor+15)%16; }
						else if( color_mode == CM_256 ) { fgcolor = (fgcolor+255)%256; }
						else if( color_mode == CM_TRUE ) { fgcolor_input = 1; }
					}
					else if( input[0] == 'b' ) { //Background color up
						if( color_mode == CM_16 ) { bgcolor = (bgcolor+1)%16; }
						else if( color_mode == CM_256 ) { bgcolor = (bgcolor+1)%256; }
						else if( color_mode == CM_TRUE ) { bgcolor_input = 1; }
					}
					else if( input[0] == 'B' ) { //Background color down
						if( color_mode == CM_16 ) { bgcolor = (bgcolor+15)%16; }
						else if( color_mode == CM_256 ) { bgcolor = (bgcolor+255)%256; }
						else if( color_mode == CM_TRUE ) { bgcolor_input = 1; }
					}
					else if( input[0] == 's' || input[0] == 'S' ) {
						save_input = 1;
					}
					else if( input[0] == 'x' || input[0] == 'X' ) {
						export_input = 1;
					}
					else if( input[0] == 'a' || input[0] == 'A' ) {
						apply_character_attributes();
					}
					else if( input[0] == 'g' || input[0] == 'G' ) {
						grab_character_attributes();
					}
					else if( input[0] == 'c' || input[0] == 'C' ) {
						if( copy_start_x == -1 || copy_end_x != -1 ) {
							copy_start_x = cursor_x;
							copy_start_y = cursor_y;
							copy_end_x = -1;
							copy_end_y = -1;
						}
						else {
							if( copy_start_x > cursor_x ) {
								copy_end_x = copy_start_x;
								copy_start_x = cursor_x;
							}
							else {
								copy_end_x = cursor_x;
							}
							if( copy_start_y > cursor_y ) {
								copy_end_y = copy_start_y;
								copy_start_y = cursor_y;
							}
							else {
								copy_end_y = cursor_y;
							}
						}
					}
					else if( input[0] == 'p' || input[0] == 'P' ) {
						paste_block();
					}
					else if( input[0] > 0x30 && input[0] < 0x39 ) {
						if( charset ) {
							insert_cell(charset[(codepage+(input[0]-0x31))%charsetlen]);
						}
						else {
							insert_cell(codepage+(input[0]-0x31));
						}
					}
				}
				else {
					insert_cell(input[0]&0xFF);
				}
			}
			else if( input[0] == 0x7F ) { //Backspace
				if( move_cursor(DIRLT) ) {
					delete_cell();
				}
			}
		}
		else if( inputlen == 3 ) {
			if( input[0] == 0x1B && (input[1] == 0x5B || input[1] == 0x4F) ) {
				if( input[2] >= DIRUP && input[2] <= DIRLT ) { //Arrow Keys
					move_cursor(input[2]);
				}
				else if( input[2] == 0x46 ) { //End
					while( cursor_x < canvas_width-1 ) {
						move_cursor(DIRRT);
					}
				}
				else if( input[2] == 0x48 ) { //Home
					while( cursor_x > 0 ) {
						move_cursor(DIRLT);
					}
				}
			}
			else if( input[0] == 0x1B && input[2] == 0xB4 ) { //F1-F4
				if( input[2] >= 0x50 && input[2] <= 0x53 ) {
					if( charset ) {
						insert_cell(charset[(codepage+(input[2]-0x50))%charsetlen]);
					}
					else {
						insert_cell(codepage+(input[2]-0x50));
					}
				}
			}
		}
		else if( inputlen == 4 ) {
			if( input[0] == 0x1B && input[1] == 0x5B && input[3] == 0x7E ) {
				if( input[2] == 0x32 ) { //Insert
					insert = 1- insert;
				}
				else if( input[2] == 0x33 ) { //Delete
					delete_cell();
				}
				else if( input[2] == 0x35 ) { //Page Up
					for( i=0; i<win_height-2; i++ ) {
						if( ! move_cursor(DIRUP) ) { break; }
					}
				}
				else if( input[2] == 0x36 ) { //Page Down
					for( i=0; i<win_height-2; i++ ) {
						if( ! move_cursor(DIRDN) ) { break; }
					}
				}
			}
		}
		else if( inputlen == 5 ) {
			if( input[0] == 0x1B && input[1] == 0x5B && input[2] == 0x31 && input[4] == 0x7e &&
					input[3] >= 0x35 && input[3] <= 0x39 ) { //F5-F8
				input[3] = input[3] - 0x35;
				if( ! input[3] ) { //F5 is one less than expected
					input[3]++;
				}
				if( charset ) {
					insert_cell(charset[(codepage+(input[3]+3))%charsetlen]);
				}
				else {
					insert_cell(codepage+(input[3]+3));
				}
			}
		}
		render();
	}
	
	termSetupReset();
	return 0;
}