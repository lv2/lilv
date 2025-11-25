// Copyright 2021-2025 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef LILV_CONFIG_H
#define LILV_CONFIG_H

// Define version unconditionally so a warning will catch a mismatch
#define LILV_VERSION "0.26.2"

// Separator between entries in variables like PATH
#ifndef LILV_PATH_SEP
#  ifdef _WIN32
#    define LILV_PATH_SEP ";"
#  else
#    define LILV_PATH_SEP ":"
#  endif
#endif

// Default value for LV2_PATH environment variable
#ifndef LILV_DEFAULT_LV2_PATH
#  if defined(__APPLE__)
#    define LILV_DEFAULT_LV2_PATH            \
      "~/.lv2:~/Library/Audio/Plug-Ins/LV2:" \
      "/usr/local/lib/lv2:/usr/lib/lv2:"     \
      "/Library/Audio/Plug-Ins/LV2"
#  elif defined(_WIN32)
#    define LILV_DEFAULT_LV2_PATH "%APPDATA%\\LV2;%COMMONPROGRAMFILES%\\LV2"
#  else
#    define LILV_DEFAULT_LV2_PATH "~/.lv2:/usr/local/lib/lv2:/usr/lib/lv2"
#  endif
#endif

#endif // LILV_CONFIG_H
