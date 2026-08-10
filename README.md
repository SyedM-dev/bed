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
Mostly done.

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

#### BEd

Better ed.<br/>
An ed implementation but with:
- Lsp support for autocomplete suggetions when in ed text mode.
- text mode where arrow keys etc. work as expected.
- All code printing done with syntax highlighting.
- Lsp diagnostics to list global errors/warnings, to print thir lines, jump to their lines, edint them etc.
- A read write command to open a set of lines similar to c command but with the text from before already filled.
- Code formatters (lsp and others).
- Ruby based extentions.
- Custom functions, live ruby code running.
- Debuggers etc. and better shell commands.
- Ruby based configs.
- Better history & session saves.
- Multiple buffers per session support.
- Indentation engine.
- More complete regex.
- Better current line semantics.
- Full unicode + graphemes support.
- SSH support for loading files/lsp etc.
- & A lot more cool stuff.
