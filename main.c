/*
 * Mini Notepad++ — Console-Based Text Editor
 * Prolab II - Project 1 / Kocaeli University
 */
#include "editor.h"
#include <signal.h>
#include <time.h>

/* ═══ Forward declarations for statics ═══ */
static void ensure_capacity(Line *line, int needed);
static int prompt_input(Editor *E, const char *prompt, char *buf, int buf_size);
static void find_matches(Editor *E, const char *query);
static void normalize_selection(Selection *s, int *sr, int *sc, int *er, int *ec);
static int is_in_selection(Editor *E, int row, int byte_col);
static int is_search_match(Editor *E, int row, int byte_col);
static void write_file_to_disk(Editor *E, const char *path);

/* ════════════════════════════════════════════
 *  UTF-8 Helpers
 * ════════════════════════════════════════════ */

int utf8_char_len(unsigned char c) {
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

int prev_utf8_char_start(const char *text, int pos) {
    if (pos <= 0) return 0;
    pos--;
    while (pos > 0 && ((unsigned char)text[pos] & 0xC0) == 0x80) pos--;
    return pos;
}

int next_utf8_char_end(const char *text, int len, int pos) {
    if (pos >= len) return len;
    int cl = utf8_char_len((unsigned char)text[pos]);
    return (pos + cl > len) ? len : pos + cl;
}

int byte_to_display_col(const char *text, int byte_pos) {
    int col = 0, i = 0;
    while (i < byte_pos) { int cl = utf8_char_len((unsigned char)text[i]); i += cl; col++; }
    return col;
}

int display_to_byte_col(const char *text, int len, int dcol) {
    int col = 0, i = 0;
    while (i < len && col < dcol) { int cl = utf8_char_len((unsigned char)text[i]); i += cl; col++; }
    return i;
}

int word_start_before(const char *text, int pos) {
    if (pos <= 0) return 0;
    pos = prev_utf8_char_start(text, pos);
    while (pos > 0 && text[pos] == ' ') pos = prev_utf8_char_start(text, pos);
    while (pos > 0) { int pp = prev_utf8_char_start(text, pos); if (text[pp] == ' ') break; pos = pp; }
    return pos;
}

int word_end_after(const char *text, int len, int pos) {
    if (pos >= len) return len;
    while (pos < len && text[pos] == ' ') pos = next_utf8_char_end(text, len, pos);
    while (pos < len && text[pos] != ' ') pos = next_utf8_char_end(text, len, pos);
    return pos;
}

/* ════════════════════════════════════════════
 *  Terminal
 * ════════════════════════════════════════════ */

void die(const char *s) {
    write(STDOUT_FILENO, "\033[2J\033[H", 7);
    perror(s); exit(1);
}

void disable_raw_mode(Editor *E) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &E->orig_termios);
    write(STDOUT_FILENO, "\033[?25h", 6);
    write(STDOUT_FILENO, "\033[0 q", 5);
}

void enable_raw_mode(Editor *E) {
    if (tcgetattr(STDIN_FILENO, &E->orig_termios) == -1) die("tcgetattr");
    struct termios raw = E->orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
    write(STDOUT_FILENO, "\033[5 q", 5);
}

void get_terminal_size(int *rows, int *cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) { *rows = 24; *cols = 80; }
    else { *rows = ws.ws_row; *cols = ws.ws_col; }
}

