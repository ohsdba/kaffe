/**
 * toolkit.h - 
 *
 * Copyright (c) 1998
 *      Transvirtual Technologies, Inc.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution 
 * of this file. 
 */

#ifndef __toolkit_h
#define __toolkit_h

#include "config.h"
#include "config-std.h"
#include "config-mem.h"
#include <native.h>		/* needed for enter/leaveUnsafeRegion */

#define DBG(x,y)
#define DBG_ACTION(x,y)

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

#include <jni.h>
#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include "config.h"
#include "config-mem.h"

/*******************************************************************************
 * color conversion structures
 */

#define N_DIRECT 256

typedef struct _Rgb2Direct {
  unsigned char red[N_DIRECT];
  unsigned char redPix[N_DIRECT];
  int           redShift;
  int           nRed;
  unsigned char green[N_DIRECT];
  unsigned char greenPix[N_DIRECT];  
  int           greenShift;
  int           nGreen;
  unsigned char blue[N_DIRECT];
  unsigned char bluePix[N_DIRECT];
  int           blueShift;
  int           nBlue;
} Rgb2Direct;

typedef struct _Rgb2True {
  unsigned int   redMask;
  unsigned int   greenMask;
  unsigned int   blueMask;
  int            blueShift;
  int            redShift;
  int            greenShift;
} Rgb2True;


typedef struct _RgbColor {
  unsigned char r;
  unsigned char g;
  unsigned char b;
} RgbColor;


typedef struct _Rgb2Pseudo {
  RgbColor       rgb[256];
  unsigned char  pix[8][8][8];
} Rgb2Pseudo;


/*******************************************************************************
 * image handling structures
 */

typedef struct _AlphaImage {       /* storage for full alpha channel images */
  unsigned char *buf;
  int           width, height;
} AlphaImage;


#define NO_SHM       0             /* we don't have MIT-Shm support in the X server */
#define USE_SHM      1             /* we have support, use it */
#define SUSPEND_SHM  2             /* we have support, but it ran out of space */

typedef struct _Image {
  Pixmap           pix;            /* pixmap for screen images */

  XImage           *xImg;          /* "real" image */
  XShmSegmentInfo  *shmiImg;       /* Shm info for shared mem real image */

  XImage           *xMask;         /* mask image for reduced alpha (on/off) images */
  XShmSegmentInfo  *shmiMask;      /* Shm info for shared mem mask image */

  AlphaImage       *alpha;         /* full alpha channel (for alpha != 0x00 or 0xff) */

  int              trans;          /* transparent index */
  int              width, height;  /* we need this in case we are a pixmap */

  int              latency;        /* between image flips, for "gif-movies" */
  struct _Image    *next;          /* next movie-frame */
} Image;


/*******************************************************************************
 * structure to store guessed and computed Frame/Dialog insets (titlebar, borders)
 */

typedef struct _DecoInset {
  int            left;
  int            top;
  int            right;
  int            bottom;
  char           guess;
} DecoInset;           


/*******************************************************************************
 * this is the master AWT structure (singleton object), glueing it al together
 */

typedef struct _Toolkit {
  Display        *dsp;
  Window         root;

  char           *buf;
  unsigned int   nBuf;

  int            colorMode;      /* refers to CM_xx constants, not X visuals */
  Rgb2True       *tclr;
  Rgb2Pseudo     *pclr;
  Rgb2Direct     *dclr;

  int            shm;
  int            shmThreshold;

  Cursor         cursors[14];

  DecoInset      frameInsets;
  DecoInset      dialogInsets;

  XEvent         event;
  char           preFetched;
  char           blocking;
  int            pending;
  int            evtId;

  Window         lastWindow;
  int            srcIdx;
  Window         *windows;
  int            nWindows;

  Window         cbdOwner;
  Window         wakeUp;

  Window         newWindow;
} Toolkit;


/*******************************************************************************
 * global data def/decl
 */
#ifdef MAIN

Toolkit   XTk;
Toolkit   *X = &XTk;

Atom WM_PROTOCOLS;
Atom WM_DELETE_WINDOW;
Atom WM_TAKE_FOCUS;
Atom WAKEUP;
Atom RETRY_FOCUS;
Atom SELECTION_DATA;
Atom JAVA_OBJECT;

jclass  AWTError;
JNIEnv  *JniEnv;

#else

extern Toolkit* X;

extern Atom WM_PROTOCOLS;
extern Atom WM_DELETE_WINDOW;
extern Atom WM_TAKE_FOCUS;
extern Atom WAKEUP;
extern Atom RETRY_FOCUS;
extern Atom SELECTION_DATA;
extern Atom JAVA_OBJECT;

extern jclass AWTError;
extern JNIEnv* JniEnv;

#endif /* MAIN */

extern long StdEvents;

/*******************************************************************************
 *
 */


