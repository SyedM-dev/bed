#include "internal/vase/vase.h"

namespace crib::internal::vase {
OriginalBuffer::OriginalBuffer(std::filesystem::path base_dir) {
  if (!std::filesystem::exists(base_dir) || !std::filesystem::is_directory(base_dir))
    throw std::runtime_error("Swap directory does not exist or is not a directory.");
  base_dir /= "tbuf.XXXXXX";
  char *s = strdup(base_dir.c_str());
  fd = mkstemp(s);
  if (fd == -1)
    throw std::runtime_error("mkstemp failed");
  unlink(s);
  free(s);
}

OriginalBuffer::~OriginalBuffer() {
  if (buf)
    munmap((char *)buf, len);
  if (fd != -1)
    close(fd);
}

void OriginalBuffer::initialize() {
  struct stat st;
  if (fstat(fd, &st) == -1)
    throw std::runtime_error("fstat failed");
  len = (uint64_t)st.st_size;
  buf = (const char *)mmap(nullptr, len, PROT_READ, MAP_PRIVATE, fd, 0);
  if (buf == MAP_FAILED) {
    buf = nullptr;
    throw std::runtime_error("mmap failed");
  }
  madvise((void *)buf, len, MADV_RANDOM);
  close(fd);
  fd = -1;
}

const char *OriginalBuffer::read(uint64_t pos) {
  if (pos >= len)
    return nullptr;
  return buf + pos;
}

uint64_t OriginalBuffer::length() {
  return len;
}
} // namespace crib::internal::vase
