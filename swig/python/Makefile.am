EXTRA_DIST = lv2_list.py

if WITH_PYTHON
all:
	swig -DPYTHON -Wall -python -I../.. -o lilv_python.c -oh lilv_python.h ../lilv.i
	gcc -fPIC -shared -I../.. $(PYTHON_CPPFLAGS) $(PYTHON_EXTRA_LDFLAGS) \
		$(PYTHON_EXTRA_LIBS) $(PYTHON_EXTRA_LDFLAGS) \
		lilv_python.c ../../src/.libs/liblilv.so -o _lilv.so

clean-local:
	rm -f *.[ch] *.so *.o lilv.py
endif
