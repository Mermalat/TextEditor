# Mini Notepad++

A fully-featured, terminal-based text editor written in C with zero external dependencies. It runs entirely inside your console using raw terminal mode and ANSI escape sequences, providing the feel of a traditional text editor without ever leaving the terminal.

---

## Features at a Glance

| Area | What it does |
|---|---|
| Text Editing | Insert/delete characters & lines, multi-byte character support |
| Selection | Mouse-free keyboard selection with `Ctrl+Shift+Arrow` |
| Clipboard | Copy, Cut, Paste — syncs to the **system clipboard** via OSC-52 |
| Undo | Deep, multi-step undo covering all editing operations |
| Search | Find all occurrences, cycle between matches, Replace All |
| File I/O | Open, Save, Save As — all through an interactive directory browser |
| UTF-8 | Full multi-byte character handling (Turkish, special chars, emoji) |
| UI | Orange toolbox bar, status bar, line numbers, auto-resize |

---

## Building

```bash
make          # Compiles everything → produces ./main.exe
make re       # Force full rebuild (fclean + all)
make clean    # Remove compiled object files
make fclean   # Remove objects + the final binary
```

**Compiler flags used:** `-Wall -Wextra -Werror -Wno-unused-result -O2`

**Dependencies:** Standard C library only (`termios`, `dirent`, `sys/ioctl`, `locale`, `wchar`)

No external libraries. No package manager. No configuration needed.

---

## Running

```bash
./main.exe
```

The editor opens with an empty buffer. Use `Ctrl+O` to open a file via the built-in browser. Files are read line-by-line; CRLF (`\r\n`) line endings are handled automatically on load.

---

## Keyboard Reference

### File Operations

| Shortcut | Action |
|---|---|
| `Ctrl+O` | Open file (launches directory browser) |
| `Ctrl+S` | Save current file |
| `Ctrl+Shift+S` | Save As (launches directory browser in save mode) |
| `ESC` | Exit editor (prompts to save if unsaved changes exist) |

### Navigation

| Shortcut | Action |
|---|---|
| `Arrow Keys` | Move cursor one character / line |
| `Home` / `End` | Jump to start / end of line |
| `Page Up` / `Page Down` | Scroll one screen |
| `Ctrl+←` / `Ctrl+→` | Jump one word left / right |
| `Ctrl+↑` / `Ctrl+↓` | Move cursor up / down by one line (with vertical jump) |

### Editing

| Shortcut | Action |
|---|---|
| Any printable key | Insert character at cursor |
| `Enter` | Split line at cursor position |
| `Backspace` | Delete character before cursor |
| `Delete` | Delete character after cursor |
| `Ctrl+Backspace` | Delete entire word to the left |

### Selection

| Shortcut | Action |
|---|---|
| `Ctrl+Shift+←` / `Ctrl+Shift+→` | Extend selection one word left / right |
| `Ctrl+Shift+↑` / `Ctrl+Shift+↓` | Extend selection one line up / down |

Typing any character while a selection is active **replaces** the selection.

### Clipboard

| Shortcut | Action |
|---|---|
| `Ctrl+C` | Copy selection to internal + system clipboard |
| `Ctrl+X` | Cut selection |
| `Ctrl+V` | Paste at cursor |

### Search & Replace

| Shortcut | Action |
|---|---|
| `Ctrl+F` | Open search prompt; highlights all matches in the document |
| `Ctrl+G` | Jump to next match (cycles through matches) |
| `Ctrl+H` | Open Replace All prompt (replaces every occurrence) |

### Undo

| Shortcut | Action |
|---|---|
| `Ctrl+Z` | Undo the last operation |

---

## Architecture

### Program Flow

```
main()
  ├── init_editor()       — zero-fill state, create first line, query terminal size
  ├── enable_raw_mode()   — disable line buffering & echo via termios
  ├── signal(SIGWINCH)    — handle terminal window resize events
  └── main loop:
        ├── render_screen()     — draw the full UI to stdout
        └── process_keypress()  — read one key, dispatch to correct handler
```

The editor never blocks: each loop iteration renders the current state and then waits for exactly one keypress.

