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
#include <sys/ioctl.h>
#include <time.h>
#include <errno.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

#include "term_encode.h"

#ifdef USE_PORTAUDIO
#include<portaudio.h>

typedef struct {
	PaStream* stream;
	uint8_t *samples;
	PaSampleFormat format;
	size_t channels;
	size_t sample_len;
	size_t frame_len;
	size_t len;
	size_t write_off;
	size_t read_off;
} paBuffer_t;

int paCallback(
		const void *input, void *output,
		unsigned long frameCount,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void *userData ) {
	size_t copyLen1; //from read_off to write_off or end of samples
	size_t copyLen2; //from beginning of samples to write_off
	size_t zeroFill; //fill what's left over
	size_t outLen; //size of output in bytes
	paBuffer_t *buffer = (paBuffer_t*)userData;
	uint8_t *out = (uint8_t*)output;

	outLen = buffer->frame_len*frameCount;
	
	if( buffer->read_off == buffer->write_off ) {
		copyLen1 = 0;
		copyLen2 = 0;
	}
	else if( buffer->write_off > buffer->read_off ) {
		copyLen1 = buffer->write_off-buffer->read_off;
		copyLen2 = 0;
		if( copyLen1 > outLen ) {
			copyLen1 = outLen;
		}
	} 
	else {
		copyLen1 = buffer->len - buffer->read_off;
		copyLen2 = 0;
		if ( copyLen1 > outLen ) {
			copyLen1 = outLen;
		}
		else if( copyLen1 < outLen ) {
			copyLen2 = buffer->write_off;
			if( copyLen1 + copyLen2 > outLen ) {
				copyLen2 = outLen - copyLen1;
			}
		}
	}
	zeroFill = outLen - (copyLen1+copyLen2);
	memcpy(out,buffer->samples+buffer->read_off,copyLen1);
	memcpy(out+copyLen1,buffer->samples,copyLen2);
	memset(out+copyLen1+copyLen2,0,zeroFill);
	
	#ifdef DEBUG
	if( zeroFill ) { 
		fprintf(stderr,"Audio Underflow: readOff(%ld) writeOff(%ld) len(%ld) copy1(%ld) copy2(%ld) outLen(%ld)\n",buffer->read_off,buffer->write_off,buffer->len,copyLen1,copyLen2,outLen);
	}
	#endif
	
	buffer->read_off = (buffer->read_off + copyLen1 + copyLen2) % buffer->len;
	return paContinue;
}

void paWriteBuffer(paBuffer_t* buffer, uint8_t **frame_data, size_t frame_count) {
	size_t ch;
	size_t i;
	size_t next_off;
	
	#ifdef DEBUG
		fprintf(stderr,"paWritebuffer: buffer(%08lX) frame_count(%ld)\n",(size_t)buffer,frame_count);
	#endif
	
	for( i=0; i<frame_count; i++ ) {
		next_off = (buffer->write_off + buffer->frame_len) % buffer->len;
		if( next_off == buffer->read_off ) {
			break;
		}
		for( ch=0; ch<buffer->channels; ch++ ) {
			memcpy(buffer->samples+buffer->write_off, frame_data[ch]+(buffer->sample_len*i), buffer->sample_len);
			buffer->write_off = (buffer->write_off + buffer->sample_len) % buffer->len;
		}
	}
	
	if( ! Pa_IsStreamActive(buffer->stream) && buffer->write_off > buffer->len/2 ) {
		#ifdef DEBUG
		printf("Starting audio stream\n");
		#endif
		Pa_StartStream(buffer->stream);
	}
}
#endif


typedef struct {
	uint8_t eof;
	size_t id;
	struct timespec start_time;
	struct timespec end_time;
	char text[1024];
	size_t textlen;
	size_t linecount;
} subtitle_t;

void double2timespec( struct timespec *dst, double src ) {
	dst->tv_sec = (time_t)src;
	dst->tv_nsec = (time_t)( (src-(double)dst->tv_sec) * (double)1000000000.0 );
}

void timespec2double( double *dst, struct timespec *src ) {
	*dst = (double)src->tv_sec + ((double)src->tv_nsec / (double)1000000000.0);
}

int timediff( struct timespec* dst, struct timespec *op1, struct timespec *op2 ) {
	if( op2->tv_sec > op1->tv_sec ) {
		if( dst ) {
			dst->tv_sec  = 0;
			dst->tv_nsec = 0;
		}
		return -1;
	}
	else if( op2->tv_sec == op1->tv_sec ) {
		if( op2->tv_nsec > op1->tv_nsec ) {
			if( dst ) {
				dst->tv_sec  = 0;
				dst->tv_nsec = 0;
				return -1;
			}
		}
		else if( op2->tv_nsec == op1->tv_nsec ) {
			if( dst ) {
				dst->tv_sec  = 0;
				dst->tv_nsec = 0;
				return 0;
			}
		}
		else { //op2->tv_sec == opt2->tv_sec && op2->tv_nsec < op1->tv_nsec
			if( dst ) {
				dst->tv_sec  = 0;
				dst->tv_nsec = op1->tv_nsec - op2->tv_nsec;
			}
			return 1;
		}
	}
	else { //op2->tv_sec < op1->tv_sec
		if( op2->tv_nsec > op1->tv_nsec ) {
			//Subtract with Borrow
			if( dst ) {
				dst->tv_sec  = op1->tv_sec-1-op2->tv_sec;
				dst->tv_nsec = (1000000000UL + op1->tv_nsec) - op2->tv_nsec;
			}
		}
		else {
			//Subtrace without Borrow
			if( dst ) {
				dst->tv_sec  = op1->tv_sec  - op2->tv_sec;
				dst->tv_nsec = op1->tv_nsec - op2->tv_nsec;
			}
		}
		return 1;
	}
	//Should never reach here
	if( dst ) {
		dst->tv_sec = 0;
		dst->tv_nsec = 0;
	}
	return 0;
}

