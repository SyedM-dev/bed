# Crib

A set of text editing tools (still in early development).

### Modules

#### `Vase`

Vase is a avl piece tree like structure which handles loading, insertion, erasure, regex searching, regex replacing etc.
<br/>
done.

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

### BEd

Better ed.<br/>
An ed implementation (mostly posix compliant) but with:
- Lsp support for autocomplete suggetions when in ed text mode.
- Autocomplete for language snippets + other words found near the cursor etc.
- text mode where arrow keys etc. work as expected.
- All code printing done with syntax highlighting.
- Lsp diagnostics command to list global errors/warnings, to print thir lines, jump to their lines, edit them etc.
- Addressing modes that work by addressing ->
    - symbol definitions (lsp)
    - diagnostic points (lsp)
- Most lsp stuff (like rename symbol, symbol reference count etc.)
- Inlay hints while editing and printing.
- A read write command to open a set of lines similar to c command but with the text from before already filled.
- Code formatters (lsp and others).
- Ruby based extentions.
- Custom functions, live ruby code running.
- Debuggers (dap).
- Better shell commands/integration.
- Ruby based configs. (embedded Mruby)
- Remappable commands/macros.
- Better history & session saves.
- Multiple buffers per session support.
- Indentation engine (with editorconfig loading.).
- Build systems, npm, nix, make etc integration.
- More complete & fast regex.
- Better current line semantics.
- Full unicode + graphemes support.
- SSH support for loading files/lsp etc (and fully remote sessions.).
- Git commands / file diffs show in editor.
- File operations, directory & files listing.
- Nerdfonts & themable colorized outputs.
- Better address marking with ranges.
- & A lot more cool stuff.

#### Implementation status:

- Internal buffer:
    - Super fast and memory efficient buffer implementation done.
    - Loading from files/subshell commands done.
- Most of the posix ed commands except regex ones done.
