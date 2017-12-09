
pocketchip-xbatt: pocketchip-xbatt.c
	gcc $< -lX11 -lXext -lXft -I /usr/include/freetype2 -o $@

install: pocketchip-xbatt .jwmrc
	cp -f ./pocketchip-xbatt /usr/bin/pocketchip-xbatt
	cp .jwmrc /home/chip/
	cp .dmrc /home/chip/
	sudo -u chip jwm -display :0 -restart