double parse_time_code(char* s, char** endptr) {
	double units[4] = {0,0,0,0};
	double den;
	double sec;
	char* end;
	
	units[1] = strtoul(s,&s,10);
	if( *s == ':' ) {
		units[2] = units[1];
		s++;
		units[1] = strtoul(s,&s,10);
		if( *s == ':' ) {
			units[3] = units[2];
			units[2] = units[1];
			s++;
			units[1] = strtoul(s,&s,10);
		}
	}
	if( *s == '.' || *s == ',' ) {
		s++;
		units[0] = strtoul(s,&end,10);
		if( end == s ) {
			den = 1;
		} else if( den == 0 ) {
			den = pow(10.0,strlen(s));
		} else {
			den = pow(10.0,(end-s));
		}
		if( endptr ) {
			*endptr = end;
		}
	}
	else {
		if( endptr ) {
			*endptr = s;
		}
	}
	sec = (units[3] * 3600) + (units[2] * 60) + units[1] + (units[0]/den);
	return sec;
}

int nextSubtitle( subtitle_t* subtitle, FILE* srtfile) {
	char c = 0;
	char line[256];
	char* pline;
	size_t linelen;
	double start_time;
	double end_time;
	subtitle->textlen = 0;
	subtitle->linecount = 0;
	
 linelen = 0;
	while( linelen < 255 ) {
		if( ! fread(&c,1,1,srtfile) ) {
			goto nextSubtitleError;
		}
		if( c == '\n' ) {
			line[linelen] = 0;
			break;
		}
		else if( c != '\r') {
			line[linelen++] = c;
		}
	}
	errno = 0;
	subtitle->id = strtoul(line,0,10);
	if( errno ) {
		goto nextSubtitleError;
	}
	
	linelen = 0;
	while( linelen < 255 ) {
		if( ! fread(&c,1,1,srtfile) ) {
			goto nextSubtitleError;
		}
		if( c == '\n' ) {
			line[linelen] = 0;
			break;
		}
		else if( c != '\r') {
			line[linelen++] = c;
		}
	}
	start_time = parse_time_code(line,&pline);
	double2timespec(&(subtitle->start_time),start_time);
	while( *pline == ' ' ) { pline++; }
	if( strncmp(pline,"-->",3) != 0 ) {
		goto nextSubtitleError;
	}
	pline = pline + 3;
	end_time = parse_time_code(pline,0);
	double2timespec(&(subtitle->end_time),end_time);
	
	while( subtitle->textlen < 1023 ) {
		if( ! fread(&c,1,1,srtfile) ) {
			break;
		}
		if( c == '\n' ) {
			if( subtitle->text[subtitle->textlen-1] == '\n' ) {
				break;
			}
			subtitle->linecount++;
		}
		if( c != '\r' ) {
			subtitle->text[subtitle->textlen++] = c;
		}
	}
	subtitle->text[subtitle->textlen] = 0;
	#ifdef DEBUG
	fprintf(stderr,"Next Subtitle id(%ld) start(%f) end(%f) linecount(%ld) text(%s)\n",subtitle->id,start_time,end_time,subtitle->linecount, subtitle->text);
	#endif
	return 0;
	
	nextSubtitleError:
	subtitle->eof = 1;
	subtitle->textlen = 0;
	subtitle->text[0] = 0;
	subtitle->start_time.tv_sec = 0;
	subtitle->start_time.tv_nsec = 0;
	subtitle->end_time.tv_sec = 0;
	subtitle->end_time.tv_nsec = 0;
	return 1;
}

void savePPM(size_t frame_number, uint8_t* pixels, size_t width, size_t height) {
	char path[256];
	FILE *fp;
	int  y;
	
	snprintf(path,255,"frame%08lu.ppm",frame_number);
	path[255] = 0;
	fp=fopen(path, "wb");
	if( fp == 0 ) {
		fprintf(stderr,"Failed to open %s to save frame.\n",path);
		return;
	}

	fprintf(fp, "P6\n%lu %lu\n255\n", width, height);
	fwrite(pixels,3,width*height,fp);
	fclose(fp);
}

