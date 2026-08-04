#include "vase/buffer/original.h"
#include "io/file.h"

OriginalBuffer::OriginalBuffer() {
  char tmp_path[] = "/tmp/tbuf.XXXXXX";
  fd = mkstemp(tmp_path);
  if (fd == -1)
    exit(1);
  unlink(tmp_path);
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

const char *OriginalBuffer::read(uint64_t pos, uint64_t *out_len) {
  if (pos >= len)
    return nullptr;
  if (out_len)
    *out_len = len - pos;
  return buf + pos;
}

uint64_t OriginalBuffer::length() {
  return len;
}
