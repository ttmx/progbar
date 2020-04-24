/* See LICENSE file for copyright and license details. */
#include <ctype.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif
#include <X11/Xft/Xft.h>
#include <X11/Xresource.h>

#include "drw.h"
#include "util.h"

/* macros */
#define INTERSECT(x,y,w,h,r)  (MAX(0, MIN((x)+(w),(r).x_org+(r).width)  - MAX((x),(r).x_org)) \
                             && MAX(0, MIN((y)+(h),(r).y_org+(r).height) - MAX((y),(r).y_org)))
#define LENGTH(X)             (sizeof X / sizeof X[0])

/* define opaqueness */
#define OPAQUE 0xFFU

/* enums */
enum { SchemeNorm, SchemeSel, SchemeOut, SchemeLast }; /* color schemes */

static char *embed;
static int mw, mh;
static int mon = -1, screen;

static Display *dpy;
static Window root, parentwin, win;

static Drw *drw;
static int usergb = 0;
static Visual *visual;
static int depth;
static Colormap cmap;
static Clr *scheme[SchemeLast];

static int percent;

#include "config.h"

static void
xinitvisual()
{
	XVisualInfo *infos;
	XRenderPictFormat *fmt;
	int nitems;
	int i;

	XVisualInfo tpl = {
		.screen = screen,
		.depth = 32,
		.class = TrueColor
	};

	long masks = VisualScreenMask | VisualDepthMask | VisualClassMask;

	infos = XGetVisualInfo(dpy, masks, &tpl, &nitems);
	visual = NULL;

	for (i = 0; i < nitems; i++){
		fmt = XRenderFindVisualFormat(dpy, infos[i].visual);
		if (fmt->type == PictTypeDirect && fmt->direct.alphaMask) {
			visual = infos[i].visual;
			depth = infos[i].depth;
			cmap = XCreateColormap(dpy, root, visual, AllocNone);
			usergb = 1;
			break;
		}
	}

	XFree(infos);

	if (! visual) {
		visual = DefaultVisual(dpy, screen);
		depth = DefaultDepth(dpy, screen);
		cmap = DefaultColormap(dpy, screen);
	}
}

static void
cleanup(void)
{
	size_t i;

	for (i = 0; i < SchemeLast; i++)
		free(scheme[i]);
	drw_free(drw);
	XSync(dpy, False);
	XCloseDisplay(dpy);
}

static void
drawmenu(void)
{
	drw_setscheme(drw, scheme[SchemeNorm]);
	if (background)
		drw_rect(drw, 0, 0, mw, mh, 1, 1);
	else
		drw_rect(drw, mw*(1-bwf)/2 + pos_x - border, 0, mw*bwf + 2*border, mh, 1, 1);

	drw_setscheme(drw, scheme[SchemeOut]);
	drw_rect(drw, mw*(1-bwf)/2 + pos_x, border, mw*bwf, bh, 1, 1);

	drw_setscheme(drw, scheme[SchemeSel]);
	drw_rect(drw, mw*(1-bwf)/2 + pos_x, border, mw*bwf*percent/100, bh, 1, 1);

	drw_map(drw, win, 0, 0, mw, mh);
}

static void
setup(void)
{
	int x, y, i, j;
	unsigned int du;
	XSetWindowAttributes swa;
	Window w, dw, *dws;
	XWindowAttributes wa;
	XClassHint ch = {"progbar", "progbar"};
#ifdef XINERAMA
	XineramaScreenInfo *info;
	Window pw;
	int a, di, n, area = 0;
#endif
	/* init appearance */
	for (j = 0; j < SchemeLast; j++)
		scheme[j] = drw_scm_create(drw, colors[j], alphas[j], 2);


	/* calculate menu geometry */
	mh = bh + 2*border;
#ifdef XINERAMA
	i = 0;
	if (parentwin == root && (info = XineramaQueryScreens(dpy, &n))) {
		XGetInputFocus(dpy, &w, &di);
		if (mon >= 0 && mon < n)
			i = mon;
		else if (w != root && w != PointerRoot && w != None) {
			/* find top-level window containing current input focus */
			do {
				if (XQueryTree(dpy, (pw = w), &dw, &w, &dws, &du) && dws)
					XFree(dws);
			} while (w != root && w != pw);
			/* find xinerama screen with which the window intersects most */
			if (XGetWindowAttributes(dpy, pw, &wa))
				for (j = 0; j < n; j++)
					if ((a = INTERSECT(wa.x, wa.y, wa.width, wa.height, info[j])) > area) {
						area = a;
						i = j;
					}
		}
		/* no focused window is on screen, so use pointer location instead */
		if (mon < 0 && !area && XQueryPointer(dpy, root, &dw, &dw, &x, &y, &di, &di, &du))
			for (i = 0; i < n; i++)
				if (INTERSECT(x, y, 1, 1, info[i]))
					break;

		x = info[i].x_org;
		y = info[i].y_org + (topbar ? pos_y : info[i].height - mh - pos_y);
		mw = info[i].width;
		XFree(info);
	} else
#endif
	{
		if (!XGetWindowAttributes(dpy, parentwin, &wa))
			die("could not get embedding window attributes: 0x%lx",
			    parentwin);
		x = 0;
		y = topbar ? pos_y : wa.height - mh - pos_y;
		mw = wa.width;
	}

	/* create menu window */
	swa.override_redirect = True;
	swa.background_pixel = scheme[SchemeNorm][ColBg].pixel;
	swa.border_pixel = 0;
	swa.colormap = cmap;
	swa.event_mask = ExposureMask | KeyPressMask | VisibilityChangeMask;
	win = XCreateWindow(dpy, parentwin, x, y, mw, mh, 0,
	                    depth, InputOutput, visual,
	                    CWOverrideRedirect | CWBackPixel | CWColormap |  CWEventMask | CWBorderPixel, &swa);
	XSetClassHint(dpy, win, &ch);

	/* open input methods */

	XMapRaised(dpy, win);
	if (embed) {
		XSelectInput(dpy, parentwin, FocusChangeMask);
		if (XQueryTree(dpy, parentwin, &dw, &w, &dws, &du) && dws) {
			for (i = 0; i < du && dws[i] != win; ++i)
				XSelectInput(dpy, dws[i], FocusChangeMask);
			XFree(dws);
		}
	}
	drw_resize(drw, mw, mh);
	drawmenu();
}

