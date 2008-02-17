EXTRA_DIST = slv2.i lv2_list.py

if WITH_SWIG

if WITH_PYTHON
all:
	swig -Wall -python slv2.i
	gcc -fPIC -shared $(PYTHON_CPPFLAGS) $(PYTHON_EXTRA_LDFLAGS) \
		$(PYTHON_EXTRA_LIBS) $(PYTHON_EXTRA_LDFLAGS) \
		-I/usr/include/python2.4 slv2_wrap.c ../src/.libs/libslv2.so -o _slv2.so
endif # WITH_PYTHON

endif # WITH_SWIG

clean-local:
	rm -f *.cxx
	rm -f *.so
	rm -f *.o
	rm -f slv2.py
