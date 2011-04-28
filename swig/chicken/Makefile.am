EXTRA_DIST = lilv.setup lv2_list.scm

if WITH_CHICKEN
all:
	rm -f lilv.scm lilv_wrap.c
	swig -DCHICKEN -Wall -chicken -proxy -nounit -I../.. -o lilv_wrap.c -oh lilv_wrap.h ../lilv.i
	tar -czf lilv.egg lilv.setup lilv.scm lilv_wrap.c

#install-exec-local:
#	chicken-setup ./lilv.egg

clean-local:
	rm -f *.[ch] *.so *.o lilv.egg lilv.scm
endif

