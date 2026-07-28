#pragma once

#define PCRE2_CODE_UNIT_WIDTH 8
#define PCRE_WORKSPACE_SIZE 512

#include <mruby.h>
#include <mruby/array.h>
#include <mruby/compile.h>
#include <mruby/hash.h>
#include <mruby/irep.h>
#include <mruby/string.h>
#include <pcre2.h>
extern "C" {
#include <grapheme.h>
#include <unicode_width.h>
}
#include <algorithm>
#include <assert.h>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <immintrin.h>
#include <iostream>
#include <limits.h>
#include <map>
#include <mutex>
#include <optional>
#include <poll.h>
#include <queue>
#include <set>
#include <shared_mutex>
#include <signal.h>
#include <stack>
#include <string.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std::chrono_literals;
