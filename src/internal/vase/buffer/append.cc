#include "internal/vase/buffer/append.h"

AppendBuffer::AppendBuffer(std::filesystem::path base_dir) {
  base_dir /= "tapp.XXXXXX";
  char *s = strdup(base_dir.c_str());
  fd = mkstemp(s);
  if (fd == -1)
    throw std::runtime_error("mkstemp failed");
  unlink(s);
  free(s);
  allocated_capacity = 1ull << 30;
  if (ftruncate(fd, allocated_capacity) == -1)
    throw std::runtime_error("ftruncate failed");
  buf = (char *)mmap(nullptr, allocated_capacity, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (buf == MAP_FAILED)
    throw std::runtime_error("mmap failed");
}

AppendBuffer::~AppendBuffer() {
  if (buf && buf != MAP_FAILED)
    munmap(buf, allocated_capacity);
  if (fd != -1)
    close(fd);
}

void AppendBuffer::grow(uint64_t len) {
  if (current_size + len > allocated_capacity) {
    uint64_t new_capacity = allocated_capacity * 2;
    if (new_capacity < current_size + len)
      new_capacity = current_size + len + (1ull << 30);
    if (ftruncate(fd, new_capacity) == -1)
      throw std::runtime_error("ftruncate failed");
#if defined(__linux__)
    void *new_buf = mremap(buf, allocated_capacity, new_capacity, MREMAP_MAYMOVE);
    if (new_buf == MAP_FAILED)
      throw std::runtime_error("mremap failed");
#else
    munmap(buf, allocated_capacity);
    void *new_buf = mmap(nullptr, new_capacity, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (new_buf == MAP_FAILED)
      throw std::runtime_error("mmap failed");
#endif
    allocated_capacity = new_capacity;
    buf = (char *)new_buf;
  }
}

uint64_t AppendBuffer::append(const char c) {
  grow(1);
  buf[current_size++] = c;
  return current_size - 1;
}

uint64_t AppendBuffer::append(const char *text, uint64_t len) {
  grow(len);
  memcpy(buf + current_size, text, len);
  uint64_t old_pos = current_size;
  current_size += len;
  return old_pos;
}

const char *AppendBuffer::read(uint64_t pos) {
  if (pos >= current_size)
    return nullptr;
  return buf + pos;
}

uint64_t AppendBuffer::length() {
  return current_size;
}
