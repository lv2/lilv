if WITH_MZSCHEME
all:
	swig -DMZSCHEME -Wall -mzscheme -I../.. -o slv2_mzscheme.c -oh slv2_mzscheme.h ../slv2.i
	gcc -fPIC -shared -Iplt -I../.. \
		-I/usr/include/plt slv2_mzscheme.c -o libslv2_mzscheme.so

clean-local:
	rm -f *.[ch] *.so *.o
endif

