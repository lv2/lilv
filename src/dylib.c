// Copyright 2020-2024 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "dylib.h"

#ifdef _WIN32

#  include <windows.h>

void*
dylib_open(const char* const filename, const unsigned flags)
{
  (void)flags;
  return LoadLibrary(filename);
}

int
dylib_close(DylibLib* const handle)
{
  return !FreeLibrary((HMODULE)handle);
}

const char*
dylib_error(void)
{
  return "Unknown error";
}

DylibFunc
dylib_func(DylibLib* handle, const char* symbol)
{
  return (DylibFunc)GetProcAddress((HMODULE)handle, symbol);
}

#else

#  include <dlfcn.h>

void*
dylib_open(const char* const filename, const unsigned flags)
{
  return dlopen(filename, flags == DYLIB_LAZY ? RTLD_LAZY : RTLD_NOW);
}

int
dylib_close(DylibLib* const handle)
{
  return dlclose(handle);
}

const char*
dylib_error(void)
{
  return dlerror();
}

DylibFunc
dylib_func(DylibLib* handle, const char* symbol)
{
  typedef DylibFunc (*VoidFuncGetter)(void*, const char*);

  VoidFuncGetter dlfunc = (VoidFuncGetter)dlsym;

  return dlfunc(handle, symbol);
}

#endif
