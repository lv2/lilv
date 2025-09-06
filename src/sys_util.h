// Copyright 2007-2025 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef LILV_SYS_UTIL_H
#define LILV_SYS_UTIL_H

#include <zix/attributes.h>

#include <stdbool.h>

typedef bool (*LilvPathExistsFunc)(const char* ZIX_NONNULL     path,
                                   const void* ZIX_UNSPECIFIED handle);

/// Get the normalized LANG from the environment
char* ZIX_UNSPECIFIED
lilv_get_lang(void);

/// Return a newly allocated normalized LANG value
char* ZIX_ALLOCATED
lilv_normalize_lang(const char* ZIX_NONNULL env_lang);

/// Find an available path by appending a numeric suffix if necessary
char* ZIX_UNSPECIFIED
lilv_find_free_path(const char* ZIX_NONNULL        in_path,
                    LilvPathExistsFunc ZIX_NONNULL exists,
                    const void* ZIX_UNSPECIFIED    user_data);

/// Return the latest copy of the file at `path` that is newer
char* ZIX_UNSPECIFIED
lilv_get_latest_copy(const char* ZIX_NONNULL path,
                     const char* ZIX_NONNULL copy_path);

#endif /* LILV_SYS_UTIL_H */
