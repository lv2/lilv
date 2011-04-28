if WITH_MZSCHEME
all:
	swig -DMZSCHEME -Wall -mzscheme -I../.. -o lilv_mzscheme.c -oh lilv_mzscheme.h ../lilv.i
	gcc -fPIC -shared -Iplt -I../.. \
		-I/usr/include/plt lilv_mzscheme.c -o liblilv_mzscheme.so

clean-local:
	rm -f *.[ch] *.so *.o
endif

