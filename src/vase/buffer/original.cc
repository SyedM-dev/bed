#include "vase/buffer/original.h"
#include "io/file.h"

OriginalBuffer::OriginalBuffer(std::filesystem::path base_dir) {
  if (!std::filesystem::exists(base_dir) || !std::filesystem::is_directory(base_dir))
    throw std::runtime_error("Swap directory does not exist or is not a directory.");
  base_dir /= "tbuf.XXXXXX";
  auto s = base_dir.string();
  std::vector<char> mutable_path(s.begin(), s.end());
  mutable_path.push_back('\0');
  fd = mkstemp(mutable_path.data());
  if (fd == -1)
    throw std::runtime_error("mkstemp failed");
  unlink(mutable_path.data());
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
