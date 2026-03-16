#include "youtube.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void shell_escape_single_quotes(const char *src, char *dst, size_t dst_sz) {
  size_t j = 0;
  for (size_t i = 0; src[i] && j + 1 < dst_sz; ++i) {
    if (src[i] == '\'') {
      const char *esc = "'\\''";
      for (size_t k = 0; esc[k] && j + 1 < dst_sz; ++k) dst[j++] = esc[k];
    } else {
      dst[j++] = src[i];
    }
  }
  dst[j] = '\0';
}

bool youtube_is_url(const char *input) {
  return strstr(input, "http://") == input || strstr(input, "https://") == input;
}

bool youtube_resolve(const char *input, bool is_search, char *resolved_url,
                     size_t url_sz, char *title, size_t title_sz,
                     char *error, size_t error_sz) {
  char escaped[2048] = {0};
  char query[2200] = {0};
  char cmd[4096] = {0};

  shell_escape_single_quotes(input, escaped, sizeof(escaped));

  if (is_search) {
    snprintf(query, sizeof(query), "ytsearch1:%s", escaped);
  } else {
    snprintf(query, sizeof(query), "%s", escaped);
  }

  snprintf(cmd, sizeof(cmd),
           "yt-dlp --no-playlist --get-url --get-title '%s' 2>/dev/null", query);

  FILE *fp = popen(cmd, "r");
  if (!fp) {
    snprintf(error, error_sz, "Failed to start yt-dlp");
    return false;
  }

  char url_line[2048] = {0};
  char title_line[1024] = {0};

  if (!fgets(url_line, sizeof(url_line), fp) || !fgets(title_line, sizeof(title_line), fp)) {
    pclose(fp);
    snprintf(error, error_sz, "No playable YouTube result");
    return false;
  }

  pclose(fp);

  url_line[strcspn(url_line, "\r\n")] = '\0';
  title_line[strcspn(title_line, "\r\n")] = '\0';

  if (url_line[0] == '\0') {
    snprintf(error, error_sz, "yt-dlp returned empty audio URL");
    return false;
  }

  snprintf(resolved_url, url_sz, "%s", url_line);
  snprintf(title, title_sz, "%s", title_line[0] ? title_line : "Unknown title");

  return true;
}