int read_key(void) {
    int nread; char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
        if (nread == -1 && errno != EAGAIN) die("read");
    if (c == '\x1b') {
        char seq[8];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return KEY_ESC;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return KEY_ESC;
        if (seq[0] == '[') {
            if (seq[1] == '1') {
                char s2;
                if (read(STDIN_FILENO, &s2, 1) != 1) return KEY_ESC;
                if (s2 == ';') {
                    char mod, dir;
                    if (read(STDIN_FILENO, &mod, 1) != 1) return KEY_ESC;
                    if (read(STDIN_FILENO, &dir, 1) != 1) return KEY_ESC;
                    if (mod == '5') {
                        switch (dir) {
                            case 'A': return CTRL_ARROW_UP;
                            case 'B': return CTRL_ARROW_DOWN;
                            case 'C': return CTRL_ARROW_RIGHT;
                            case 'D': return CTRL_ARROW_LEFT;
                        }
                    } else if (mod == '6') {
                        switch (dir) {
                            case 'A': return CTRL_SHIFT_UP;
                            case 'B': return CTRL_SHIFT_DOWN;
                            case 'C': return CTRL_SHIFT_RIGHT;
                            case 'D': return CTRL_SHIFT_LEFT;
                        }
                    }
                } else if (s2 == '~') return KEY_HOME;
            } else if (seq[1] >= '0' && seq[1] <= '9') {
                char s2;
                if (read(STDIN_FILENO, &s2, 1) != 1) return KEY_ESC;
                if (s2 == '~') {
                    switch (seq[1]) {
                        case '3': return KEY_DELETE;
                        case '4': return KEY_END;
                        case '5': return KEY_PAGE_UP;
                        case '6': return KEY_PAGE_DOWN;
                        case '7': return KEY_HOME;
                        case '8': return KEY_END;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return KEY_ARROW_UP;
                    case 'B': return KEY_ARROW_DOWN;
                    case 'C': return KEY_ARROW_RIGHT;
                    case 'D': return KEY_ARROW_LEFT;
                    case 'H': return KEY_HOME;
                    case 'F': return KEY_END;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) { case 'H': return KEY_HOME; case 'F': return KEY_END; }
        }
        return KEY_ESC;
    }
    if (c == 8) return CTRL_H;
    return (int)(unsigned char)c;
}

/* ════════════════════════════════════════════
 *  Line Management (Linked List)
 * ════════════════════════════════════════════ */

Line *create_line(void) {
    Line *l = malloc(sizeof(Line));
    l->cap = INITIAL_LINE_CAP;
    l->text = malloc(l->cap);
    l->text[0] = '\0'; l->len = 0;
    l->prev = NULL; l->next = NULL;
    return l;
}

void free_line(Line *line) { if (line) { free(line->text); free(line); } }

Line *get_line_at(Editor *E, int index) {
    Line *l = E->head;
    for (int i = 0; i < index && l; i++) l = l->next;
    return l;
}

void insert_line_after(Editor *E, Line *after, Line *new_line) {
    if (!after) {
        new_line->next = E->head;
        if (E->head) E->head->prev = new_line;
        E->head = new_line;
    } else {
        new_line->next = after->next;
        new_line->prev = after;
        if (after->next) after->next->prev = new_line;
        after->next = new_line;
    }
    E->num_lines++;
}

void remove_line(Editor *E, Line *line) {
    if (line->prev) line->prev->next = line->next;
    else E->head = line->next;
    if (line->next) line->next->prev = line->prev;
    E->num_lines--;
}

static void ensure_capacity(Line *line, int needed) {
    if (line->len + needed + 1 > line->cap) {
        while (line->cap < line->len + needed + 1) line->cap *= 2;
        line->text = realloc(line->text, line->cap);
    }
}

void line_insert_char(Line *line, int pos, const char *ch, int ch_len) {
    ensure_capacity(line, ch_len);
    memmove(line->text + pos + ch_len, line->text + pos, line->len - pos);
    memcpy(line->text + pos, ch, ch_len);
    line->len += ch_len;
    line->text[line->len] = '\0';
}

void line_delete_char(Line *line, int pos, int ch_len) {
    if (pos + ch_len > line->len) return;
    memmove(line->text + pos, line->text + pos + ch_len, line->len - pos - ch_len);
    line->len -= ch_len;
    line->text[line->len] = '\0';
}

void line_insert_text(Line *line, int pos, const char *text, int text_len) {
    ensure_capacity(line, text_len);
    memmove(line->text + pos + text_len, line->text + pos, line->len - pos);
    memcpy(line->text + pos, text, text_len);
    line->len += text_len;
    line->text[line->len] = '\0';
}

/* ════════════════════════════════════════════
 *  Undo Stack
 * ════════════════════════════════════════════ */

void push_undo(Editor *E, UndoType type, int row, int col,
               const char *data, int data_len,
               int extra_row, int extra_col,
               const char *extra_data, int extra_data_len) {
    UndoAction *a = malloc(sizeof(UndoAction));
    a->type = type; a->row = row; a->col = col;
    a->data = NULL; a->data_len = data_len;
    if (data && data_len > 0) { a->data = malloc(data_len + 1); memcpy(a->data, data, data_len); a->data[data_len] = '\0'; }
    a->extra_row = extra_row; a->extra_col = extra_col;
    a->extra_data = NULL; a->extra_data_len = extra_data_len;
    if (extra_data && extra_data_len > 0) { a->extra_data = malloc(extra_data_len + 1); memcpy(a->extra_data, extra_data, extra_data_len); a->extra_data[extra_data_len] = '\0'; }
    a->next = E->undo_stack;
    E->undo_stack = a;
}

void undo(Editor *E) {
    UndoAction *a = E->undo_stack;
    if (!a) return;
    E->undo_stack = a->next;
    Line *line;
    switch (a->type) {
        case UNDO_INSERT_CHAR:
            line = get_line_at(E, a->row);
            if (line) line_delete_char(line, a->col, a->data_len);
            E->cursor_row = a->row; E->cursor_col = a->col;
            break;
        case UNDO_DELETE_CHAR:
            line = get_line_at(E, a->row);
            if (line && a->data) line_insert_text(line, a->col, a->data, a->data_len);
            E->cursor_row = a->row; E->cursor_col = a->col + a->data_len;
            break;
        case UNDO_SPLIT_LINE: {
            Line *cur = get_line_at(E, a->row);
            Line *nxt = get_line_at(E, a->row + 1);
            if (cur && nxt) {
                line_insert_text(cur, cur->len, nxt->text, nxt->len);
                remove_line(E, nxt); free_line(nxt);
            }
            E->cursor_row = a->row; E->cursor_col = a->col;
            break;
        }
        case UNDO_MERGE_LINE: {
            line = get_line_at(E, a->row);
            if (line) {
                Line *new_l = create_line();
                int split_pos = a->col;
                if (split_pos < line->len) {
                    line_insert_text(new_l, 0, line->text + split_pos, line->len - split_pos);
                    line->len = split_pos; line->text[line->len] = '\0';
                }
                insert_line_after(E, line, new_l);
            }
            E->cursor_row = a->row + 1; E->cursor_col = 0;
            break;
        }
        case UNDO_DELETE_SELECTION:
            if (a->data) {
                int r = a->row, c = a->col;
                for (int i = 0; i < a->data_len; i++) {
                    if (a->data[i] == '\n') {
                        Line *cl = get_line_at(E, r);
                        Line *nl = create_line();
                        if (c < cl->len) { line_insert_text(nl, 0, cl->text + c, cl->len - c); cl->len = c; cl->text[cl->len] = '\0'; }
                        insert_line_after(E, cl, nl); r++; c = 0;
                    } else {
                        int cl_len = utf8_char_len((unsigned char)a->data[i]);
                        Line *cl = get_line_at(E, r);
                        line_insert_char(cl, c, &a->data[i], cl_len);
                        c += cl_len; i += cl_len - 1;
                    }
                }
            }
            E->cursor_row = a->row; E->cursor_col = a->col;
            break;
        case UNDO_PASTE:
            if (a->data) {
                int sr = a->row, sc = a->col, er = a->extra_row, ec = a->extra_col;
                if (sr == er) {
                    Line *l = get_line_at(E, sr);
                    if (l && ec > sc) { memmove(l->text + sc, l->text + ec, l->len - ec); l->len -= (ec - sc); l->text[l->len] = '\0'; }
                } else {
                    Line *sl = get_line_at(E, sr);
                    Line *el = get_line_at(E, er);
                    if (sl && el) {
                        sl->len = sc; sl->text[sl->len] = '\0';
                        line_insert_text(sl, sl->len, el->text + ec, el->len - ec);
                        for (int i = er; i > sr; i--) { Line *dl = get_line_at(E, sr + 1); if (dl) { remove_line(E, dl); free_line(dl); } }
                    }
                }
            }
            E->cursor_row = a->row; E->cursor_col = a->col;
            break;
        default: break;
    }
    E->modified = 1;
    free(a->data); free(a->extra_data); free(a);
}

void free_undo_stack(Editor *E) {
    while (E->undo_stack) {
        UndoAction *a = E->undo_stack; E->undo_stack = a->next;
        free(a->data); free(a->extra_data); free(a);
    }
}

/* ════════════════════════════════════════════
 *  Selection
 * ════════════════════════════════════════════ */

void clear_selection(Editor *E) { E->sel.active = 0; }

void start_selection(Editor *E) {
    if (!E->sel.active) {
        E->sel.active = 1;
        E->sel.start_row = E->cursor_row;
        E->sel.start_col = E->cursor_col;
    }
    E->sel.end_row = E->cursor_row;
    E->sel.end_col = E->cursor_col;
}

void update_selection(Editor *E) {
    E->sel.end_row = E->cursor_row;
    E->sel.end_col = E->cursor_col;
}

static void normalize_selection(Selection *s, int *sr, int *sc, int *er, int *ec) {
    if (s->start_row < s->end_row || (s->start_row == s->end_row && s->start_col <= s->end_col)) {
        *sr = s->start_row; *sc = s->start_col; *er = s->end_row; *ec = s->end_col;
    } else {
        *sr = s->end_row; *sc = s->end_col; *er = s->start_row; *ec = s->start_col;
    }
}

char *get_selected_text(Editor *E, int *out_len) {
    if (!E->sel.active) { *out_len = 0; return NULL; }
    int sr, sc, er, ec;
    normalize_selection(&E->sel, &sr, &sc, &er, &ec);
    int cap = 1024, len = 0;
    char *buf = malloc(cap);
    for (int r = sr; r <= er; r++) {
        Line *l = get_line_at(E, r);
        if (!l) continue;
        int start = (r == sr) ? sc : 0;
        int end = (r == er) ? ec : l->len;
        int seg = end - start;
        if (len + seg + 2 > cap) { cap = (len + seg + 2) * 2; buf = realloc(buf, cap); }
        memcpy(buf + len, l->text + start, seg); len += seg;
        if (r < er) buf[len++] = '\n';
    }
    buf[len] = '\0'; *out_len = len;
    return buf;
}

void delete_selection(Editor *E) {
    if (!E->sel.active) return;
    int sr, sc, er, ec;
    normalize_selection(&E->sel, &sr, &sc, &er, &ec);
    int sel_len; char *sel_text = get_selected_text(E, &sel_len);
    push_undo(E, UNDO_DELETE_SELECTION, sr, sc, sel_text, sel_len, er, ec, NULL, 0);
    free(sel_text);
    if (sr == er) {
        Line *l = get_line_at(E, sr);
        if (l && ec > sc) { memmove(l->text + sc, l->text + ec, l->len - ec); l->len -= (ec - sc); l->text[l->len] = '\0'; }
    } else {
        Line *sl = get_line_at(E, sr); Line *el = get_line_at(E, er);
        if (sl && el) {
            sl->len = sc; sl->text[sl->len] = '\0';
            line_insert_text(sl, sl->len, el->text + ec, el->len - ec);
            for (int i = er; i > sr; i--) { Line *dl = get_line_at(E, sr + 1); if (dl) { remove_line(E, dl); free_line(dl); } }
        }
    }
    E->cursor_row = sr; E->cursor_col = sc;
    E->sel.active = 0; E->modified = 1;
}

/* ════════════════════════════════════════════
 *  Clipboard
 * ════════════════════════════════════════════ */

void copy_selection(Editor *E) {
    if (!E->sel.active) return;
    free(E->clipboard);
    E->clipboard = get_selected_text(E, &E->clipboard_len);
}

void cut_selection(Editor *E) { copy_selection(E); delete_selection(E); }

void paste(Editor *E) {
    if (!E->clipboard || E->clipboard_len == 0) return;
    if (E->sel.active) delete_selection(E);
    int start_row = E->cursor_row, start_col = E->cursor_col;
    for (int i = 0; i < E->clipboard_len; i++) {
        if (E->clipboard[i] == '\n') {
            Line *cur = get_line_at(E, E->cursor_row);
            Line *new_l = create_line();
            if (E->cursor_col < cur->len) {
                line_insert_text(new_l, 0, cur->text + E->cursor_col, cur->len - E->cursor_col);
                cur->len = E->cursor_col; cur->text[cur->len] = '\0';
            }
            insert_line_after(E, cur, new_l); E->cursor_row++; E->cursor_col = 0;
        } else {
            int cl = utf8_char_len((unsigned char)E->clipboard[i]);
            Line *cur = get_line_at(E, E->cursor_row);
            line_insert_char(cur, E->cursor_col, &E->clipboard[i], cl);
            E->cursor_col += cl; i += cl - 1;
        }
    }
    push_undo(E, UNDO_PASTE, start_row, start_col, E->clipboard, E->clipboard_len, E->cursor_row, E->cursor_col, NULL, 0);
    E->modified = 1;
}

/* ════════════════════════════════════════════
 *  Search & Replace
 * ════════════════════════════════════════════ */

static int prompt_input(Editor *E, const char *prompt, char *buf, int buf_size) {
    set_cursor_position(E->screen_rows, 1);
    write(STDOUT_FILENO, "\033[K\033[7m", 7);
    write(STDOUT_FILENO, prompt, strlen(prompt));
    write(STDOUT_FILENO, "\033[0m ", 5);
    int len = 0; buf[0] = '\0';
    while (1) {
        set_cursor_position(E->screen_rows, (int)strlen(prompt) + 2);
        write(STDOUT_FILENO, "\033[K", 3);
        write(STDOUT_FILENO, buf, len);
        int key = read_key();
        if (key == KEY_ENTER) return 1;
        if (key == KEY_ESC) return 0;
        if (key == KEY_BACKSPACE || key == 127) {
            if (len > 0) { len = prev_utf8_char_start(buf, len); buf[len] = '\0'; }
        } else if (key >= 32) {
            int cl = utf8_char_len((unsigned char)key);
            if (cl == 1 && len + 1 < buf_size) { buf[len++] = (char)key; buf[len] = '\0'; }
            else if (cl > 1 && len + cl < buf_size) {
                buf[len++] = (char)key;
                for (int j = 1; j < cl; j++) { char cb; if (read(STDIN_FILENO, &cb, 1) == 1) buf[len++] = cb; }
                buf[len] = '\0';
            }
        }
    }
}

void clear_search(Editor *E) {
    free(E->matches); E->matches = NULL;
    E->match_count = 0; E->current_match = -1;
    E->search_active = 0; E->search_query[0] = '\0';
}

static void find_matches(Editor *E, const char *query) {
    free(E->matches); E->matches = NULL; E->match_count = 0;
    int cap = 64; E->matches = malloc(cap * sizeof(SearchMatch));
    int qlen = strlen(query);
    Line *l = E->head; int row = 0;
    while (l) {
        const char *p = l->text;
        while ((p = strstr(p, query)) != NULL) {
            if (E->match_count >= cap) { cap *= 2; E->matches = realloc(E->matches, cap * sizeof(SearchMatch)); }
            E->matches[E->match_count].row = row;
            E->matches[E->match_count].col = (int)(p - l->text);
            E->matches[E->match_count].len = qlen;
            E->match_count++; p += qlen;
        }
        l = l->next; row++;
    }
}

void search_text(Editor *E) {
    char query[MAX_SEARCH_LEN];
    if (!prompt_input(E, "Find:", query, MAX_SEARCH_LEN)) { clear_search(E); return; }
    strcpy(E->search_query, query);
    find_matches(E, query);
    E->search_active = 1;
    E->current_match = E->match_count > 0 ? 0 : -1;
}

void navigate_matches(Editor *E) {
    char query[MAX_SEARCH_LEN];
    if (!prompt_input(E, "Go to:", query, MAX_SEARCH_LEN)) { clear_search(E); return; }
    strcpy(E->search_query, query);
    find_matches(E, query); E->search_active = 1;
    if (E->match_count == 0) { E->current_match = -1; return; }
    E->current_match = 0;
    while (1) {
        E->cursor_row = E->matches[E->current_match].row;
        E->cursor_col = E->matches[E->current_match].col;
        int text_rows = E->screen_rows - TOOLBOX_HEIGHT - 1;
        if (E->cursor_row < E->scroll_row) E->scroll_row = E->cursor_row;
        if (E->cursor_row >= E->scroll_row + text_rows) E->scroll_row = E->cursor_row - text_rows + 1;
        render_screen(E);
        int key = read_key();
        if (key == KEY_ARROW_RIGHT || key == KEY_ARROW_DOWN) E->current_match = (E->current_match + 1) % E->match_count;
        else if (key == KEY_ARROW_LEFT || key == KEY_ARROW_UP) E->current_match = (E->current_match - 1 + E->match_count) % E->match_count;
        else if (key == KEY_ESC || key == KEY_ENTER) break;
    }
}

void replace_all(Editor *E) {
    char old_text[MAX_SEARCH_LEN], new_text[MAX_SEARCH_LEN];
    if (!prompt_input(E, "Find:", old_text, MAX_SEARCH_LEN)) return;
    if (!prompt_input(E, "Replace with:", new_text, MAX_SEARCH_LEN)) return;
    int old_len = strlen(old_text), new_len = strlen(new_text);
    Line *l = E->head;
    while (l) {
        char *p; int offset = 0;
        while ((p = strstr(l->text + offset, old_text)) != NULL) {
            int pos = (int)(p - l->text);
            memmove(l->text + pos, l->text + pos + old_len, l->len - pos - old_len);
            l->len -= old_len;
            ensure_capacity(l, new_len);
            memmove(l->text + pos + new_len, l->text + pos, l->len - pos);
            memcpy(l->text + pos, new_text, new_len);
            l->len += new_len; l->text[l->len] = '\0';
            offset = pos + new_len;
        }
        l = l->next;
    }
    E->modified = 1; clear_search(E);
}

/* ════════════════════════════════════════════
 *  File Browser & File Operations
 * ════════════════════════════════════════════ */

void file_browser(Editor *E, char *result_path, int save_mode) {
    char cwd[MAX_PATH_LEN];
    if (!getcwd(cwd, sizeof(cwd))) strcpy(cwd, ".");
    int selected = 0;
    while (1) {
        DIR *d = opendir(cwd);
        if (!d) { result_path[0] = '\0'; return; }
        struct dirent *ent;
        char entries[512][256]; int is_dir_arr[512]; int count = 0;
        strcpy(entries[count], ".."); is_dir_arr[count] = 1; count++;
        while ((ent = readdir(d)) != NULL && count < 512) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
            strcpy(entries[count], ent->d_name);
            char fullpath[MAX_PATH_LEN];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", cwd, ent->d_name);
            struct stat st;
            is_dir_arr[count] = (stat(fullpath, &st) == 0 && S_ISDIR(st.st_mode));
            count++;
        }
        closedir(d);
        if (selected >= count) selected = count - 1;
        if (selected < 0) selected = 0;

        write(STDOUT_FILENO, "\033[2J\033[H", 7);
        char header[MAX_PATH_LEN + 80];
        int hlen = snprintf(header, sizeof(header), "\033[1;36m === File Browser ===\033[0m\r\n\033[33m %s\033[0m\r\n\r\n", cwd);
        write(STDOUT_FILENO, header, hlen);
        int text_rows = E->screen_rows - 6;
        int scroll = (selected >= text_rows) ? selected - text_rows + 1 : 0;
        for (int i = scroll; i < count && i < scroll + text_rows; i++) {
            char lb[512]; int ll;
            if (i == selected) {
                if (is_dir_arr[i]) ll = snprintf(lb, sizeof(lb), " \033[7;34m > [%s/]\033[0m\r\n", entries[i]);
                else ll = snprintf(lb, sizeof(lb), " \033[7m > %s\033[0m\r\n", entries[i]);
            } else {
                if (is_dir_arr[i]) ll = snprintf(lb, sizeof(lb), "   \033[34m[%s/]\033[0m\r\n", entries[i]);
                else ll = snprintf(lb, sizeof(lb), "   %s\r\n", entries[i]);
            }
            write(STDOUT_FILENO, lb, ll);
        }
        const char *instr = "\r\n\033[90m Up/Down: Navigate | Enter: Select | ESC: Cancel";
        write(STDOUT_FILENO, instr, strlen(instr));
        if (save_mode) { const char *si = " | n: New filename\033[0m"; write(STDOUT_FILENO, si, strlen(si)); }
        write(STDOUT_FILENO, "\033[0m\r\n", 6);

        int key = read_key();
        if (key == KEY_ARROW_UP && selected > 0) selected--;
        else if (key == KEY_ARROW_DOWN && selected < count - 1) selected++;
        else if (key == KEY_ENTER) {
            if (is_dir_arr[selected]) {
                char newpath[MAX_PATH_LEN];
                if (strcmp(entries[selected], "..") == 0) {
                    char *slash = strrchr(cwd, '/');
                    if (slash && slash != cwd) *slash = '\0'; else strcpy(cwd, "/");
                } else { snprintf(newpath, sizeof(newpath), "%s/%s", cwd, entries[selected]); strcpy(cwd, newpath); }
                selected = 0;
            } else {
                snprintf(result_path, MAX_PATH_LEN, "%s/%s", cwd, entries[selected]);
                return;
            }
        } else if (key == KEY_ESC) { result_path[0] = '\0'; return; }
        else if (save_mode && key == 'n') {
            char fname[256];
            if (prompt_input(E, "Filename:", fname, 256) && fname[0]) {
                snprintf(result_path, MAX_PATH_LEN, "%s/%s", cwd, fname);
                return;
            }
        }
    }
}

void open_file(Editor *E) {
    char path[MAX_PATH_LEN];
    file_browser(E, path, 0);
    if (path[0] == '\0') return;
    FILE *f = fopen(path, "r");
    if (!f) return;
    Line *l = E->head;
    while (l) { Line *nx = l->next; free_line(l); l = nx; }
    E->head = NULL; E->num_lines = 0;
    free_undo_stack(E); clear_search(E); clear_selection(E);
    char buf[4096]; Line *prev = NULL;
    while (fgets(buf, sizeof(buf), f)) {
        Line *nl = create_line();
        int blen = strlen(buf);
        if (blen > 0 && buf[blen - 1] == '\n') buf[--blen] = '\0';
        if (blen > 0 && buf[blen - 1] == '\r') buf[--blen] = '\0';
        line_insert_text(nl, 0, buf, blen);
        insert_line_after(E, prev, nl); prev = nl;
    }
    fclose(f);
    if (!E->head) { E->head = create_line(); E->num_lines = 1; }
    strcpy(E->filename, path);
    E->cursor_row = 0; E->cursor_col = 0; E->scroll_row = 0; E->modified = 0;
}

static void write_file_to_disk(Editor *E, const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    Line *l = E->head;
    while (l) { fwrite(l->text, 1, l->len, f); fputc('\n', f); l = l->next; }
    fclose(f);
    strcpy(E->filename, path); E->modified = 0;
}

void save_file(Editor *E) {
    if (E->filename[0]) write_file_to_disk(E, E->filename);
    else save_as_file(E);
}

void save_as_file(Editor *E) {
    char path[MAX_PATH_LEN];
    file_browser(E, path, 1);
    if (path[0] == '\0') return;
    write_file_to_disk(E, path);
}

/* ════════════════════════════════════════════
 *  Rendering
 * ════════════════════════════════════════════ */

void set_cursor_position(int row, int col) {
    char buf[32]; int n = snprintf(buf, sizeof(buf), "\033[%d;%dH", row, col);
    write(STDOUT_FILENO, buf, n);
}

static int is_in_selection(Editor *E, int row, int byte_col) {
    if (!E->sel.active) return 0;
    int sr, sc, er, ec;
    normalize_selection(&E->sel, &sr, &sc, &er, &ec);
    if (row < sr || row > er) return 0;
    if (row == sr && row == er) return (byte_col >= sc && byte_col < ec);
    if (row == sr) return byte_col >= sc;
    if (row == er) return byte_col < ec;
    return 1;
}

static int is_search_match(Editor *E, int row, int byte_col) {
    if (!E->search_active) return 0;
    for (int i = 0; i < E->match_count; i++) {
        if (E->matches[i].row == row && byte_col >= E->matches[i].col && byte_col < E->matches[i].col + E->matches[i].len)
            return (i == E->current_match) ? 2 : 1;
    }
    return 0;
}

void render_toolbox(Editor *E) {
    set_cursor_position(1, 1);
    write(STDOUT_FILENO, "\033[K\033[1;37;44m", 13);
    const char *items[] = {
        "Open(^O)", "Save(^S)", "SaveAs(^Shift+S)",
        "Find(^F)", "GoTo(^G)", "Replace(^H)",
        "Copy(^C)", "Cut(^X)", "Paste(^V)",
        "Undo(^Z)", "Exit(ESC)"
    };
    int nItems = 11, col = 1, row = 1;
    for (int i = 0; i < nItems; i++) {
        int item_len = (int)strlen(items[i]);
        int display_len = byte_to_display_col(items[i], item_len);
        if (col + display_len + 3 > E->screen_cols && col > 1) {
            int remain = E->screen_cols - col + 1;
            for (int r = 0; r < remain; r++) write(STDOUT_FILENO, " ", 1);
            row++;
            set_cursor_position(row, 1);
            write(STDOUT_FILENO, "\033[K\033[1;37;44m", 13);
            col = 1;
        }
        write(STDOUT_FILENO, " ", 1);
        write(STDOUT_FILENO, items[i], item_len);
        write(STDOUT_FILENO, " |", 2);
        col += display_len + 3;
    }
    int remain = E->screen_cols - col + 1;
    for (int r = 0; r < remain; r++) write(STDOUT_FILENO, " ", 1);
    write(STDOUT_FILENO, "\033[0m", 4);
    set_cursor_position(row + 1, 1);
    write(STDOUT_FILENO, "\033[K\033[36m", 8);
    for (int i = 0; i < E->screen_cols; i++) write(STDOUT_FILENO, "-", 1);
    write(STDOUT_FILENO, "\033[0m", 4);
}

void render_screen(Editor *E) {
    get_terminal_size(&E->screen_rows, &E->screen_cols);
    int text_rows = E->screen_rows - TOOLBOX_HEIGHT - 1;
    if (E->cursor_row < E->scroll_row) E->scroll_row = E->cursor_row;
    if (E->cursor_row >= E->scroll_row + text_rows) E->scroll_row = E->cursor_row - text_rows + 1;
    write(STDOUT_FILENO, "\033[?25l", 6);
    render_toolbox(E);
    Line *line = E->head; int line_idx = 0;
    while (line && line_idx < E->scroll_row) { line = line->next; line_idx++; }
    for (int screen_r = 0; screen_r < text_rows; screen_r++) {
        set_cursor_position(TOOLBOX_HEIGHT + 1 + screen_r, 1);
        write(STDOUT_FILENO, "\033[K", 3);
        if (line) {
            char num_buf[32];
            int nlen = snprintf(num_buf, sizeof(num_buf), "\033[33m%4d\033[90m|\033[0m", line_idx + 1);
            write(STDOUT_FILENO, num_buf, nlen);
            int byte = 0;
            while (byte < line->len) {
                int cl = utf8_char_len((unsigned char)line->text[byte]);
                int sel = is_in_selection(E, line_idx, byte);
                int srch = is_search_match(E, line_idx, byte);
                if (srch == 2) write(STDOUT_FILENO, "\033[1;37;42m", 10);
                else if (srch == 1) write(STDOUT_FILENO, "\033[44m", 5);
                else if (sel) write(STDOUT_FILENO, "\033[43m", 5);
                write(STDOUT_FILENO, line->text + byte, cl);
                if (sel || srch) write(STDOUT_FILENO, "\033[0m", 4);
                byte += cl;
            }
            line = line->next; line_idx++;
        } else {
            write(STDOUT_FILENO, "\033[90m   ~\033[0m", 11);
        }
    }
    set_cursor_position(E->screen_rows, 1);
    write(STDOUT_FILENO, "\033[K\033[7m", 7);
    Line *cur_line = get_line_at(E, E->cursor_row);
    char status[512];
    int slen = snprintf(status, sizeof(status), " %s%s | Line: %d/%d | Col: %d ",
        E->filename[0] ? E->filename : "[New File]",
        E->modified ? " [*]" : "",
        E->cursor_row + 1, E->num_lines,
        cur_line ? byte_to_display_col(cur_line->text, E->cursor_col) + 1 : 1);
    write(STDOUT_FILENO, status, slen);
    int pad = E->screen_cols - slen;
    if (pad < 0) pad = 0;
    for (int i = 0; i < pad; i++) write(STDOUT_FILENO, " ", 1);
    write(STDOUT_FILENO, "\033[0m", 4);
    int disp_col = cur_line ? byte_to_display_col(cur_line->text, E->cursor_col) : 0;
    set_cursor_position(TOOLBOX_HEIGHT + 1 + (E->cursor_row - E->scroll_row), LINE_NUM_WIDTH + 1 + disp_col);
    write(STDOUT_FILENO, "\033[?25h", 6);
}

/* ════════════════════════════════════════════
 *  Process Keypress & Main Loop
 * ════════════════════════════════════════════ */

void try_exit(Editor *E) {
    if (E->modified) {
        /* Simple single-key prompt instead of prompt_input */
        set_cursor_position(E->screen_rows, 1);
        write(STDOUT_FILENO, "\033[K", 3);
        const char *msg = "\033[7m Save changes? (y/n/ESC) \033[0m";
        write(STDOUT_FILENO, msg, strlen(msg));
        while (1) {
            int key = read_key();
            if (key == 'y' || key == 'Y') {
                save_file(E);
                E->running = 0;
                return;
            } else if (key == 'n' || key == 'N') {
                E->running = 0;
                return;
            } else if (key == KEY_ESC) {
                return; /* Cancel exit, keep running */
            }
        }
    }
    E->running = 0;
}

void process_keypress(Editor *E) {
    int key = read_key();
    Line *cur = get_line_at(E, E->cursor_row);
    if (!cur) return;

    /* Clear search on non-search key */
    if (key != CTRL_F && key != CTRL_G && E->search_active &&
        key != KEY_ARROW_UP && key != KEY_ARROW_DOWN &&
        key != KEY_ARROW_LEFT && key != KEY_ARROW_RIGHT) {
        clear_search(E);
    }

    switch (key) {
    case KEY_ESC:
        try_exit(E); break;

    /* ─── File Operations ─── */
    case CTRL_O: open_file(E); break;
    case CTRL_S: save_file(E); break;
    case CTRL_SHIFT_S: save_as_file(E); break;

    /* ─── Search ─── */
    case CTRL_F: search_text(E); break;
    case CTRL_G: navigate_matches(E); break;
    case CTRL_H: replace_all(E); break;

    /* ─── Clipboard ─── */
    case CTRL_C_KEY: copy_selection(E); break;
    case CTRL_X: cut_selection(E); break;
    case CTRL_V: paste(E); break;

    /* ─── Undo ─── */
    case CTRL_Z: undo(E); break;

    /* ─── Selection (CTRL+Arrow) ─── */
    case CTRL_ARROW_LEFT:
        start_selection(E);
        if (E->cursor_col > 0) {
            E->cursor_col = prev_utf8_char_start(cur->text, E->cursor_col);
        } else if (E->cursor_row > 0) {
            E->cursor_row--;
            cur = get_line_at(E, E->cursor_row);
            E->cursor_col = cur->len;
        }
        update_selection(E); break;
    case CTRL_ARROW_RIGHT:
        start_selection(E);
        if (E->cursor_col < cur->len) {
            E->cursor_col = next_utf8_char_end(cur->text, cur->len, E->cursor_col);
        } else if (E->cursor_row < E->num_lines - 1) {
            E->cursor_row++; E->cursor_col = 0;
        }
        update_selection(E); break;
    case CTRL_ARROW_UP:
        start_selection(E);
        if (E->cursor_row > 0) { E->cursor_row--; cur = get_line_at(E, E->cursor_row); if (E->cursor_col > cur->len) E->cursor_col = cur->len; }
        update_selection(E); break;
    case CTRL_ARROW_DOWN:
        start_selection(E);
        if (E->cursor_row < E->num_lines - 1) { E->cursor_row++; cur = get_line_at(E, E->cursor_row); if (E->cursor_col > cur->len) E->cursor_col = cur->len; }
        update_selection(E); break;

    /* ─── Selection (CTRL+SHIFT+Arrow) word-level ─── */
    case CTRL_SHIFT_LEFT:
        start_selection(E);
        E->cursor_col = word_start_before(cur->text, E->cursor_col);
        update_selection(E); break;
    case CTRL_SHIFT_RIGHT:
        start_selection(E);
        E->cursor_col = word_end_after(cur->text, cur->len, E->cursor_col);
        update_selection(E); break;
    case CTRL_SHIFT_UP:
        start_selection(E);
        if (E->cursor_row > 0) { E->cursor_row--; E->cursor_col = 0; }
        update_selection(E); break;
    case CTRL_SHIFT_DOWN:
        start_selection(E);
        if (E->cursor_row < E->num_lines - 1) { E->cursor_row++; cur = get_line_at(E, E->cursor_row); E->cursor_col = cur->len; }
        update_selection(E); break;

    /* ─── Arrow keys (movement) ─── */
    case KEY_ARROW_LEFT:
        clear_selection(E);
        if (E->cursor_col > 0) E->cursor_col = prev_utf8_char_start(cur->text, E->cursor_col);
        else if (E->cursor_row > 0) { E->cursor_row--; cur = get_line_at(E, E->cursor_row); E->cursor_col = cur->len; }
        break;
    case KEY_ARROW_RIGHT:
        clear_selection(E);
        if (E->cursor_col < cur->len) E->cursor_col = next_utf8_char_end(cur->text, cur->len, E->cursor_col);
        else if (E->cursor_row < E->num_lines - 1) { E->cursor_row++; E->cursor_col = 0; }
        break;
    case KEY_ARROW_UP:
        clear_selection(E);
        if (E->cursor_row > 0) { E->cursor_row--; cur = get_line_at(E, E->cursor_row); if (E->cursor_col > cur->len) E->cursor_col = cur->len; }
        break;
    case KEY_ARROW_DOWN:
        clear_selection(E);
        if (E->cursor_row < E->num_lines - 1) { E->cursor_row++; cur = get_line_at(E, E->cursor_row); if (E->cursor_col > cur->len) E->cursor_col = cur->len; }
        break;
    case KEY_HOME: clear_selection(E); E->cursor_col = 0; break;
    case KEY_END: clear_selection(E); E->cursor_col = cur->len; break;
    case KEY_PAGE_UP:
        clear_selection(E);
        { int text_rows = E->screen_rows - TOOLBOX_HEIGHT - 1;
          E->cursor_row -= text_rows; if (E->cursor_row < 0) E->cursor_row = 0;
          cur = get_line_at(E, E->cursor_row); if (E->cursor_col > cur->len) E->cursor_col = cur->len; }
        break;
    case KEY_PAGE_DOWN:
        clear_selection(E);
        { int text_rows = E->screen_rows - TOOLBOX_HEIGHT - 1;
          E->cursor_row += text_rows; if (E->cursor_row >= E->num_lines) E->cursor_row = E->num_lines - 1;
          cur = get_line_at(E, E->cursor_row); if (E->cursor_col > cur->len) E->cursor_col = cur->len; }
        break;

    /* ─── Enter ─── */
    case KEY_ENTER: {
        if (E->sel.active) delete_selection(E);
        cur = get_line_at(E, E->cursor_row);
        push_undo(E, UNDO_SPLIT_LINE, E->cursor_row, E->cursor_col, NULL, 0, 0, 0, NULL, 0);
        Line *new_l = create_line();
        if (E->cursor_col < cur->len) {
            line_insert_text(new_l, 0, cur->text + E->cursor_col, cur->len - E->cursor_col);
            cur->len = E->cursor_col; cur->text[cur->len] = '\0';
        }
        insert_line_after(E, cur, new_l);
        E->cursor_row++; E->cursor_col = 0; E->modified = 1;
        break;
    }

    /* ─── Backspace ─── */
    case KEY_BACKSPACE: {
        if (E->sel.active) { delete_selection(E); break; }
        if (E->cursor_col > 0) {
            int prev_pos = prev_utf8_char_start(cur->text, E->cursor_col);
            int ch_len = E->cursor_col - prev_pos;
            push_undo(E, UNDO_DELETE_CHAR, E->cursor_row, prev_pos, cur->text + prev_pos, ch_len, 0, 0, NULL, 0);
            line_delete_char(cur, prev_pos, ch_len);
            E->cursor_col = prev_pos; E->modified = 1;
        } else if (E->cursor_row > 0) {
            Line *prev_l = get_line_at(E, E->cursor_row - 1);
            int merge_col = prev_l->len;
            push_undo(E, UNDO_MERGE_LINE, E->cursor_row - 1, merge_col, NULL, 0, 0, 0, NULL, 0);
            line_insert_text(prev_l, prev_l->len, cur->text, cur->len);
            remove_line(E, cur); free_line(cur);
            E->cursor_row--; E->cursor_col = merge_col; E->modified = 1;
        }
        break;
    }

    /* ─── Delete key ─── */
    case KEY_DELETE: {
        if (E->sel.active) { delete_selection(E); break; }
        if (E->cursor_col < cur->len) {
            int ch_len = utf8_char_len((unsigned char)cur->text[E->cursor_col]);
            push_undo(E, UNDO_DELETE_CHAR, E->cursor_row, E->cursor_col, cur->text + E->cursor_col, ch_len, 0, 0, NULL, 0);
            line_delete_char(cur, E->cursor_col, ch_len);
            E->modified = 1;
        } else if (cur->next) {
            Line *next_l = cur->next;
            push_undo(E, UNDO_MERGE_LINE, E->cursor_row, cur->len, NULL, 0, 0, 0, NULL, 0);
            line_insert_text(cur, cur->len, next_l->text, next_l->len);
            remove_line(E, next_l); free_line(next_l);
            E->modified = 1;
        }
        break;
    }

    /* ─── CTRL+Backspace (word delete) ─── */
    case CTRL_BACKSPACE: {
        if (E->cursor_col > 0) {
            int new_col = word_start_before(cur->text, E->cursor_col);
            int del_len = E->cursor_col - new_col;
            push_undo(E, UNDO_DELETE_CHAR, E->cursor_row, new_col, cur->text + new_col, del_len, 0, 0, NULL, 0);
            memmove(cur->text + new_col, cur->text + E->cursor_col, cur->len - E->cursor_col);
            cur->len -= del_len; cur->text[cur->len] = '\0';
            E->cursor_col = new_col; E->modified = 1;
        }
        break;
    }

    /* ─── Regular character input ─── */
    default:
        if (key >= 32) {
            if (E->sel.active) delete_selection(E);
            cur = get_line_at(E, E->cursor_row);
            char ch_buf[8]; int cl = utf8_char_len((unsigned char)key);
            ch_buf[0] = (char)key;
            if (cl > 1) { for (int j = 1; j < cl; j++) { char cb; if (read(STDIN_FILENO, &cb, 1) == 1) ch_buf[j] = cb; else ch_buf[j] = 0; } }
            ch_buf[cl] = '\0';
            push_undo(E, UNDO_INSERT_CHAR, E->cursor_row, E->cursor_col, ch_buf, cl, 0, 0, NULL, 0);
            line_insert_char(cur, E->cursor_col, ch_buf, cl);
            E->cursor_col += cl; E->modified = 1;
        }
        break;
    }
}

/* ════════════════════════════════════════════
 *  Init / Free / Main
 * ════════════════════════════════════════════ */

void init_editor(Editor *E) {
    memset(E, 0, sizeof(Editor));
    E->head = create_line();
    E->num_lines = 1;
    E->running = 1;
    E->current_match = -1;
    get_terminal_size(&E->screen_rows, &E->screen_cols);
}

void free_editor(Editor *E) {
    Line *l = E->head;
    while (l) { Line *nx = l->next; free_line(l); l = nx; }
    free_undo_stack(E);
    free(E->clipboard);
    free(E->matches);
}

static Editor g_ed;

static void handle_sigwinch(int sig) {
    (void)sig;
    get_terminal_size(&g_ed.screen_rows, &g_ed.screen_cols);
}

int main(void) {
    setlocale(LC_ALL, "");
    init_editor(&g_ed);
    enable_raw_mode(&g_ed);
    signal(SIGWINCH, handle_sigwinch);

    while (g_ed.running) {
        render_screen(&g_ed);
        process_keypress(&g_ed);
    }

    disable_raw_mode(&g_ed);
    write(STDOUT_FILENO, "\033[2J\033[H", 7);
    free_editor(&g_ed);
    return 0;
}
