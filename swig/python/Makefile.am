EXTRA_DIST = lv2_list.py

if WITH_PYTHON
all:
	swig -DPYTHON -Wall -python -I../.. -o slv2_python.c -oh slv2_python.h ../slv2.i
	gcc -fPIC -shared -I../.. $(PYTHON_CPPFLAGS) $(PYTHON_EXTRA_LDFLAGS) \
		$(PYTHON_EXTRA_LIBS) $(PYTHON_EXTRA_LDFLAGS) \
		slv2_python.c ../../src/.libs/libslv2.so -o _slv2.so

clean-local:
	rm -f *.[ch] *.so *.o slv2.py
endif
