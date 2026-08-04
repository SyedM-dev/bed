#include "vase/vase.h"

std::vector<RegexMatch> Vase::_regex_search(
  std::string_view pattern, Range range, std::string_view options
) {
  bool global = false;
  uint64_t flags = PCRE2_MULTILINE;

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
  out.reserve(pattern.size() * 2);
  bool escaped = false;
  bool in_class = false;
  bool in_quote = false;
  for (size_t i = 0; i < pattern.size(); i++) {
    char c = pattern[i];
    if (escaped) {
      out += c;
      escaped = false;
      continue;
    }
    if (c == '\\') {
      out += c;
      if (i + 1 < pattern.size()) {
        char next = pattern[i + 1];
        if (next == 'Q')
          in_quote = true;
        else if (next == 'E')
          in_quote = false;
        out += next;
        i++;
        continue;
      }
      escaped = true;
      continue;
    }
    if (in_quote) {
      out += c;
      continue;
    }
    if (c == '[') {
      in_class = true;
      out += c;
      continue;
    }
    if (c == ']' && in_class) {
      in_class = false;
      out += c;
      continue;
    }
    if (c == '.' && !in_class) {
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

  uint64_t start_offset = offset_of(range.start);
  uint64_t end_offset = offset_of(range.end);

  ChunkIterator it(root, Direction::Forward);
  it.seek_offset(start_offset);

  const char *data;
  uint64_t length;

  char buf[2048];

  uint64_t global_offset = start_offset;
  uint64_t offset = UINT64_MAX;

  auto record_match = [&](int rc, PCRE2_SIZE *ovector) {
    if (global_offset + (uint64_t)ovector[1] > end_offset || ovector[0] == ovector[1])
      return;
    RegexMatch match{
      .start = global_offset + (uint64_t)ovector[0],
      .end = global_offset + (uint64_t)ovector[1]
    };
    for (int i = 1; i < rc && i <= 9; ++i) {
      PCRE2_SIZE s = ovector[2 * i];
      PCRE2_SIZE e = ovector[2 * i + 1];
      if (s != PCRE2_UNSET) {
        match.groups[i].start = global_offset + (uint64_t)s;
        match.groups[i].end = global_offset + (uint64_t)e;
      }
    }
    results.push_back(std::move(match));
  };

  auto skip_to_next_line = [&](const char *&data, uint64_t &length, uint64_t &offset, uint64_t &global_offset) {
    while (true) {
      const char *p = (const char *)memchr(data + offset, '\n', length - offset);
      if (p) {
        uint64_t nl = p - data;
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
    if (offset == UINT64_MAX) {
      bool more = it.next(&data, &length);
      if (!more)
        break;
      offset = 0;
    }

    while (offset < length) {
      if (offset == UINT64_MAX)
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
        uint64_t buflen = 0;

        PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);
        uint64_t partial_start = ovector[0];
        uint64_t partial_length = length - partial_start;

        if (partial_length >= 2048) {
          global_offset += offset;
          offset = UINT64_MAX;
          continue;
        }

        global_offset += partial_start;

        memcpy(buf, data + partial_start, partial_length);
        buflen += partial_length;

        offset = UINT64_MAX;
        bool exhausted = false;

        while (true) {
          if (buflen >= 2048)
            break;

          bool more = it.next(&data, &length);
          if (!more) {
            exhausted = true;
            break;
          }
          uint64_t l = std::min(length, 2048 - buflen);
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
          uint64_t search_from = 0;
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
              search_from = (ov[1] == ov[0]) ? (uint64_t)ov[1] + 1 : (uint64_t)ov[1];
            } else {
              const void *p = memchr(buf + ov[1], '\n', buflen - ov[1]);
              if (!p)
                break;
              search_from = (const char *)p - buf + 1;
            }
          }
          offset = UINT64_MAX;
        }

        continue;
      } else if (rc > 0) {
        PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);
        record_match(rc, ovector);
        if (global) {
          if (ovector[1] == offset)
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
    offset = UINT64_MAX;
  }

  pcre2_match_data_free(match_data);
  pcre2_code_free(re);

  return results;
}