static __inline__ char* java2CString ( JNIEnv *env, Toolkit* X, jstring jstr ) {
  jboolean isCopy;
  register int i;
  int      n = (*env)->GetStringLength( env, jstr);
  const jchar    *jc = (*env)->GetStringChars( env, jstr, &isCopy);

  if ( n >= X->nBuf ) {
	if ( X->buf )
	  KFREE( X->buf);
	X->buf = KMALLOC( n+1);
	X->nBuf = n+1;
  }

  for ( i=0; i<n; i++ ) X->buf[i] = (char) jc[i];
  X->buf[i] = 0;
  (*env)->ReleaseStringChars( env, jstr, jc);

  return X->buf;
}

static __inline__ char* jchar2CString ( Toolkit* X, jchar* jc, int len ) {
  register int i;
  int      n = len+1;
  
  if ( n > X->nBuf ) {
	if ( X->buf )
	  KFREE( X->buf);
	X->buf  = KMALLOC( n);
	X->nBuf = n;
  }

  for ( i=0; i<len; i++ ) X->buf[i] = (char) jc[i];
  X->buf[i] = 0;

  return X->buf;
}

static __inline__ void* getBuffer ( Toolkit* X, unsigned int nBytes ) {
  if ( nBytes > X->nBuf ) {
	if ( X->buf )
	  KFREE( X->buf);
	X->buf  = KMALLOC( nBytes);
	X->nBuf = nBytes;
  }
  return X->buf;
}


/*****************************************************************************************
 * color functions & defines
 */

/*
 * These are our directly supported visuals / color modes. Note that there is
 * no more 1-1 correspondence to X visuals, since we do a categorisation with
 * respect to our internal RGB <-> pixel conversion. All visuals not listed
 * explicitly are handled via the generic XAllocColor/XQueryColor (which might
 * slow down images considerably)
 *
 * NOTE: these values have to be != 0, since '0' is used to trigger color init
 */
#define CM_PSEUDO_256   1  /* PseudoColor visual */
#define CM_TRUE         2  /* general TrueColor visual */
#define CM_TRUE_888     3  /* special 8-8-8 bit TrueColor visual */
#define CM_DIRECT       4
#define CM_GENERIC      5  /* grays, DirectColor (packed) etc. */


void initColorMapping ( JNIEnv* env, Toolkit* X);
jlong Java_java_awt_Toolkit_clrBright ( JNIEnv* env, jclass clazz, jint rgb );
jlong Java_java_awt_Toolkit_clrDark ( JNIEnv* env, jclass clazz, jint rgb );

#define JRGB(_r,_g,_b)  (_r<<16 | _g<<8 | _b)
#define JRED(_rgb)      ((_rgb & 0xff0000) >> 16)
#define JGREEN(_rgb)    ((_rgb & 0x00ff00) >> 8)
#define JBLUE(_rgb)     (_rgb & 0x0000ff)

#define D8   36.43  /* 255 / 7 */
#define D16  18.21

#define JI8(_v) (int)((_v + D16) / D8)
#define XI8(_v) (int)(((_v>>8) + D16) / D8)

#define ROUND_SHORT2CHAR(_n) \
  ((unsigned short)_n >= 0xff70 ) ? 0xff : (unsigned char)(((unsigned short)_n + 0x80) >> 8)

/* shift ops with negative values have undefined results */
#define SIGNED_RSHIFT(_var,_val,_shift)   \
   _var = _val;                           \
   if ( _shift > 0 )	 _var >>= _shift; \
   else                  _var <<= -_shift;

#define SIGNED_LSHIFT(_var,_val,_shift)   \
   _var = _val;                           \
   if ( _shift > 0 )	 _var <<= _shift; \
   else                  _var >>= -_shift;


static __inline__ jint
pixelValue ( Toolkit* X, jint rgb )
{
  int      r,g,b;
  XColor   xclr;

  switch ( X->colorMode ) {
  case CM_PSEUDO_256:
	return X->pclr->pix [JI8(JRED(rgb))] [JI8(JGREEN(rgb))] [JI8(JBLUE(rgb))];

  case CM_TRUE:
	SIGNED_RSHIFT( b, (rgb & X->tclr->blueMask), X->tclr->blueShift);
	SIGNED_RSHIFT( g, (rgb & X->tclr->greenMask), X->tclr->greenShift);
	SIGNED_RSHIFT( r, (rgb & X->tclr->redMask), X->tclr->redShift);
	return ( b | g | r );

  case CM_TRUE_888:
	return (rgb & 0xffffff);

  case CM_DIRECT:
	return (((jint)X->dclr->redPix[JRED(rgb)]     << X->dclr->redShift) |
			((jint)X->dclr->greenPix[JGREEN(rgb)] << X->dclr->greenShift) |
			((jint)X->dclr->bluePix[JBLUE(rgb)]   << X->dclr->blueShift));

  default:
	/*
	 * this is a generic fallback for "exotic" visuals and might be *awefully*
	 * slow (esp. for images) because XAllocColor is a roundtrip
	 */
	xclr.red = (rgb & 0xff0000) >> 8;
	xclr.green = (rgb & 0xff00);
	xclr.blue = (rgb & 0xff) << 8;
	xclr.flags = DoRed | DoGreen | DoBlue;
	XAllocColor( X->dsp, DefaultColormapOfScreen( DefaultScreenOfDisplay( X->dsp)), &xclr);
	return xclr.pixel;
  }
}

