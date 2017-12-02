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

int main(int argc, char **argv) {
	
	Display *display = XOpenDisplay(NULL);
	if (!display) {
		fprintf(stderr, "Could not open the display\n");
		return 1;
	}

	int screen = DefaultScreen(display);

	Visual *visual = DefaultVisual(display, screen); 
	Colormap colormap = DefaultColormap(display, screen); 
	
	XColor background_xcolor;
	XAllocNamedColor(display, colormap, "#ff007f", &background_xcolor, &background_xcolor);

	Window w = XCreateSimpleWindow(
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
	
	XSelectInput(display, w, StructureNotifyMask);
	XMapWindow(display, w);

	while (1) {
		XEvent e;
		XNextEvent(display, &e);
		if (e.type == MapNotify)
			break;
	}

	GC gc = XCreateGC(display, w, 0, NULL);

	XftFont *font = XftFontOpenName(display, screen, "Sans-8");
	if (!font) {
		return 2;
	}

	XftDraw *font_draw = XftDrawCreate(display, w, visual, colormap);
	if (!font_draw) {
		return 3;
	}
	
	XftColor font_color;
	XftColorAllocName(display, visual, colormap, "#ffffff", &font_color);

	XftColor background_color;
	XftColorAllocName(display, visual, colormap, "#ff007f", &background_color);

	char buf[256];	
	while (1) {

		int gauge = 0;
		if (read_battery_file("/usr/lib/pocketchip-batt/gauge", &gauge) == 0) {
			/*~
			// Just for comparison, let's calculate the level using voltages like pocket-home does it.
			const int max_voltage = 4250;
			const int min_voltage = 3275;
			int voltage_gauge = (voltage - min_voltage) * 100 / (max_voltage - min_voltage);
			*/
			sprintf(buf, "%d%%", gauge);
		} else {
			sprintf(buf, "?");
		}

		XftDrawRect(font_draw, &background_color, 0, 0, 50, 18);		
		XftDrawString8(font_draw, &font_color, font, 0, 12, buf, strlen(buf));

		XFlush(display);

		sleep(5);
	}
}

