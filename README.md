# newdraw
A Unicode termial art program for the terminal.  The goal of this program
is to be portable and simple.  In addition to the editor, conversion tools
are also included.  The imgconvert command can render images to the terminal
and generate files that can be opened by the editor.  The vidconvert command
can render video files to the terminal.

# Building:
The editor itself has no requirements beyond libc.  The included image 
conversion tool has additional, optional, dependencies.

Edit Makefile to set the following variables base on what libraries are 
available:

USE_LIBSIXEL=1 or 0

USE_AALIB=1 or 0

USE_LIBCACA=1 or 0

USE_FFMPEG=1 or 0

USE_PORTAUDIO=1 or 0

USE_QUANTPNM=1 or 0

USE_DEBUG=1 or 0


The only option that affect newdraw is USE_DEBUG, which enables certaind debugging
output.  The other options affect imgconvert and vidconvert.  

Libsixel is required for sixel output (not supported by newdraw).  AALib and Libcaca 
are required for ouptut based upon their rendering engines respectively.  If 
USE_FFMPEG is set, then vidconvert will be built, using the ffmpeg libraries for
video decoding.  If USE_PORTAUDIO is set, then audio support will be included in
vidconvert.

Two different algorithms are avilable for quantizing/palettizing images prior to 
rendering.  One is a simple, non-dithered, algorithm which is used by default.  If
USE_QUANTPNM, then the mediancut algorithm use in pnmcolormap (and libsixel) is
used instead.

Run make

This will build newdraw, imgconvert, and optionally vidconvert.

# newdraw usage:
newdraw [-h] [-s width height] [-m 16|256|true] [-dos] [-c codepage(hex)]
      [-es] [-nec] binpath

  -h  : Print usage message
  -m  : Select color mode -
           16 color standard palette
          256 color standard palette
         true color - 24 bit
  -dos: Use DOS character set (codepage 437)
        Exports are still in UTF-8
  -c  : Starting special character code page
  -es : DISABLE export of blank characters as spaces
  -ec : DISABLE export start with terminal clear


The editor can open and save an internal binary format, but it can export 
standard files that cat be interpreted directly by a terminal.  The bottom
two lines of the screen are the menu.  The menu items are accesiable by 
hitting the Escape key.  Hitting Escape again will exit the menu and allow for 
regular typing.


# imgconvert usage:
imgconvert [-h] [-sp 16|256 | -p #] [-w #] [-c] [-b binfile]
     [-crop x y w h]  [-edge | -line | -glow | -hi 0xFFGGBB] renderer imgfile

-h   : Print usage message
-sp  : Use standard 16 or 256 color palette
-p   : Use a true color palette of # colors (<= 256)
-w   : Set the character width (terminal width used by default)
-c   : Clear terminal
-b   : Binary file to save (for newdraw)
-crop: Crop the image before processing
-edge: Render edge detection (scaled) using specified color
-line: Render edges as solid lines using specified color
-glow: Mix edge detection (scaled) with image
-hi  : Mix edge line with images

-= Renderers =-
-sixel  : Sixel
-simple : ANSI Simple
-half   : ANSI Half-Height
-double : ANSI Double-Line
-qchar  : ANSI Quarter-Character
-aa     : ASCII Art
-aaext  : ASCII Art with extended characters
-aadl   : ASCII Art Double-Line
-aadlext: ASCII Art Double-Line with extended characaters
-aafg   : ASCII Art Dobule-Line with Foreground Color
-aafgext: ASCII Art Dobule-Line with Foreground Color and
          extended characters
-aabg   : ASCII Art Double-Line with Background Color
-aabgext: ASCII Art Double-Line with Background Color and
          extended characters
-caca   : CACA (Color ASCII Art)
-cacablk: CACA with block characters


The converter reads in an image file, and renders it to the terminal using
one of the avaialble renderers.  Optionally, a binary file can be saved that
can be opened by the editor.


# vidconvert usage:
vidconvert [-h] [-v] [-srt subfile] [-seek 0:00:00.000] [-sp 16|256 | -p #] [-w #]
  [-crop x y w h] [-edge | -line | -glow | -hi 0xFFGGBB] [renderer] vidfile

-h   : Print usage message
-v   : Print information after the frame
-srt : Show subtitles from specified .srt file
-seek: Seek to specifed time code
-sp  : Use standard 16 or 256 color palette
-p   : Use a true color palette of # colors (<= 256)
-w   : Set the character width (terminal width used by default)
-crop: Crop the video before processing
-edge: Render edge detection (scaled) using specified color
-line: Render edges as solid lines using specified color
-glow: Mix edge detection (scaled) with image
-hi  : Mix edge line with images

If no renderer is specified, the first frame will be saved to frame.ppm

-= Renderers =-
-sixel  : Sixel (not well supported)
-half   : ANSI Half-Height
-double : ANSI Double-Line
-qchar  : ANSI Quarter-Character
-aadl   : ASCII Art Double-Line
-aadlext: ASCII Art Double-Line with extended characaters
-aafg   : ASCII Art Dobule-Line with Foreground Color
-aafgext: ASCII Art Dobule-Line with Foreground Color and
          extended characters
-aabg   : ASCII Art Double-Line with Background Color
-aabgext: ASCII Art Double-Line with Background Color and
          extended characters
-caca   : CACA (Color ASCII Art)
-cacablk: CACA with block characters


The converter reads in a media file (image or video), and renders it to the
terimal using one of the available renderes. If no renderers are provided,
then the frames are dumped to the files frameXXXXXXXXX.ppm.  If audio support
was included at compile time, then the first audio stream will be played 
to the default output device.
