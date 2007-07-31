EXTRA_DIST = slv2.i lv2_list.py

if WITH_SWIG
all:
	swig -Wall -python slv2.i
	gcc -fPIC -shared -I/usr/include/python2.4 slv2_wrap.c ../src/.libs/libslv2.so -o _slv2.so
endif

clean-local:
	rm -f *.cxx
	rm -f *.so
	rm -f *.o
	rm -f slv2.py