static void
usage(void)
{
	fputs("usage: progbar [-v] [-b] [-nb] [-m monitor] [-w windowid]\n"
	      "               [-bg color] [-be color] [-bf color]\n"
		  "               [-x x] [-y y] [-h height] [-w width]\n"
		  "               [-bt border] [-t duration] percentage\n", stderr);
	exit(1);
}

void
read_Xresources(void) {
	XrmInitialize();

	char* xrm;
	if ((xrm = XResourceManagerString(drw->dpy))) {
		char *type;
		XrmDatabase xdb = XrmGetStringDatabase(xrm);
		XrmValue xval;

		if (XrmGetResource(xdb, "progbar.font", "*", &type, &xval) == True) /* font or font set */
			fonts[0] = strdup(xval.addr);
		if (XrmGetResource(xdb, "progbar.color0", "*", &type, &xval) == True)  /* background color */
			colors[SchemeNorm][ColBg] = strdup(xval.addr);
		if (XrmGetResource(xdb, "progbar.color6", "*", &type, &xval) == True)  /* bar empty color */
			colors[SchemeSel][ColBg] = strdup(xval.addr);
		if (XrmGetResource(xdb, "progbar.color7", "*", &type, &xval) == True)  /* bar fill color */
			colors[SchemeOut][ColBg] = strdup(xval.addr);

		XrmDestroyDatabase(xdb);
	}
}

int
main(int argc, char *argv[])
{
	XWindowAttributes wa;
	char* end;
	int i = 0;

	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	if (!XSetLocaleModifiers(""))
		fputs("warning: no locale modifiers support\n", stderr);
	if (!(dpy = XOpenDisplay(NULL)))
		die("cannot open display");
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	if (!embed || !(parentwin = strtol(embed, NULL, 0)))
		parentwin = root;
	if (!XGetWindowAttributes(dpy, parentwin, &wa))
		die("could not get embedding window attributes: 0x%lx",
		    parentwin);
	xinitvisual();
	drw = drw_create(dpy, screen, root, wa.width, wa.height, visual, depth, cmap);
	read_Xresources();

	for (i = 1; i < argc; i++)
		/* these options take no arguments */
		if (!strcmp(argv[i], "-v")) {      /* prints version information */
			puts("progbar-"VERSION);
			exit(0);
		} else if (!strcmp(argv[i], "-b")) /* appears at the bottom of the screen */
			topbar = 0;
		else if (i + 1 == argc) {
			percent = strtol(argv[i], &end, 10);
			if(*end || percent>100 || percent<0)
				usage();
		}
		else if (!strcmp(argv[i], "-nb"))  /* no background */
			background = 0;
		/* these options take one argument */
		else if (!strcmp(argv[i], "-m"))
			mon = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-w"))   /* embedding window id */
			embed = argv[++i];
		else if (!strcmp(argv[i], "-bg"))  /* background/border color */
			colors[SchemeNorm][ColBg] = argv[++i];
		else if (!strcmp(argv[i], "-be"))  /* bar empty color */
			colors[SchemeOut][ColBg] = argv[++i];
		else if (!strcmp(argv[i], "-bf"))  /* bar fill color */
			colors[SchemeSel][ColBg] = argv[++i];
		else if (!strcmp(argv[i], "-x"))   /* bar horizontal position */
			pos_x = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-y"))   /* bar vertical position */
			pos_y = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-h"))   /* bar height */
			bh = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-w"))   /* bar width fraction */
			bwf = atof(argv[++i]);
		else if (!strcmp(argv[i], "-bt"))  /* bar border thickness */
			border = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-t"))   /* bar display duration */
			sleeptime = atof(argv[++i]);
	    else
			usage();


#ifdef __OpenBSD__
	if (pledge("stdio rpath", NULL) == -1)
		die("pledge");
#endif

	setup();
	struct timespec ts = { .tv_sec = (int) sleeptime, .tv_nsec = (long) ((sleeptime-((int) sleeptime)) * 1000000000) };
	nanosleep(&ts,NULL);
	cleanup();
	return 0;
}
