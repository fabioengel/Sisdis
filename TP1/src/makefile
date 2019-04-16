all: tp1_sisdis

tp1_sisdis: tp1_sisdis.o smpl.o rand.o cisj.o
	$(LINK.c) -o $@ -Bstatic tp1_sisdis.o cisj.o smpl.o rand.o -lm

cisj.o: cisj.c cisj.h
	$(COMPILE.c)  -g cisj.c

smpl.o: smpl.c smpl.h
	$(COMPILE.c)  -g smpl.c

nbndD12.o: tp1_sisdis.c smpl.h
	$(COMPILE.c) -g  tp1_sisdis.c

rand.o: rand.c
	$(COMPILE.c) -g rand.c

clean:
	$(RM) *.o tp1_sisdis relat saida