#=======================================
# global makefile stub
#=======================================

RELEASE=$(shell date +%Y%m%d)

all:
	@cd amigaos;make
	@cd aros;make

clean:
	@cd amigaos;make clean
	@cd aros;make clean

install: all 
	@(cd ..;make)
	@echo "=============== building package $(RELEASE) ==============="
	@mkdir -p ~/releases/$(RELEASE)
	@mkdir -p ~/releases/$(RELEASE)/janus-uae
	@mkdir -p ~/releases/$(RELEASE)/janus-uae/68k-AmigaOS
	@mkdir -p ~/releases/$(RELEASE)/janus-uae/68k-AROS
	@mkdir -p ~/releases/$(RELEASE)/janus-uae/docs
	@mkdir -p ~/releases/$(RELEASE)/janus-uae/docs/manual
	cp amigaos/janusd              ~/releases/$(RELEASE)/janus-uae/68k-AmigaOS/
	cp amigaos/clipd               ~/releases/$(RELEASE)/janus-uae/68k-AmigaOS/
	cp amigaos/clonescreenmode     ~/releases/$(RELEASE)/janus-uae/68k-AmigaOS/
	cp amigaos/launchd             ~/releases/$(RELEASE)/janus-uae/68k-AmigaOS/
	cp amigaos/janus-splash        ~/releases/$(RELEASE)/janus-uae/68k-AmigaOS/
	cp var/rtg.library             ~/releases/$(RELEASE)/janus-uae/68k-AmigaOS/
	cp var/transdisk               ~/releases/$(RELEASE)/janus-uae/68k-AmigaOS/
	cp var/transrom                ~/releases/$(RELEASE)/janus-uae/68k-AmigaOS/
	cp var/rtg.library             ~/releases/$(RELEASE)/janus-uae/68k-AmigaOS/
	cp aros/janusd                 ~/releases/$(RELEASE)/janus-uae/68k-AROS/
	cp amigaos/clipd               ~/releases/$(RELEASE)/janus-uae/68k-AROS/
	cp amigaos/clonescreenmode     ~/releases/$(RELEASE)/janus-uae/68k-AROS/
	cp amigaos/launchd             ~/releases/$(RELEASE)/janus-uae/68k-AROS/
	cp amigaos/janus-splash        ~/releases/$(RELEASE)/janus-uae/68k-AROS/
	cp var/transdisk               ~/releases/$(RELEASE)/janus-uae/68k-AROS/
	@chmod 755 ~/releases/$(RELEASE)/janus-uae/68k-AROS/* ~/releases/$(RELEASE)/janus-uae/68k-AmigaOS/*
	cp ../docs/C*                  ~/releases/$(RELEASE)/janus-uae/docs/
	cp ../docs/Readme.txt          ~/releases/$(RELEASE)/janus-uae/docs/
	cp ../docs/manual/*            ~/releases/$(RELEASE)/janus-uae/docs/manual/
	cp ../src/uae                  ~/releases/$(RELEASE)/janus-uae/janus-uae
	@echo "=============== building tar $(RELEASE) ==============="
	cd ~/releases/$(RELEASE);tar zcf janus-uae.i386.tar.gz janus-uae

