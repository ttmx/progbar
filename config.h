/* See LICENSE file for copyright and license details. */
/* Default settings; can be overriden by command line. */

static int topbar = 1;                      /* -b  option; if 0, progbar appears at bottom     */
/* -fn option overrides fonts[0]; default X11 font or font set */
static const char *fonts[] = {
	"Ubuntu-mono:size=12:antialias=true:autohint=true",
	"Ubuntu-mono:size=12",
	"FontAwesome5Free:pixelsize=12:antialias=true:autohint=true",
	"JoyPixels:pixelsize=15:antialias=true:autohint=true"
};
static const unsigned int bgalpha = 0xff;
static const unsigned int fgalpha = OPAQUE;
static const char *colors[SchemeLast][2] = {
	/*     fg         bg       */
	[SchemeNorm] = { "#e5e9f0", "#1b1e2b" },
	[SchemeSel] = { "#e5e9f0", "#5e81ac" },
	[SchemeOut] = { "#e5e9f0", "#8fbcbb" },
};
static const unsigned int alphas[SchemeLast][2] = {
	/*		fgalpha		bgalphga	*/
	[SchemeNorm] = { fgalpha, bgalpha },
	[SchemeSel] = { fgalpha, bgalpha },
	[SchemeOut] = { fgalpha, bgalpha },
};

/*
 * Characters not considered part of a word while deleting words
 * for example: " /?\"&[]"
 */
static const char worddelimiters[] = " ";

// Bar height
static int bh = 18;
// Width of the bar in fraction of screen width
static float bwf = 0.4;
// Border thickness
static int border = 2;
// Background
static int background = 1;
// Seconds to wait before disappearing
static float sleeptime = 2;
// Position of the bar in the horizontal direction
static int pos_x = 0;
// Position of the bar in the vertical direction
static int pos_y = 0;