double findFramePeriod(AVStream* stream, AVCodecContext *codecCtx, double default_framerate) {
		////libavcodec display timestamps are garbage.  They forward and backward.
		////Additionally timeing information is filled, seemingly at random. 
		////So we'll build our own frame time stamps using whatever we can find.
		double frame_period_sec = 2;
		if( stream->r_frame_rate.num && stream->r_frame_rate.den ) {
			//If the framerate is valid use it.
			frame_period_sec = ((double)stream->r_frame_rate.den / (double)stream->r_frame_rate.num);
			#ifdef DEBUG
			fprintf(stderr,"frame_period_sec(%f) - Based on AVStream.r_frame_rate(%d/%d)\n",frame_period_sec,
					stream->r_frame_rate.num,
					stream->r_frame_rate.den);
			#endif
		} 
		if( (frame_period_sec > 1 || frame_period_sec < 0.01 ) && 
				stream->time_base.num && stream->time_base.den ) {
			//Else/If the time_base is valid, use it.
			frame_period_sec = ((double)stream->time_base.num / (double)stream->time_base.den);
			#ifdef DEBUG
			fprintf(stderr,"frame_period_sec(%f) - Based on AVStream.time_base(%d/%d)\n",frame_period_sec,
						 stream->time_base.num,
						 stream->time_base.den);
			#endif
		}
		if( (frame_period_sec > 1 || frame_period_sec < 0.01 ) && 
				codecCtx->time_base.num && codecCtx->time_base.den ) {
			//Else/If the time_base in the video codec is valid, use it
			frame_period_sec = ((double)codecCtx->time_base.num / (double)codecCtx->time_base.den) * (double)codecCtx->ticks_per_frame;
			#ifdef DEBUG
			fprintf(stderr,"frame_period_sec(%f) - Based on AVCodecContext.time_base(%d/%d) and ticks_per_frame(%d)\n",frame_period_sec,
					codecCtx->time_base.num,
					codecCtx->time_base.den,
					codecCtx->ticks_per_frame);
			#endif
		} 
		if( frame_period_sec > 1 || frame_period_sec < 0.01 ) {
			//Just assume 23.976024 frames per second, and hope for the best
			frame_period_sec = ((double)1.0/(double)default_framerate);
			#ifdef DEBUG
			fprintf(stderr,"frame_period_sec(%f) - Based on assume 23.976024 frame rate\n",frame_period_sec);
			#endif
		}
		return frame_period_sec;
}

