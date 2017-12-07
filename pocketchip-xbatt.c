// vim: sw=4 ts=4

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>

static int read_battery_file(const char *name, int *value) {

	FILE *f = fopen(name, "r");
	if (!f) {
		return -1;
	}

	int result = fscanf(f, "%d", value) >= 1 ? 0 : -1;

	fclose(f);

	return result;
}

static Display *display;
static int screen;
static Visual *visual;
static Colormap colormap;
static Window window;
static XftColor font_color;
static XftColor background_color;
static XColor background_xcolor;

static int draw_gauge(int x, int y, int width, int height, int percentage) {

	GC gc = XCreateGC(display, window, 0, NULL);
	if (!gc)
		return;

	XSetForeground(display, gc, font_color.pixel); 
	
	int w = width - 1;
	XDrawRectangle(display, window, gc, x, y, w, height);
	
	int filled = (percentage * (w - 1) + 50) / 100;
	XFillRectangle(display, window, gc, x + 1, y, filled, height);
	XFillRectangle(display, window, gc, x + width, y + 2, 1, height - 3);

	XFreeGC(display, gc);
}

static int draw() {

	int result = 1;

	XWindowAttributes attrs = {};
	XGetWindowAttributes(display, window, &attrs);

	XftFont *font = NULL;
	XftDraw *font_draw = NULL;
	do {

		font = XftFontOpenName(display, screen, "Sans-8:bold");
		if (!font)
			break;

		font_draw = XftDrawCreate(display, window, visual, colormap);
		if (!font_draw) 
			break;
		
		char buf[256];	

		int gauge = 0;
		int voltage = 0;
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

		const int width = 70;
		const int height = 18; 

		XftDrawRect(font_draw, &background_color, 0, 0, width, height);	

		const int gauge_width = 17;
		const int gauge_height = 6;
		const int p = 6;
		draw_gauge(width - gauge_width - p, (height - gauge_height) / 2 - 1, gauge_width, gauge_height, gauge);

		XGlyphInfo text_info;
		XftTextExtents8(display, font, buf, strlen(buf), &text_info);

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

int main(int argc, char **argv) {
	
	display = XOpenDisplay(NULL);
	if (!display) {
		fprintf(stderr, "Could not open the display\n");
		return 1;
	}

	screen = DefaultScreen(display);

	visual = DefaultVisual(display, screen); 
	colormap = DefaultColormap(display, screen); 
	
	XAllocNamedColor(display, colormap, "#ff007f", &background_xcolor, &background_xcolor);
	XftColorAllocName(display, visual, colormap, "#ffffff", &font_color);
	XftColorAllocName(display, visual, colormap, "#ff007f", &background_color);

	window = XCreateSimpleWindow(
		display,
		DefaultRootWindow(display),
		0, 0, 50, 18,
		0, background_xcolor.pixel, 
		background_xcolor.pixel 
	);

	XClassHint *class_hint = XAllocClassHint();
	class_hint->res_name = "pocketchip-xbatt";
	class_hint->res_class = "pocketchip-xbatt";
	XSetClassHint(display, window, class_hint);
	
	XSelectInput(display, window, StructureNotifyMask | ExposureMask);
	
	XMapWindow(display, window);

	while (1) {
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
		sleep(3);
	}

	return 0;
}