### Module Breakdown

#### `terminal.c` / `terminal_keys.c` — Raw Mode & Key Reading

The editor switches the terminal into **raw mode** (`termios`) on start:
- Echo is disabled — typed characters don't print automatically.
- Canonical mode is off — input is available byte-by-byte, not line-by-line.
- `SIGWINCH` signal triggers a live terminal resize by calling `ioctl(TIOCGWINSZ)`.

`read_key()` reads bytes from stdin and translates escape sequences (e.g., `\033[A` → `KEY_ARROW_UP`, `\033[1;6D` → `CTRL_SHIFT_LEFT`) into the `e_key` enum values. This mapping covers arrows, Home, End, Page Up/Down, Delete, and all `Ctrl`/`Ctrl+Shift` combinations used by the editor.

#### `line_list.c` / `line_ops.c` — Doubly-Linked Line List

Text is stored as a **doubly-linked list of `t_line` nodes**. Each node holds:
- `char *text` — heap-allocated, null-terminated byte buffer.
- `int len` — current byte count (not character count).
- `int cap` — allocated capacity; doubled when full (`ensure_capacity`).
- `prev` / `next` — links to adjacent lines.

Key operations:
- `line_insert_char(line, pos, ch, ch_len)` — inserts a UTF-8 character at a byte position, shifting the rest of the buffer right.
- `line_delete_char(line, pos, ch_len)` — removes bytes at a position using `memmove`.
- `line_insert_text(line, pos, text, len)` — bulk-inserts a segment (used during paste & undo).
- `insert_line_after(after, new)` — splices a new node into the linked list.
- `remove_line(line)` — unlinks a node (caller must `free_line` it separately).

`get_line_at(index)` traverses from `head` to reach a line by zero-based row number.

#### `utf8.c` / `utf8_words.c` — UTF-8 Engine

All cursor movement and character insertion is byte-aware:
- `utf8_char_len(c)` — reads the leading byte (`0xxx xxxx`, `110x xxxx`, `1110 xxxx`, `1111 0xxx`) to determine if a character is 1, 2, 3, or 4 bytes wide.
- `prev_utf8_char_start(text, pos)` — walks backwards over continuation bytes (`10xx xxxx`) to find the start of the previous character.
- `next_utf8_char_end(text, len, pos)` — advances past the current character.
- `byte_to_display_col(text, byte_pos)` — counts *characters* (not bytes) to derive the visual cursor column shown in the status bar.
- `display_to_byte_col(text, len, dcol)` — the inverse: maps a display column back to a byte offset (used for mouse-free cursor rendering).
- `word_start_before` / `word_end_after` — scan for word boundaries to support `Ctrl+Arrow` jumps and `Ctrl+Backspace` deletion.

#### `render.c` / `render_utils.c` — Screen Rendering

Every frame is drawn with direct `write(STDOUT_FILENO, …)` calls using ANSI escape codes. The cursor is hidden (`\033[?25l`) during drawing and restored afterwards to prevent flicker.

The screen is divided into three sections:
1. **Toolbox** (top, `TOOLBOX_HEIGHT = 3` rows) — rendered in bold white on orange (`\033[1;37;48;5;208m`), listing all shortcuts. Wraps automatically if terminal is too narrow.
2. **Line area** (middle) — each line is prefixed with a yellow 4-digit line number followed by a grey `|` separator. Characters are coloured on-the-fly:
   - **Selection highlight**: yellow background (`\033[43m`).
   - **Search match**: blue background (`\033[44m`).
   - **Current match**: bold green background (`\033[1;37;42m`).
3. **Status bar** (bottom row) — displays filename (or `[New File]`), a `[*]` unsaved-changes indicator, current `Line:row/total` and `Col:N` in reverse video (`\033[7m`).

Scrolling is purely vertical (`scroll_row`): the renderer walks the linked list to the `scroll_row`-th node and draws up to `screen_rows - TOOLBOX_HEIGHT - 1` lines.

#### `events.c` — Keypress Dispatcher

