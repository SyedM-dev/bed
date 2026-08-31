#include "internal/io/io.h"

namespace bed::internal::io {
termios IO::orig_termios{};
termios IO::raw_termios{};
bool IO::cleaned = true;
volatile std::atomic_bool IO::resized(false);

IO::IO() {
  if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
    throw fatal_error("Can't get terminal state.", 1);
  struct sigaction sa{};
  sa.sa_handler = handle_sigwinch;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(SIGWINCH, &sa, nullptr) == -1)
    throw fatal_error("Can't install SIGWINCH handler.", 1);
  raw_termios = orig_termios;
  raw_termios.c_iflag &= ~(BRKINT | ISTRIP | IXON);
  raw_termios.c_cflag |= (CS8);
  raw_termios.c_lflag &= ~(ECHO | ICANON | ISIG);
  raw_termios.c_cc[VMIN] = 1;
  raw_termios.c_cc[VTIME] = 0;
  enable_raw();
  atexit(cleanup);
}

IO::~IO() {
  cleanup();
}

void IO::enable_mouse() {
  const char *seq = "\x1b[?1000h";
  write_all(STDOUT_FILENO, seq, 8);
}

void IO::disable_mouse() {
  const char *seq = "\x1b[?1000l";
  write_all(STDOUT_FILENO, seq, 8);
}

std::pair<uint16_t, uint16_t> IO::terminal_size() {
  struct winsize ws{};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
    throw fatal_error("Can't get terminal size.", 1);
  return {ws.ws_row, ws.ws_col};
}

std::pair<uint16_t, uint16_t> IO::cursor_position() {
  write_all(STDOUT_FILENO, "\x1b[6n", 4);
  std::string response;
  char c;
  if (read(STDIN_FILENO, &c, 1) != 1 || c != '\x1b')
    throw fatal_error("Invalid cursor position response.", 1);
  if (read(STDIN_FILENO, &c, 1) != 1 || c != '[')
    throw fatal_error("Invalid cursor position response.", 1);
  while (true) {
    if (read(STDIN_FILENO, &c, 1) != 1)
      throw fatal_error("Invalid cursor position response.", 1);
    if (c == 'R')
      break;
    response += c;
  }
  unsigned row;
  unsigned col;
  if (sscanf(response.c_str(), "%u;%u", &row, &col) != 2)
    throw fatal_error("Invalid cursor position response.", 1);
  return {(uint16_t)row, (uint16_t)col};
}

void IO::enable_raw() {
  if (!cleaned)
    return;
  std::string os = "\x1b[?2004h";
  write_all(STDOUT_FILENO, os.c_str(), os.size());
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_termios) == -1)
    throw fatal_error("Can't set raw terminal state.", 1);
  cleaned = false;
}

void IO::cleanup() {
  if (cleaned)
    return;
  std::string os = "\x1b[?1000l\x1b[?2004l";
  write_all(STDOUT_FILENO, os.c_str(), os.size());
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
    perror("Can't clean up terminal.");
  cleaned = true;
}

void IO::handle_sigwinch(int) {
  resized.store(true);
}

void IO::move_cursor(uint16_t row, uint16_t col) {
  char buf[32];
  int n = snprintf(buf, sizeof(buf), "\x1b[%u;%uH", row, col);
  write_all(STDOUT_FILENO, buf, n);
}

void IO::write(const char *buf, uint64_t n) {
  write_all(STDOUT_FILENO, buf, n);
}

void IO::write(std::string_view s) {
  write_all(STDOUT_FILENO, s.data(), s.size());
}

void IO::run_pty(const std::string &cmd) {
  int master_fd = -1;
  struct winsize ws{};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
    throw fatal_error("Can't get terminal size.", 1);
  pid_t pid = forkpty(
    &master_fd,
    nullptr,
    &orig_termios,
    &ws
  );
  if (pid == -1)
    throw fatal_error("Can't create PTY.", 1);
  if (pid == 0) {
    execl("/bin/sh", "sh", "-c", cmd.c_str(), (char *)nullptr);
    _exit(127);
  }
  struct pollfd fds[2];
  while (true) {
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = master_fd;
    fds[1].events = POLLIN;
    int rc = poll(fds, 2, -1);
    if (rc == -1) {
      if (errno == EINTR)
        continue;
      break;
    }
    if (resized.exchange(false)) {
      struct winsize new_ws{};
      if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &new_ws) == 0)
        ioctl(master_fd, TIOCSWINSZ, &new_ws);
    }
    if (fds[0].revents & POLLIN) {
      char buf[4096];
      ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
      if (n > 0)
        write_all(master_fd, buf, n);
      else if (n == 0)
        break;
    }
    if (fds[1].revents & POLLIN) {
      char buf[8192];
      ssize_t n = read(master_fd, buf, sizeof(buf));
      if (n > 0)
        write_all(STDOUT_FILENO, buf, n);
      else
        break;
    }
    if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
      break;
    if (fds[1].revents & (POLLERR | POLLHUP | POLLNVAL))
      break;
  }
  close(master_fd);
  int status;
  while (waitpid(pid, &status, 0) == -1)
    if (errno != EINTR)
      break;
}
} // namespace bed::internal::io
