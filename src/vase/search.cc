#include "vase/search.h"
#include "vase/iterators/chunk.h"

std::vector<RegexMatch> regex_search(
  Shard *root, std::string_view pattern_str,
  uint32_t start_offset, uint32_t end_offset,
  std::string_view options
) {
  bool global = false;
  uint32_t flags = PCRE2_MULTILINE;

  const char *dot = "(?:(?!\\n)\\X)";

  for (char c : options) {
    switch (c) {
    case 'g':
      global = true;
      break;
    case 'i':
      flags |= PCRE2_CASELESS;
      break;
    case 's':
      dot = "(?:\\X)";
      break;
    case 'x':
      flags |= PCRE2_EXTENDED;
      break;
    case 'U':
      flags |= PCRE2_UNGREEDY;
      break;
    case 'n':
      flags |= PCRE2_NO_AUTO_CAPTURE;
      break;
    case 'm':
      flags &= ~PCRE2_MULTILINE;
      break;
    }
  }

  std::vector<RegexMatch> results;

  int errornumber;
  PCRE2_SIZE erroroffset;

  std::string out;
  out.reserve(pattern_str.size() * 2);

  bool escaped = false;

  for (char c : pattern_str) {
    // TODO: also dont switch out in [.] style groups.
    if (escaped) {
      out += c;
      escaped = false;
      continue;
    }

    if (c == '\\') {
      out += c;
      escaped = true;
      continue;
    }

    if (c == '.') {
      out += dot;
      continue;
    }

    out += c;
  }

  pcre2_code *re = pcre2_compile(
    (PCRE2_SPTR)out.data(),
    out.size(),
    flags,
    &errornumber,
    &erroroffset,
    NULL
  );

  if (re == NULL)
    return results;

  pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, NULL);

  ChunkIterator it(root);
  it.seek_offset(start_offset);

  const char *data;
  uint32_t length;

  char buf[2048];

  uint32_t global_offset = start_offset;
  int64_t offset = -1;

  auto record_match = [&](int rc, PCRE2_SIZE *ovector) {
    if (global_offset + (uint32_t)ovector[1] > end_offset || ovector[0] == ovector[1])
      return;
    RegexMatch match{
      .start = global_offset + (uint32_t)ovector[0],
      .end = global_offset + (uint32_t)ovector[1]
    };
    for (int i = 1; i < rc && i <= 9; ++i) {
      PCRE2_SIZE s = ovector[2 * i];
      PCRE2_SIZE e = ovector[2 * i + 1];
      if (s != PCRE2_UNSET) {
        match.groups[i].start = global_offset + (uint32_t)s;
        match.groups[i].end = global_offset + (uint32_t)e;
      }
    }
    results.push_back(std::move(match));
  };

  auto skip_to_next_line = [&](const char *&data, uint32_t &length, int64_t &offset, uint32_t &global_offset) {
    while (true) {
      const char *p = (const char *)memchr(data + offset, '\n', length - offset);
      if (p) {
        uint32_t nl = p - data;
        offset = nl + 1;
        return;
      }
      global_offset += length;
      offset = -1;
      if (!it.next(&data, &length))
        return;
      offset = 0;
    }
  };

  while (global_offset < end_offset) {
    if (offset == -1) {
      bool more = it.next(&data, &length);
      if (!more)
        break;
      offset = 0;
    }

    while (offset < length) {
      if (offset == -1)
        break;

      int rc = pcre2_match(
        re,
        (PCRE2_SPTR)data,
        (size_t)length,
        offset,
        PCRE2_PARTIAL_HARD,
        match_data,
        NULL
      );

      if (rc == PCRE2_ERROR_PARTIAL) {
        uint32_t buflen = 0;

        PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);
        uint32_t partial_start = ovector[0];
        uint32_t partial_length = length - partial_start;

        if (partial_length >= 2048) {
          global_offset += offset;
          offset = -1;
          continue;
        }

        global_offset += partial_start;

        memcpy(buf, data + partial_start, partial_length);
        buflen += partial_length;

        offset = -1;
        bool exhausted = false;

        while (true) {
          if (buflen >= 2048)
            break;

          bool more = it.next(&data, &length);
          if (!more) {
            exhausted = true;
            break;
          }
          uint32_t l = std::min(length, 2048 - buflen);
          memcpy(buf + buflen, data, l);
          buflen += l;

          int rc = pcre2_match(
            re,
            (PCRE2_SPTR)buf,
            (size_t)buflen,
            0,
            PCRE2_PARTIAL_HARD,
            match_data,
            NULL
          );

          if (rc == PCRE2_ERROR_PARTIAL) {
            continue;
          } else if (rc > 0) {
            PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);
            record_match(rc, ovector);
            global_offset += buflen - l;
            offset = l - (buflen - ovector[1]);
            if (!global)
              skip_to_next_line(data, length, offset, global_offset);
            break;
          } else if (rc == PCRE2_ERROR_NOMATCH) {
            global_offset += buflen - l;
            offset = 0;
            break;
          } else {
            break;
          }
        }

        if (exhausted) {
          uint32_t search_from = 0;
          while (search_from <= buflen) {
            int rc = pcre2_match(
              re,
              (PCRE2_SPTR)buf,
              (size_t)buflen,
              search_from,
              0,
              match_data,
              NULL
            );
            if (rc <= 0)
              break;
            PCRE2_SIZE *ov = pcre2_get_ovector_pointer(match_data);
            record_match(rc, ov);
            if (global) {
              search_from = (ov[1] == ov[0]) ? (uint32_t)ov[1] + 1 : (uint32_t)ov[1];
            } else {
              const void *p = memchr(buf + ov[1], '\n', buflen - ov[1]);
              if (!p)
                break;
              search_from = (const char *)p - buf + 1;
            }
          }
          offset = -1;
        }

        continue;
      } else if (rc > 0) {
        PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);
        record_match(rc, ovector);
        if (global) {
          if ((int64_t)ovector[1] == offset)
            offset++;
          else
            offset = ovector[1];
        } else {
          offset = ovector[1];
          skip_to_next_line(data, length, offset, global_offset);
        }
      } else if (rc == PCRE2_ERROR_NOMATCH) {
        offset = length;
        break;
      } else {
        offset = length;
        break;
      }
    }

    global_offset += offset;
    offset = -1;
  }

  pcre2_match_data_free(match_data);
  pcre2_code_free(re);

  return results;
}
