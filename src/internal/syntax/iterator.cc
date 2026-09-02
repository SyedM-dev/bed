#include "internal/syntax/parser.h"

namespace bed::internal::syntax {
Iterator::Iterator(uint64_t target, ParserSnapshot p, vase::Shard *vase)
    : snap(p) {
  uint64_t offset;
  TreeCursor c = TreeCursor(*p.lang, p.root, target, &offset);
  at = target - offset;
  state = p.lang->copy(c.leaf->state);
  it = vase::Iterator(vase, at, Direction::Forward);
  while (at < target) {
    it->next();
    tokens.clear();
    events.clear();
    p.lang->parse(&state, it->line, at == 0, &tokens, &events);
    at++;
  }
}

Iterator::~Iterator() {
  if (state)
    snap.lang->destroy(state);
  release(snap);
}

Iterator::Iterator(Iterator &&other)
    : snap(other.snap),
      it(std::move(other.it)),
      state(other.state),
      tokens(std::move(other.tokens)) {
  other.snap = {};
  other.state = nullptr;
}

Iterator &Iterator::operator=(Iterator &&other) {
  if (this == &other)
    return *this;
  if (state)
    snap.lang->destroy(state);
  snap = other.snap;
  it = std::move(other.it);
  state = other.state;
  tokens = std::move(other.tokens);
  other.state = nullptr;
  return *this;
}

void Iterator::next() {
  it->next();
  tokens.clear();
  events.clear();
  snap.lang->parse(&state, it->line, at++ == 0, &tokens, &events);
}
} // namespace bed::internal::syntax
