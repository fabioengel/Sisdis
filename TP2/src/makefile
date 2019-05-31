all: tp2_sisdis

tp2_sisdis: tp2_sisdis.o smpl.o rand.o cisj.o eventList.o
	$(LINK.c) -o $@ -Bstatic tp2_sisdis.o cisj.o smpl.o rand.o eventList.o -lm

cisj.o: cisj.c cisj.h
	$(COMPILE.c)  -g cisj.c

smpl.o: smpl.c smpl.h
	$(COMPILE.c)  -g smpl.c

sisdis.o: tp2_sisdis.c smpl.h
	$(COMPILE.c) -g  tp2_sisdis.c


eventList.o: eventList.c eventList.h
	$(COMPILE.c)  -g eventList.c	

rand.o: rand.c
	$(COMPILE.c) -g rand.c

clean:
	$(RM) *.o tp2_sisdis relat saida