static __inline__ void
rgbValues ( Toolkit* X, unsigned long pixel, int* r, int* g, int* b )
{
  Visual         *v;
  XColor         xclr;

  switch ( X->colorMode ) {
  case CM_PSEUDO_256:
	*r = X->pclr->rgb[(unsigned char)pixel].r;
	*g = X->pclr->rgb[(unsigned char)pixel].g;
	*b = X->pclr->rgb[(unsigned char)pixel].b;
	break;

  case CM_TRUE:
	v = DefaultVisual( X->dsp, DefaultScreen( X->dsp));
	SIGNED_LSHIFT( *r, (pixel & v->red_mask), X->tclr->redShift);
	SIGNED_LSHIFT( *g, (pixel & v->green_mask), X->tclr->greenShift);
	SIGNED_LSHIFT( *b, (pixel & v->blue_mask), X->tclr->blueShift);
	*r >>= 16;
	*g >>= 8;
	break;
/*
	*r = ((pixel & v->red_mask)   << X->tclr->redShift)   >> 16;
	*g = ((pixel & v->green_mask) << X->tclr->greenShift) >> 8;
	*b = ((pixel & v->blue_mask)  << X->tclr->blueShift);
	break;
*/
  case CM_TRUE_888:
	*r = JRED( pixel);
	*g = JGREEN( pixel);
	*b = JBLUE( pixel);
	break;

  case CM_DIRECT:
	v = DefaultVisual( X->dsp, DefaultScreen( X->dsp));
	*r = X->dclr->red[   ((pixel & v->red_mask) >> X->dclr->redShift) ];
	*g = X->dclr->green[ ((pixel & v->green_mask) >> X->dclr->greenShift) ];
	*b = X->dclr->blue[  ((pixel & v->blue_mask) >> X->dclr->blueShift) ];
	break;

  default:
	/*
	 * this is a generic fallback for "exotic" visuals and might be *awefully*
	 * slow (esp. for images) because XAllocColor is a roundtrip
	 */
	xclr.pixel = pixel;
	XQueryColor( X->dsp, DefaultColormapOfScreen( DefaultScreenOfDisplay( X->dsp)), &xclr);
	*r = xclr.red >> 8;
	*g = xclr.green >> 8;
	*b = xclr.blue >> 8;

    break;
  }
}


/*****************************************************************************************
 * image functions
 */

Image* createImage ( int width, int height);
void createXMaskImage ( Toolkit* X, Image* img );
void createXImage ( Toolkit* X, Image* img );
void createAlphaImage ( Toolkit* X, Image* img );
void initScaledImage ( Toolkit* X, Image *tgt, Image *src,
					   int dx0, int dy0, int dx1, int dy1,
					   int sx0, int sy0, int sx1, int sy1 );
void Java_java_awt_Toolkit_imgFreeImage( JNIEnv* env, jclass clazz, Image * img);


static __inline__ void
PutAlpha ( AlphaImage* img, int col, int row, unsigned char alpha )
{
  img->buf[ row*img->width + col ] = alpha;
}

static __inline__ int
GetAlpha ( AlphaImage* img, int col, int row )
{
  return img->buf[ row*img->width + col];
}

/*****************************************************************************************
 * clipboard functions
 */

jobject selectionClear ( JNIEnv* env, Toolkit* X );
jobject selectionRequest ( JNIEnv* env, Toolkit* X );


/*****************************************************************************************
 * async (multithreaded) macros
 * this can be used to solve the problem of deferred drawing requests, not being
 * flushed because of infrequent (non-polled) XNextEvent calls.
 * (for now, we go with a backgound flush thread)
 */

#define XFLUSH(_X,_force)


/*****************************************************************************************
 * heap wrapper macros
 */

//#undef malloc
//#undef calloc
//#undef free

static __inline__ void* _awt_malloc_wrapper ( size_t size )
{
  void *adr;
  enterUnsafeRegion();
  adr = malloc( size);
  leaveUnsafeRegion();
  DBG( awt_mem, ("malloc: %d  -> %x\n", size, adr));
  return adr;
}

static __inline__ void* _awt_calloc_wrapper ( int n, size_t size )
{
  void *adr;
  enterUnsafeRegion();
  adr = calloc( n, size);
  leaveUnsafeRegion();
  DBG( awt_mem, ("calloc: %d,%d  -> %x\n", n, size, adr));
  return adr;
}

static __inline__ void _awt_free_wrapper ( void* adr )
{
  DBG( awt_mem, ("free: %x\n", adr));
  enterUnsafeRegion();
  free( adr);
  leaveUnsafeRegion();
}


#define AWT_MALLOC(_n) \
  _awt_malloc_wrapper( _n)

#define AWT_CALLOC(_n,_sz) \
  _awt_calloc_wrapper( _n, _sz)

#define AWT_FREE(_adr) \
  _awt_free_wrapper( _adr);


#endif
