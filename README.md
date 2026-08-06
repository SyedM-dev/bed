# Crib

A set of text editing tools (still in early development).

### Modules

#### `Vase`

Vase is a avl piece tree like structure which handles loading, insertion, erasure, regex searching, regex replacing etc.
<br/>
done.

#### Ed

Ed is an ed (the posix line editor) implementation using `Vase`.
<br/>
Not done yet.

#### `hl`

`hl` is a stateful super fast syntax highlighter.
<br/>
Not done yet.

#### `lsp`

`lsp` is a generic plug for lsp's.
<br/>
<br/>
It should support:
- Lsp over ssh or stdin/out or unix sockets etc.
- Handling multiple lsps at once (including over a single file.)
- Converting offsets encoding between utf16 - utf8 according to lsp standards.
- Loading lsp features etc. automatically.
- Error handling.
- And more.
<br/>
<br/>
Not done yet.
