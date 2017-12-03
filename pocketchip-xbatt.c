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
static Window w;

static int draw() {

	int result = 1;

	XftFont *font = NULL;
	XftDraw *font_draw = NULL;
	do {

		GC gc = XCreateGC(display, w, 0, NULL);

		font = XftFontOpenName(display, screen, "Sans-8");
		if (!font)
			break;

		font_draw = XftDrawCreate(display, w, visual, colormap);
		if (!font_draw) 
			break;
		
		XftColor font_color;
		XftColorAllocName(display, visual, colormap, "#ffffff", &font_color);

		XftColor background_color;
		XftColorAllocName(display, visual, colormap, "#ff007f", &background_color);

		char buf[256];	

		int gauge = 0;
		if (read_battery_file("/usr/lib/pocketchip-batt/gauge", &gauge) == 0) {
			sprintf(buf, "%d%%", gauge);
		} else {
			sprintf(buf, "?");
		}

		const int width = 50;
		const int height = 18;

		XftDrawRect(font_draw, &background_color, 0, 0, width, height);	

		XGlyphInfo text_info;
		XftTextExtents8(display, font, buf, strlen(buf), &text_info);

		XftDrawString8(font_draw, &font_color, font, (width - text_info.width) / 2, 12, buf, strlen(buf));

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
	
	XColor background_xcolor;
	XAllocNamedColor(display, colormap, "#ff007f", &background_xcolor, &background_xcolor);

	w = XCreateSimpleWindow(
		display,
		DefaultRootWindow(display),
		0, 0, 50, 18,
		0, background_xcolor.pixel, 
		background_xcolor.pixel 
	);

	XClassHint *class_hint = XAllocClassHint();
	class_hint->res_name = "pocketchip-xbatt";
	class_hint->res_class = "pocketchip-xbatt";
	XSetClassHint(display, w, class_hint);
	
	XSelectInput(display, w, StructureNotifyMask | ExposureMask);
	
	XMapWindow(display, w);

	while (1) {
		while (XPending(display)) {
			XEvent e;
			XNextEvent(display, &e);
			switch (e.type) {
				case MapNotify:
					break;
				case Expose:
					draw();
					break;
			}
		}
		sleep(3);
	}

	return 0;
}

