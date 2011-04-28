EXTRA_DIST = lilv.setup lv2_list.scm

if WITH_CHICKEN
all:
	rm -f lilv.scm slv2_wrap.c
	swig -DCHICKEN -Wall -chicken -proxy -nounit -I../.. -o lilv_wrap.c -oh slv2_wrap.h ../slv2.i
	tar -czf lilv.egg slv2.setup slv2.scm slv2_wrap.c

#install-exec-local:
#	chicken-setup ./lilv.egg

clean-local:
	rm -f *.[ch] *.so *.o lilv.egg slv2.scm
endif

