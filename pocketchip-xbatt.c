/* 
 * A little program displaying battery level on PocketCHIP. 
 * Supposed to be swallowed by JWM.
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

static int read_battery_file(const char *name, int *value) {

	FILE *f = fopen(name, "r");
	if (!f)
		return -1;

	int result = fscanf(f, "%d", value) >= 1 ? 0 : -1;
	fclose(f);
	return result;
}

static Display *display;
static int x11_fd;
static int screen;
static Visual *visual;
static Colormap colormap;
static Window window;
static XftColor font_color;
static XftColor background_color;

static char *font_name = "Sans-8:bold";
static char *foreground_color_name = "#ffffff";
static char *background_color_name = "#ff007f";

static int draw_gauge(int x, int y, int width, int height, int percentage, int charging) {

	GC gc = XCreateGC(display, window, 0, NULL);
	if (!gc)
		return;

	XSetForeground(display, gc, font_color.pixel); 
	
	int w = width - 1;
	XDrawRectangle(display, window, gc, x + w, y + 2, 1, height - 4);

	XGCValues values;
	if (charging) {
		values.line_style = LineOnOffDash;
	} else {
		values.line_style = LineSolid;
	}
	XChangeGC(display, gc, GCLineStyle, &values);
	XSetDashes(display, gc, 0, (char[]){ 1, 1 }, 2);
	XDrawRectangle(display, window, gc, x, y, w, height);

	int filled = (percentage * (w - 1) + 50) / 100;
	XFillRectangle(display, window, gc, x + 1, y + 1, filled, height - 1);

	XFreeGC(display, gc);
}

static int draw() {

	int result = 1;

	XWindowAttributes attrs = {};
	XGetWindowAttributes(display, window, &attrs);

	XftFont *font = NULL;
	XftDraw *font_draw = NULL;

	do {

		font = XftFontOpenName(display, screen, font_name); 
		if (!font)
			break;

		font_draw = XftDrawCreate(display, window, visual, colormap);
		if (!font_draw) 
			break;
		
		char buf[256];	

		int gauge = 0;
		int voltage = 0;
		int charging = 0;
		if (read_battery_file("/usr/lib/pocketchip-batt/voltage", &voltage) == 0) {

			// I was using /usr/lib/pocketchip-batt/gauge previously which was set by pocketchip-one service, 
			// but then thought that making it compatible with the standard pocketchip-batt service would be a good idea.
			// The newer pocketchip-one will correct the voltage, so the level calculated this way will match the gauge.
			const int max_voltage = 4250;
            const int min_voltage = 3275;
			gauge = 100 * (voltage - min_voltage) / (max_voltage - min_voltage);
			if (gauge < 0)
				gauge = 0;
			if (gauge > 100)
				gauge = 100;

			sprintf(buf, "%d%%", gauge);

		} else {
			sprintf(buf, "?");
		}

		if (read_battery_file("/usr/lib/pocketchip-batt/charging", &charging) != 0) {
			charging = 0;
		}

		// TODO: use the actual window size here
		const int width = 70;
		const int height = 18; 

		XftDrawRect(font_draw, &background_color, 0, 0, width, height);	

		const int gauge_width = 17;
		const int gauge_height = 6;
		const int p = 6;
		draw_gauge(width - gauge_width - p, (height - gauge_height) / 2 - 1, gauge_width, gauge_height, gauge, charging);

		XGlyphInfo text_info;
		XftTextExtents8(display, font, buf, strlen(buf), &text_info);

		// TODO: calculate vertical offset
		XftDrawString8(font_draw, &font_color, font, (width - p - gauge_width - p - text_info.width), 12, buf, strlen(buf));

		XFlush(display);

		result = 0;

	} while (0);

	if (font)
		XftFontClose(display, font);

	if (font_draw)
		XftDrawDestroy(font_draw);

	return result;
}

static void usage() {
	printf(
		"\n"
		"Draws battery indicator on Pocket CHIP which can be swollowed by JWM's tray.\n"
		"\n"
		"Usage:\n"
		"\tpocketchip-xbatt [-fc foeground_color] [-bc background_color] [-font font]\n"
		"\n"
	);
}

int main(int argc, char **argv) {

	static struct option longopts[] = {
		{ "text", optional_argument, NULL, 't' },
		{ "background", optional_argument, NULL, 'b' },
		{ "font", optional_argument, NULL, 'f' },
		{ 0 }
	};

	int ch;
	while ((ch = getopt_long_only(argc, argv, "", longopts, NULL)) != -1) {
		switch (ch) {
			case 'f':
				font_name = optarg;
				break;
			case 't':
				foreground_color_name = optarg;
				break;
			case 'b':
				background_color_name = optarg;
				break;
			case 0:
				break;
			default:
				usage();
				return 0;
		}
	}
	
	display = XOpenDisplay(NULL);
	if (!display) {
		fprintf(stderr, "Could not open the display\n");
		return 1;
	}

	x11_fd = ConnectionNumber(display);

	screen = DefaultScreen(display);

	visual = DefaultVisual(display, screen); 
	colormap = DefaultColormap(display, screen); 
	
	XftColorAllocName(display, visual, colormap, foreground_color_name, &font_color);
	XftColorAllocName(display, visual, colormap, background_color_name, &background_color);

	window = XCreateSimpleWindow(
		display,
		DefaultRootWindow(display),
		0, 0, 50, 18,
		0, background_color.pixel, 
		background_color.pixel 
	);

	XClassHint *class_hint = XAllocClassHint();
	class_hint->res_name = "pocketchip-xbatt";
	class_hint->res_class = "pocketchip-xbatt";
	XSetClassHint(display, window, class_hint);
	
	XSelectInput(display, window, StructureNotifyMask | ExposureMask);
	
	XMapWindow(display, window);
	XFlush(display);

	while (1) {

		if (!XPending(display)) {	
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(x11_fd, &fds);

			struct timeval timeout = { 0 };
			timeout.tv_sec = 5;

			int num_ready = select(x11_fd + 1, &fds, NULL, NULL, &timeout);
			if (num_ready == 0) {
				draw();
				continue;
			}
		}

		while (XPending(display)) {
			XEvent e;
			XNextEvent(display, &e);
			switch (e.type) {
				case MapNotify:
					break;
				case Expose:
					if (e.xexpose.count == 0)
						draw();
					break;
			}
		}
	}

	return 0;
}

// vim: sw=4 ts=4
