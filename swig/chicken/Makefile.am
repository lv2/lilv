EXTRA_DIST = slv2.setup lv2_list.scm

if WITH_CHICKEN
all:
	rm -f slv2.scm slv2_wrap.c
	swig -DCHICKEN -Wall -chicken -proxy -nounit -I../.. -o slv2_wrap.c -oh slv2_wrap.h ../slv2.i
	tar -czf slv2.egg slv2.setup slv2.scm slv2_wrap.c

#install-exec-local:
#	chicken-setup ./slv2.egg

clean-local:
	rm -f *.[ch] *.so *.o slv2.egg slv2.scm
endif

