#include "internal/io/io.h"
#include "bed.h"

namespace bed::internal::io {
termios IO::orig_termios{};
termios IO::raw_termios{};
bool IO::cleaned = true;
volatile std::atomic_bool IO::resized(false);
IO::Mode IO::mode = IO::Mode::PIPE;

IO::IO(BEd &bed) : bed(bed) {
  if (!isatty(STDIN_FILENO))
    return;
  mode = Mode::TERMINAL;
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

void IO::apply(const io::Token::Kind &t) {
  if (bed.suppress_mode)
    return;
  write("\x1b[0m");
  const auto &hl = bed.theme.get(t);
  const uint8_t r = (hl.fg >> 16) & 0xff;
  const uint8_t g = (hl.fg >> 8) & 0xff;
  const uint8_t b = hl.fg & 0xff;
  write(std::format("\x1b[38;2;{};{};{}m", r, g, b));
  if (hl.bg != 0) {
    const uint8_t br = (hl.bg >> 16) & 0xff;
    const uint8_t bg = (hl.bg >> 8) & 0xff;
    const uint8_t bb = hl.bg & 0xff;
    write(std::format("\x1b[48;2;{};{};{}m", br, bg, bb));
  }
  if (hl.flags & Highlight::Bold)
    write("\x1b[1m");
  if (hl.flags & Highlight::Italic)
    write("\x1b[3m");
  if (hl.flags & Highlight::Underline)
    write("\x1b[4m");
  if (hl.flags & Highlight::Strikethrough)
    write("\x1b[9m");
}

void IO::reset() {
  if (bed.suppress_mode)
    return;
  write("\x1b[0m");
}

void IO::enable_mouse() {
  if (mode == Mode::PIPE)
    throw fatal_error("no mouse in pipe mode.", 1);
  const char *seq = "\x1b[?1000h";
  write_all(STDOUT_FILENO, seq, 8);
}

void IO::disable_mouse() {
  if (mode == Mode::PIPE)
    throw fatal_error("no mouse in pipe mode.", 1);
  const char *seq = "\x1b[?1000l";
  write_all(STDOUT_FILENO, seq, 8);
}

std::pair<uint16_t, uint16_t> IO::terminal_size() {
  if (mode == Mode::PIPE)
    throw fatal_error("no terminal size in pipe mode.", 1);
  struct winsize ws{};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
    throw fatal_error("Can't get terminal size.", 1);
  return {ws.ws_row, ws.ws_col};
}

std::pair<uint16_t, uint16_t> IO::cursor_position() {
  if (mode == Mode::PIPE)
    throw fatal_error("no cursor in pipe mode.", 1);
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
  if (!cleaned || mode == Mode::PIPE)
    return;
  std::string os = "\x1b[?2004h";
  write_all(STDOUT_FILENO, os.c_str(), os.size());
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_termios) == -1)
    throw fatal_error("Can't set raw terminal state.", 1);
  cleaned = false;
}

void IO::cleanup() {
  if (cleaned || mode == Mode::PIPE)
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
  if (mode == Mode::PIPE)
    return;
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

void IO::write_line(std::string_view s) {
  write(s);
  write("\n", 1);
}

void IO::run_pty(const std::string &cmd) {
  if (mode == Mode::PIPE)
    throw ed_error("Shell running not allowed in pipe mode.");
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
    const char *shell = getenv("BED_SHELL");
    if (!shell || !*shell)
      shell = getenv("SHELL");
    if (!shell || !*shell)
      shell = "/bin/sh";
    execl(shell, shell, "-i", "-c", cmd.c_str(), (char *)nullptr);
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
