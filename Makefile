
pocketchip-xbatt: pocketchip-xbatt.c
	gcc $< -lX11 -lXext -lXft -I /usr/include/freetype2 -o $@

install: pocketchip-xbatt .jwmrc
	cp -f ./pocketchip-xbatt /usr/bin/pocketchip-xbatt
	cp .jwmrc .dmrc ~/
	sudo -u chip jwm -display :0 -restart
