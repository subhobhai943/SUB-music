#ifndef YOUTUBE_H
#define YOUTUBE_H

#include <stdbool.h>
#include <stddef.h>

bool youtube_is_url(const char *input);
bool youtube_resolve(const char *input, bool is_search, char *resolved_url,
                     size_t url_sz, char *title, size_t title_sz,
                     char *error, size_t error_sz);

#endif