`process_keypress()` acts as a central router:
- Clears the active search state if any non-search key is pressed.
- `ESC` → `try_exit()`: if the buffer is modified, asks "Save changes? (y/n/ESC)" in the status bar before quitting.
- Regular printable characters: if a selection is active it is deleted first, then the character (possibly multi-byte UTF-8) is read completely from stdin and inserted.
- `Enter`: splits the current line at the cursor (text after cursor moves to a new line node).
- All other keys are dispatched to domain handler functions.

#### `undo.c` / `undo_exec.c` — Undo System

The undo history is a singly-linked **stack** of `t_undo_action` entries pushed before every mutation:

| Undo Type | Triggered by | How it reverses |
|---|---|---|
| `UNDO_INSERT_CHAR` | Typing a character | Deletes the inserted bytes |
| `UNDO_DELETE_CHAR` | Backspace / Delete | Re-inserts the deleted bytes |
| `UNDO_SPLIT_LINE` | Enter key | Joins the two lines back together (`undo_split_line`) |
| `UNDO_MERGE_LINE` | Backspace at line start | Splits the line at the stored column (`undo_merge_line`) |
| `UNDO_DELETE_SELECTION` | Deleting a selection | Re-inserts text char-by-char, re-creating line breaks where `\n` appears in the stored data |
| `UNDO_PASTE` | Paste | Removes the pasted region; handles single-line and multi-line paste separately |
| `UNDO_REPLACE_ALL` | Replace All | (stored for future reversal) |

Each action stores: operation type, the row/column where it happened, the raw text data affected, and an optional secondary position (`extra_row`/`extra_col`) needed for multi-point operations like paste.

#### `selection.c` / `selection_utils.c` — Selection

The selection is tracked as `(start_row, start_col) → (end_row, end_col)` in screen coordinates. Because the user can drag "backwards", `normalize_selection()` always produces a top-to-bottom, left-to-right range regardless of direction.

`get_selected_text()` collects the selected bytes into a dynamically-growing `t_buf`, inserting `\n` between lines where necessary. This buffer is used both for copying to the clipboard and for saving the data in undo actions before deletion.

#### Clipboard — System Integration via OSC-52

When copying, the editor sends an **OSC-52** terminal escape sequence:

```
ESC ] 52 ; c ; <base64-encoded data> BEL
```

This instructs compliant terminals (kitty, iTerm2, WezTerm, modern xterm, tmux with `set-clipboard on`) to write the content directly to the system clipboard. The base64 encoding is implemented from scratch in `osc52_copy()` / `osc52_write_chunk()` within `selection.c`.

Paste (`Ctrl+V`) uses the internal clipboard buffer (not a terminal read-back), so it works in all terminals.

#### `search.c` / `search_navigate.c` — Find & Replace

`find_matches(query)` does a linear scan through every line using `strstr`, collecting all `(row, byte_col, match_len)` triples into an array of `t_search_match`. The array grows dynamically with a doubling strategy.

`is_search_match(row, byte_col)` is called per-character during rendering to decide whether to apply search highlight colours:
- Returns `1` for a non-current match (blue).
- Returns `2` for the currently selected match (bright green).

`navigate_matches()` increments `current_match` cyclically and adjusts `scroll_row` so the active match is always visible.

`replace_all()` prompts for a replacement string, then iterates matches in reverse line order (to preserve earlier byte offsets) and splices in the replacement text.

#### `browser.c` / `browser_render.c` — Directory Browser

The browser is a full-screen file picker that replaces the main editor view temporarily:
- Reads directory entries via `opendir` / `readdir` / `stat`.
- Always prepends `..` for going up.
- Directories are flagged with `is_dir[]`; selecting one navigates into it.
- `↑` / `↓` move the selection, `Enter` opens a directory or selects a file.
- In **save mode** (`Ctrl+Shift+S`), pressing `n` shows an inline prompt (`prompt_input`) to type a new filename.
- `ESC` cancels the browser without changing the current buffer.

The result is written back to the calling `open_file()` / `save_as_file()` function via a `char *result_path` out-parameter.

#### `file_io.c` — File Reading & Writing

**Opening:** The file is read line-by-line with `fgets(buf, 4096, f)`. Both `\n` and `\r\n` endings are stripped. Each line becomes a new `t_line` node appended to the list. The undo stack, search state, and selection are all cleared on open.