static void usage(char* cmd) {
	fprintf(stderr,"Usage:\n");
	fprintf(stderr,"%s [-h] [-v] ",cmd);
	#ifdef USE_PORTAUDIO
	fprintf(stderr,"[-m] ");
	#endif
	fprintf(stderr,"[-srt subfile] [-seek 0:00:00.000]\n");
	fprintf(stderr,"  [-sp 16|256|24 | -p #] [-w #]");
	#ifdef USE_QUANTPNM
	fprintf(stderr," [-dither]");
	#endif //USE_QUANTPNM
	fprintf(stderr,"\n");
	fprintf(stderr,"  [-crop x y w h] [-edge | -line | -glow | -hi 0xFFGGBB]\n");
	fprintf(stderr,"  renderer vidfile\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"-h     : Print usage message\n");
	fprintf(stderr,"-v     : Print information after the frame\n");
	#ifdef USE_PORTAUDIO
	fprintf(stderr,"-m     : Mute audio\n");
	#endif
	fprintf(stderr,"-srt   : Show subtitles from specified .srt file\n");
	fprintf(stderr,"-seek  : Seek to specifed time code\n");
	fprintf(stderr,"-sp    : Use standard 16 or 256 color palette (or 24 shade grey scale)\n");
	fprintf(stderr,"-p     : Use a true color palette of # colors (<= 256)\n");
	fprintf(stderr,"-w     : Set the character width (terminal width used by default)\n");
	#ifdef USE_QUANTPNM
	fprintf(stderr,"-dither: Use palette quantizer with dither\n");
	#endif //USE_QUANTPNM
	fprintf(stderr,"-crop  : Crop the video before processing\n");
	fprintf(stderr,"-edge  : Render edge detection (scaled) using specified color\n");
	fprintf(stderr,"-line  : Render edges as solid lines using specified color\n");
	fprintf(stderr,"-glow  : Mix edge detection (scaled) with image\n");
	fprintf(stderr,"-hi    : Mix edge line with images\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"-= Renderers =-\n");
	#ifdef USE_LIBSIXEL
	fprintf(stderr,"-sixel   : Sixel (not well supported)\n");
	#endif //USE_LIBSIXEL
	fprintf(stderr,"-frames #: Dump frames as PPM files\n");
	fprintf(stderr,"-half    : ANSI Half-Height\n");
	fprintf(stderr,"-double  : ANSI Double-Line\n");
	fprintf(stderr,"-qchar   : ANSI Quarter-Character\n");
	fprintf(stderr,"-six     : ANSI Sextant-Character\n");
	#ifdef USE_AALIB
	fprintf(stderr,"-aadl    : ASCII Art Double-Line\n");
	fprintf(stderr,"-aadlext : ASCII Art Double-Line with extended characaters\n");
	fprintf(stderr,"-aafg    : ASCII Art Dobule-Line with Foreground Color\n");
	fprintf(stderr,"-aafgext : ASCII Art Dobule-Line with Foreground Color and\n");
	fprintf(stderr,"           extended characters\n");
	fprintf(stderr,"-aabg    : ASCII Art Double-Line with Background Color\n");
	fprintf(stderr,"-aabgext : ASCII Art Double-Line with Background Color and\n");
	fprintf(stderr,"           extended characters\n");
	#endif //USE_AALIB
	#ifdef USE_LIBCACA
	fprintf(stderr,"-caca    : CACA (Color ASCII Art)\n");
	fprintf(stderr,"-cacablk : CACA with block characters\n");
	#endif
	fprintf(stderr,"\n");
	exit(1);
}
	
int main(int argc, char** argv) {
	size_t i;
	term_encode_t enc;
	uint8_t dump_frames = 0;
	char* vidpath = 0;
	struct winsize ws;
	size_t win_height;
	size_t scale_width;
	size_t scale_height;
	float ratio;
	AVFormatContext *pFormatCtx = NULL;
	#ifdef USE_PORTAUDIO
	uint8_t mute=0;
	ssize_t audioStream = -1;
	AVCodecParameters *pAudioCodecParam = NULL;
	AVCodecContext  *pAudioCodecCtx = NULL;
	const AVCodec   *pAudioCodec = NULL;
	paBuffer_t paBuffer;
	const PaStreamInfo* paInfo;
	#endif
	ssize_t videoStream = -1;
	AVCodecParameters *pVideoCodecParam = NULL;
	AVCodecContext  *pVideoCodecCtx = NULL;
	const AVCodec   *pVideoCodec = NULL;
	AVFrame         *pFrame = NULL;
	AVFrame         *pFrameRGB = NULL;
	AVPacket        packet;
	int             numBytes;
	uint8_t         *buffer = NULL;
	AVDictionary    *optionsDict = NULL;
	struct SwsContext      *sws_ctx = NULL;
	struct timespec start_time;
	struct timespec current_time;
	struct timespec frame_time;
	struct timespec display_time;
	struct timespec sleep_time;
	double current_time_sec;
	double frame_time_sec;
	double frame_period_sec;
	double display_time_sec;
	double sleep_time_sec;
	double tmp_time_sec;
	double seek_time_sec = 0.0;
	size_t seek_frame_count;
	time_t tmp_time;
	uint8_t verbose = 0;
	uint8_t skip = 0;
	size_t frame_number = 0;
	FILE* srtfile = 0;
	subtitle_t subtitle;
	char *psub;
	size_t linelen;
	size_t lineoff;
	
	term_encode_init(&enc);
	
	i=1;
	while( i < argc ) {
		if( strcmp(argv[i],"-h") == 0 ) {
			usage(argv[0]);
		}
		else if( strcmp(argv[i],"-v") == 0 ) {
			if( verbose ) {
				usage(argv[0]);
			}
			verbose = 1;
		}
		#ifdef USE_PORTAUDIO
		else if( strcmp(argv[i],"-m") == 0 ) {
			if( mute ) {
				usage(argv[0]);
			}
			mute = 1;
		}
		#endif
		else if( strcmp(argv[i],"-srt") == 0 ) {
			if( i >= argc-1 ) {
				usage(argv[0]);
			}
			if( srtfile ) {
				usage(argv[0]);
			}
			srtfile = fopen(argv[++i],"rb");
		}
		else if( strcmp(argv[i],"-seek") == 0 ) {
			if( i >= argc-1 ) {
				usage(argv[0]);
			}
			if( seek_time_sec != 0.0 ) {
				usage(argv[0]);
			}
			seek_time_sec = parse_time_code(argv[++i],0);
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
		else if( strcmp(argv[i],"-w") == 0 ) {
			if( i >= argc-1 ) {
				usage(argv[0]);
			}
			enc.win_width = atoi(argv[++i]);
			if( enc.win_width == 0 ) {
				usage(argv[0]);
			}
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
			if( i >= argc-1 ) {
				usage(argv[0]);
			}
			enc.edge = ENC_EDGE_SCALE;
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
			if( i >= argc-1 ) {
				usage(argv[0]);
			}
			enc.edge = ENC_EDGE_LINE;
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
			if( i >= argc-1 ) {
				usage(argv[0]);
			}
			enc.edge = ENC_EDGE_GLOW;
			errno = 0;
			enc.color_rgb = strtoul(argv[++i],0,16);
			if( errno ) {
				usage(argv[0]);
			}
		}
		else if( strcmp(argv[i],"-hi") == 0 ) {
			if( i >= argc-1 ) {
				usage(argv[0]);
			}
			enc.edge = ENC_EDGE_HIGHLIGHT;
			errno = 0;
			enc.color_rgb = strtoul(argv[++i],0,16);
			if( errno ) {
				usage(argv[0]);
			}
		}
		else if( strcmp(argv[i],"-frames") == 0 ) {
			if( i >= argc-1 ) {
				usage(argv[0]);
			}
			if( enc.renderer ) {
				usage(argv[0]);
			}
			enc.renderer = 0;
			dump_frames = atoi(argv[++i]);
			if( dump_frames == 0 ) {
				usage(argv[0]);
			}
		}
		#ifdef USE_LIBSIXEL
		else if( strcmp(argv[i],"-sixel") == 0 ) {
			if( enc.renderer ) {
				usage(argv[0]);
			}
			enc.renderer = ENC_RENDER_SIXEL;
		}
		#endif //SIXEL
		else if( strcmp(argv[i],"-double") == 0 ) {
			if( enc.renderer ) {
				usage(argv[0]);
			}
			enc.renderer = ENC_RENDER_DOUBLE;
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
		#ifdef USE_AALIB
		else if( strcmp(argv[i],"-aadl") == 0 ) {
			if( enc.renderer ) {
				usage(argv[0]);
			}
			enc.renderer = ENC_RENDER_AADL;
		}
		else if( strcmp(argv[i],"-aadlext") == 0 ) {
			if( enc.renderer ) {
				usage(argv[0]);
			}
			enc.renderer = ENC_RENDER_AADLEXT;
		}
		else if( strcmp(argv[i],"-aadlext") == 0 ) {
			if( enc.renderer ) {
				usage(argv[0]);
			}
			enc.renderer = ENC_RENDER_AADLEXT;
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
			if( vidpath != 0 ) {
				usage(argv[0]);
			}
			vidpath = argv[i];
		}
		i++;
	}
	
	if( vidpath == 0 ) {
		fprintf(stderr,"No Video path specified\n");
		return 1;
	}
	
	if( !enc.renderer && !dump_frames ) {
		fprintf(stderr,"No renderer specified\n");
		return 1;
	}
	
	enc.enctext = 1;
	enc.clearterm = 0;

	//Double check special sixel concerns
	if( enc.renderer == ENC_RENDER_SIXEL && (verbose || srtfile )) {
		fprintf(stderr,"Verbose output and subtitles are not supported with sixel renderer.\n");
		return 1;
	}
	
	// Open video file
	if(avformat_open_input(&pFormatCtx, vidpath, NULL, NULL)!=0) {
		fprintf(stderr,"Failed to open Video file.\n");
		return 1;
	}

	// Retrieve stream information
	if(avformat_find_stream_info(pFormatCtx, NULL)<0) {
		fprintf(stderr,"Failed to to find stream information\n");
		return 1;
	}

	// Find the first video (and audio) stream
	for(i=0; i<pFormatCtx->nb_streams; i++) {
		if(pFormatCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO) {
			if( videoStream < 0 ) {
				videoStream=i;
			}
		}
		#ifdef USE_PORTAUDIO
		else if(pFormatCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO) {
			if( audioStream < 0 && !mute ) {
				audioStream = i;
			}
		}
		#endif
	}
	if(videoStream==-1) {
		fprintf(stderr,"Failed to find Video stream\n");
		return 1;
	}

	// Find the decoder for the video stream
	pVideoCodec=avcodec_find_decoder(pFormatCtx->streams[videoStream]->codecpar->codec_id);
	if( !pVideoCodec ) {
		fprintf(stderr,"Unsupported video codec!\n");
		return 1; // Codec not found
	}
	// Copy context
	pVideoCodecCtx = avcodec_alloc_context3(pVideoCodec);
	if( pVideoCodecCtx == 0 ) {
		fprintf(stderr,"Failed to allocate video codec context\n");
		return 1;
	}
	if( avcodec_parameters_to_context(pVideoCodecCtx,pFormatCtx->streams[videoStream]->codecpar) < 0 ) {
		fprintf(stderr,"Couldn't copy video codec parameters");
		return 1;
	}
	// Open Video Codec
	if( avcodec_open2(pVideoCodecCtx, pVideoCodec, NULL) ) {
		fprintf(stderr,"Failed to open video codec\n");
		return 1;
	}
	
	//Calculate the proper scale sizes for this
	//terminal window and renderer.  This helps
	//Ensure that the encoding fits inside the
	//terminal window and that term_encode doesn't
	//try to resize the image again.
	if( ioctl(0,TIOCGWINSZ,&ws) ) {
		fprintf(stderr,"Failed to determine terminal size\n");
		return 1;
	}
	if( enc.renderer == ENC_RENDER_NONE ) {
		scale_width   = pVideoCodecCtx->width;
		scale_height  = pVideoCodecCtx->height;
	}
	else if( enc.renderer == ENC_RENDER_SIXEL ) {
		if( enc.win_width == 0 ) {
			#ifdef DEBUG
			fprintf(stderr,"Using original image size of sixel render\n");
			#endif
			enc.win_width = pVideoCodecCtx->width;
		}
		ratio = (float)pVideoCodecCtx->height / (float)pVideoCodecCtx->width;
		scale_width   = enc.win_width;
		scale_height  = scale_width * ratio;
	}
	else {
		if( enc.win_width == 0 ) {
			enc.win_width = ws.ws_col;
		}
		if( verbose ) {
			//Leave room at the bottom for verbose output
			win_height = ws.ws_row - 1;
		} else {
			win_height = ws.ws_row;
		}
		//All selectable renderers are 2 pixels per character in
		//height, so we can double our effective terminal height
		win_height = win_height * 2;
		
		//If the video is being cropped, then we have to 
		//send it into term_encode pixel perfect (so the 
		//crop coodinates are correct).  Let sws_scale "resize" the
		//frame to the same size, but attempt to adjust enc.win_width 
		//so the whole cropped results can fit on a single terminal screen.
		if( enc.crop.w || enc.crop.h ) {
			scale_width   = pVideoCodecCtx->width;
			scale_height  = pVideoCodecCtx->height;
			if( enc.crop.y+enc.crop.h > pVideoCodecCtx->height ) {
				ratio = (float)(pVideoCodecCtx->height - enc.crop.y);
			} else {
				ratio = (float)enc.crop.h;
			}
			if( enc.crop.x+enc.crop.w > pVideoCodecCtx->width ) {
				ratio = ratio / (float)(pVideoCodecCtx->width - enc.crop.x);
			} else {
				ratio = ratio / (float)enc.crop.w;
			}
			while( enc.win_width * ratio > win_height ) {
				enc.win_width--;
			}
		}
		//If the video is not being cropped, then attempt
		//to set the FFMPEG scale so that the whole frame can
		//fit on a single termal screen, but also adjust where
		//possible to speed up term_encode by preventing another
		//resize after sws_scale.
		else {
			ratio = (float)pVideoCodecCtx->height / (float)pVideoCodecCtx->width;
			#ifdef DEBUG
			fprintf(stderr,"Frame Size: %d / %d\n",pVideoCodecCtx->height,pVideoCodecCtx->width);
			fprintf(stderr,"Crop: x(%ld) y(%ld) w(%ld) h(%ld)\n",enc.crop.x, enc.crop.y,enc.crop.w,enc.crop.h);
			fprintf(stderr,"Ratio  %f = %f / %f\n",ratio,(float)pVideoCodecCtx->height,(float)pVideoCodecCtx->width);
			fprintf(stderr,"Terminal Size: %ld / %ld\n",enc.win_width,win_height);
			#endif
			scale_width = enc.win_width;
			scale_height = scale_width * ratio;
			while( scale_height > win_height && scale_width > 2) {
				scale_width--;
				scale_height = scale_width * ratio;
			}
			enc.win_width = scale_width;
			#ifdef DEBUG
			fprintf(stderr,"Scaled Frame Size: %ld / %ld\n",scale_width,scale_height);
			#endif
			//Adjust the scale width based upon the selected renderer
			//This a bit of a cheat to prevent term_encode from resizing the image later
			//to accomodate specific renderers.
			if( enc.renderer == ENC_RENDER_SEXTANT ) {
				scale_width = scale_width*3/2;
			}
			else if( enc.renderer >= ENC_RENDER_QUARTER && enc.renderer <= ENC_RENDER_AABGEXT ) {
				scale_width = scale_width*2;
			}
			#ifdef DEBUG
			fprintf(stderr,"Prescaled Frame Size for renderer: %ld / %ld\n",scale_width,scale_height);
			#endif
		}
	}
	
	// Allocate video frame
	pFrame=av_frame_alloc();

	// Allocate an AVFrame structure
	pFrameRGB=av_frame_alloc();
	if(pFrameRGB==NULL) {
		fprintf(stderr,"Failed to allocate frame\n");
		return 1;
	}

	// Determine required buffer size and allocate buffer
	numBytes=av_image_get_buffer_size(AV_PIX_FMT_RGB24, scale_width, scale_height,1);
	buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	if( av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24, 
		 scale_width, scale_height,1) < 0 ) {
		fprintf(stderr,"Failed to fill RGB frame\n");
		return 1;
	}

	// initialize SWS context for software scaling
	sws_ctx = sws_getContext(
		pVideoCodecCtx->width, pVideoCodecCtx->height, pVideoCodecCtx->pix_fmt,
		scale_width, scale_height, AV_PIX_FMT_RGB24,
		SWS_BILINEAR,
		NULL,NULL,NULL);
	
	frame_period_sec = findFramePeriod(pFormatCtx->streams[videoStream],pVideoCodecCtx,23.976024);
	seek_frame_count = seek_time_sec / frame_period_sec;
	
	#ifdef USE_PORTAUDIO
	if( audioStream != -1 ) {
		// Find the decoder for the audio stream
		pAudioCodec=avcodec_find_decoder(pFormatCtx->streams[audioStream]->codecpar->codec_id);
		if( !pAudioCodec ) {
			fprintf(stderr,"Unsupported audio codec!\n");
			return 1; // Codec not found
		}
		// Copy context
		pAudioCodecCtx = avcodec_alloc_context3(pAudioCodec);
		if( pAudioCodecCtx == 0 ) {
			fprintf(stderr,"Failed to allocate audio codec context\n");
			return 1;
		}
		if( avcodec_parameters_to_context(pAudioCodecCtx,pFormatCtx->streams[audioStream]->codecpar) < 0 ) {
			fprintf(stderr,"Couldn't copy audio codec parameters");
			return 1;
		}
		// Open Audio Codec
		if( avcodec_open2(pAudioCodecCtx, pAudioCodec, NULL) ) {
			fprintf(stderr,"Failed to open audio codec\n");
			return 1;
		}

		if( pAudioCodecCtx->sample_fmt == AV_SAMPLE_FMT_FLTP ) {
			paBuffer.format = paFloat32;
		} else if( pAudioCodecCtx->sample_fmt == AV_SAMPLE_FMT_S32P ) {
			paBuffer.format = paInt32;
		} else if( pAudioCodecCtx->sample_fmt == AV_SAMPLE_FMT_S16P ) {
			paBuffer.format = paInt16;
		} else if( pAudioCodecCtx->sample_fmt == AV_SAMPLE_FMT_U8P ) {
			paBuffer.format = paUInt8;
		} else {
			fprintf(stderr,"Unsupported audio sample format: %d\n",pAudioCodecCtx->sample_fmt);
			return 1;
		}
		paBuffer.channels = pAudioCodecCtx->channels;
		paBuffer.sample_len =  Pa_GetSampleSize(paBuffer.format);
		paBuffer.frame_len = paBuffer.sample_len*paBuffer.channels;
		paBuffer.len = paBuffer.frame_len*pAudioCodecCtx->sample_rate;
		if( paBuffer.len == 0 ) {
			fprintf(stderr,"Bad buffer len(%ld) channels(%ld) sample_len(%ld) frame_len(%ld) sample_rate(%d)\n",
					paBuffer.len,paBuffer.channels,paBuffer.sample_len,paBuffer.frame_len,pAudioCodecCtx->sample_rate);
			return 1;
		}
		paBuffer.samples = (uint8_t*)malloc(paBuffer.len);
		if( paBuffer.samples == 0 ) {
			fprintf(stderr,"Failed to allocate audio samples buffer\n");
			return 1;
		}

		if( Pa_Initialize() ) {
			fprintf(stderr,"Failed to initialize PortAudio\n");
			return 1;
		}
		if( Pa_OpenDefaultStream( &(paBuffer.stream),0,pAudioCodecCtx->channels,
						paBuffer.format,pAudioCodecCtx->sample_rate,
						pAudioCodecCtx->frame_size,
						paCallback,(void*)&paBuffer) ) {
			fprintf(stderr,"Failed to open PortAudio stream\n");
			return 1;
		}
		paBuffer.read_off  = 0;
		paBuffer.write_off = 0;
		paInfo = Pa_GetStreamInfo(paBuffer.stream);
	}
	#endif
	
	//Setup initial time values
	//start_time will be filled by our first frame;
	start_time.tv_sec = 0;
	start_time.tv_nsec = 0;
	frame_time_sec = 0;
	display_time_sec = frame_time_sec + seek_time_sec;
	
	if( srtfile ) {
		if( nextSubtitle( &subtitle, srtfile) ) {
			fclose(srtfile);
			srtfile = 0;
		}
	}
		
	// Read frames
	while(av_read_frame(pFormatCtx, &packet)>=0) {
		#ifdef USE_PORTAUDIO
		if( packet.stream_index==audioStream ) {
			if( seek_frame_count ) {
				av_packet_unref(&packet);
				continue;
			}
			avcodec_send_packet(pAudioCodecCtx,&packet);
			if( avcodec_receive_frame(pAudioCodecCtx,pFrame) == 0 ) {
				paWriteBuffer(&paBuffer,pFrame->data,pFrame->nb_samples);
			}
		} else
		#endif
		if(packet.stream_index==videoStream) {
			if( seek_frame_count ) {
				seek_frame_count--;
				if( verbose ) {
					fprintf(stderr,"\rSeeking frames(%08ld)",seek_frame_count);
				}
				av_packet_unref(&packet);
				continue;
			}
			#ifdef USE_PORTAUDIO
			if( audioStream != -1 && !Pa_IsStreamActive(paBuffer.stream) ) {
				av_packet_unref(&packet);
				continue;
			}
			#endif
			// Decode video frame
			avcodec_send_packet(pVideoCodecCtx,&packet);
			if( avcodec_receive_frame(pVideoCodecCtx,pFrame) == 0 ) {
				#ifdef DEBUG
				fprintf(stderr,"Got a new frame %08lu\n",frame_number);
				#endif
				clock_gettime(CLOCK_MONOTONIC,&current_time);
				//If this is the first frame, then use the curren time as
				//the epoch for our frame time calculations.
				if( !start_time.tv_sec && !start_time.tv_nsec ) {
					#ifndef DEBUG
					printf("\x1b[2J\x1b[0m");
					#endif
					start_time.tv_nsec = current_time.tv_nsec;
					start_time.tv_sec  = current_time.tv_sec;
					#ifdef USE_PORTAUDIO
					if( audioStream != -1 ) {
						//Adjust the start time of the first frame to sync with the 
						//associated audio.  We add the PortAudio stream output
						//latency and then 0.5 sec, because paBuffer.len represents 
						//1 sec and we start the stream when it is half full.
						timespec2double(&tmp_time_sec,&start_time);
						double2timespec(&start_time, tmp_time_sec+paInfo->outputLatency+0.5);
					}
					#endif
					#ifdef DEBUG
					fprintf(stderr,"First frame used as epoch\n");
					#endif
				}
				//Adjust the current time to be a reference to start time
				timediff(&current_time,&current_time,&start_time);
				
				//Fill in the frame/display timespecs
				double2timespec(&frame_time,frame_time_sec);
				double2timespec(&display_time,display_time_sec);
				
				#ifdef DEBUG
				fprintf(stderr,"current time: tv_sec(%lu) tv_nsec(%lu)\n",current_time.tv_sec,current_time.tv_nsec);
				fprintf(stderr,"frame time: tv_sec(%lu) tv_nsec(%lu)\n",frame_time.tv_sec,frame_time.tv_nsec);
				#endif
				
				if( enc.renderer && timediff(&sleep_time,&frame_time,&current_time) < 0 ) {
					#ifdef DEBUG
					fprintf(stderr,"SKIP FRAME!\n");
					#endif
					skip++;
				}
				else {
					if( enc.renderer ) {
						//Wait until enough time has past to renderer the next frame
						#ifdef DEBUG
						fprintf(stderr,"sleep tv_sec(%lu) tv_nsec(%lu)\n",sleep_time.tv_sec,sleep_time.tv_nsec);
						#endif
						nanosleep(&sleep_time,0);
					}
					
					sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
						pFrame->linesize, 0, pVideoCodecCtx->height,
						pFrameRGB->data, pFrameRGB->linesize);

					if( enc.renderer ) {
						enc.imgpixels = pFrameRGB->data[0];
						enc.imgwidth  = scale_width;
						enc.imgheight = scale_height;
						#ifndef DEBUG
						printf("\x1b[H");
						#endif
						term_encode(&enc);
						
						if( srtfile ) {
							#ifdef DEBUG
							printf("subtitle start.tv_sec(%ld) start.tv_nsec(%ld) end.tv_sec(%ld) end_tv.nsec(%ld)\n",
										 subtitle.start_time.tv_sec,subtitle.start_time.tv_nsec,
										 subtitle.end_time.tv_sec  ,subtitle.end_time.tv_nsec );
							#endif
							if( timediff(0,&display_time,&(subtitle.start_time)) >= 0 ) {
								while( srtfile && timediff(0,&display_time,&(subtitle.end_time)) >= 0 ) {
									if( nextSubtitle( &subtitle, srtfile) ) {
										fclose(srtfile);
										srtfile = 0;
									}
								}
								if( srtfile ) {
									printf("\x1b[%ldF",subtitle.linecount+1);
									psub = subtitle.text;
									for( i=0; i<subtitle.linecount; i++ ) {
										for( linelen=0; psub[linelen] != 0 && psub[linelen] != '\n'; linelen++ ){ ; } //Find strlen
										if( linelen > enc.win_width ) { lineoff = 0; } 
										else { lineoff = (enc.win_width-linelen)/2; }
										printf("\x1b[%ldG",lineoff);
										fwrite(psub,1,linelen,stdout);
										psub = psub + linelen + 1;
										printf("\x1b[E");
									}
									printf("\x1b[E");
								}
							}
						}
						
						if( verbose ) {
							printf("\r                                           \r");
							printf("Frame(%08lu) Time(%02lu:%02lu:%02lu.%03lu) Skip(%02d)",frame_number,
								(time_t)(display_time_sec)/3600,
								(time_t)(display_time_sec)%3600 / 60,
								(time_t)(display_time_sec)%60,
								(time_t)(display_time_sec*1000.0) % 1000,
								skip);
						}
						fflush(stdout);
						skip = 0;
					} else if( dump_frames ) {
						printf("\rSaving frame(%08ld)",frame_number);
						savePPM(frame_number,pFrameRGB->data[0],scale_width,scale_height);
						dump_frames--;
						if( dump_frames == 0 ) {
							break;
						}
					}
				}
				#ifdef DEBUG
				fprintf(stderr,"Done with frame. %08lu\n",frame_number);
				#endif
				frame_number++;
				frame_time_sec = frame_period_sec * frame_number;
				display_time_sec = frame_time_sec + seek_time_sec;
			}
		}
		// Free the packet that was allocated by av_read_frame
		av_packet_unref(&packet);
	}

	sws_freeContext(sws_ctx);

	// Free the RGB image
	//buffer will be freed by term_encode_destroy
	//av_free(buffer);
	av_frame_free(&pFrameRGB);

	// Free the YUV frame
	av_frame_free(&pFrame);

	// Close the codecs
	avcodec_close(pVideoCodecCtx);
	
	// Close the video file
	avformat_close_input(&pFormatCtx);
	
	term_encode_destroy(&enc);
		
	if( srtfile ) {
		fclose(srtfile);
	}
	
	#if USE_PORTAUDIO
		avcodec_close(pAudioCodecCtx);
		if( Pa_IsStreamActive(paBuffer.stream) ) {
			Pa_AbortStream(paBuffer.stream);
			while( Pa_IsStreamActive(paBuffer.stream) );
		}
		Pa_Terminate();
		free(paBuffer.samples);
	#endif
	return 0;
}