**Saving:** The entire linked list is written sequentially — `fwrite(line->text, 1, line->len, f)` followed by a single `\n`. The filename is stored in `g_ed.filename` and the modified flag is cleared.

If `Ctrl+S` is pressed with no filename set (new file), it automatically falls back to `save_as_file()`.

---

## Data Structures

```c
// One line of text — node in the doubly-linked list
typedef struct s_line {
    char        *text;   // heap buffer
    int          len;    // used bytes
    int          cap;    // allocated bytes
    struct s_line *prev, *next;
} t_line;

// The global editor state (one instance: g_ed)
typedef struct s_editor {
    t_line      *head;          // first line
    int          num_lines;
    int          cursor_row, cursor_col;   // byte-offset column
    int          scroll_row, scroll_col;
    int          screen_rows, screen_cols;
    char         filename[512];
    int          modified, running;
    t_selection  sel;
    char        *clipboard;
    int          clipboard_len;
    t_search_match *matches;
    int          match_count, current_match, search_active;
    char         search_query[256];
    t_undo_action *undo_stack;  // singly-linked, LIFO
    struct termios orig_termios;
} t_editor;

// One entry on the undo stack
typedef struct s_undo_action {
    t_undo_type  type;
    int          row, col;        // position before the action
    char        *data;            // text affected
    int          data_len;
    int          extra_row, extra_col;   // secondary anchor (paste end, etc.)
    char        *extra_data;
    int          extra_data_len;
    struct s_undo_action *next;
} t_undo_action;
```

---

## Project File Map

```
prolab1/
├── Makefile
├── inc/
│   └── editor.h              ← all structs, enums, constants, prototypes
└── src/
    ├── main.c                ← entry point, init_editor, free_editor, SIGWINCH
    ├── terminal.c            ← raw mode enable/disable, terminal size query
    ├── terminal_keys.c       ← escape sequence → e_key enum parser
    ├── line_list.c           ← linked list create/insert/remove operations
    ├── line_ops.c            ← char-level insert/delete within a line buffer
    ├── render.c              ← render_screen, render_toolbox, scroll update
    ├── render_utils.c        ← render_lines, render_status_bar, prt_char
    ├── events.c              ← process_keypress, handle_enter, try_exit
    ├── events_movement.c     ← arrow keys, Home/End, Page, word jump
    ├── events_clipboard.c    ← paste logic
    ├── events_delete.c       ← Backspace, Delete, Ctrl+Backspace
    ├── selection.c           ← copy, cut, delete_selection, OSC-52 copy
    ├── selection_utils.c     ← normalize, is_in_selection, clear/start/update
    ├── search.c              ← find_matches, is_search_match, clear_search
    ├── search_navigate.c     ← navigate_matches, replace_all, search_text
    ├── undo.c                ← push_undo, undo, free_undo_stack
    ├── undo_exec.c           ← execute_complex_undo (split, merge, sel, paste)
    ├── browser.c             ← file_browser, directory navigation loop
    ├── browser_render.c      ← render_browser_screen
    ├── file_io.c             ← open_file, save_file, save_as_file
    ├── prompt.c              ← prompt_input inline text input widget
    ├── utf8.c                ← byte length, prev/next char, byte↔display col
    └── utf8_words.c          ← word_start_before, word_end_after
```

---

## Implementation Notes

- **No `ncurses`** — all rendering is done with direct ANSI escape sequences over `write(STDOUT_FILENO, …)`.
- **`cursor_col` is always a byte offset**, not a display column. `byte_to_display_col` converts it for the status bar and actual cursor positioning.
- **Undo never uses a redo stack** — any new edit after an undo permanently discards the future history.
- **The clipboard has two layers**: an internal `g_ed.clipboard` buffer (always available) and the system clipboard updated via OSC-52 (terminal-dependent).
- **Terminal resize** is handled by catching `SIGWINCH` and updating `screen_rows`/`screen_cols`; the next render loop iteration will automatically adapt the layout.

---

## License

Developed as a university lab project. No license applied.
