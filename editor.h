#ifndef EDITOR_H
#define EDITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <locale.h>
#include <wchar.h>
#include <errno.h>

/* ─── Constants ─── */
#define TOOLBOX_HEIGHT    3
#define LINE_NUM_WIDTH    5
#define INITIAL_LINE_CAP  128
#define CLIPBOARD_CAP     4096
#define MAX_SEARCH_LEN    256
#define MAX_FILENAME      512
#define MAX_PATH_LEN      1024

/* ─── Key Codes ─── */
enum Key {
    KEY_NULL       = 0,
    KEY_ENTER      = 13,
    KEY_ESC        = 27,
    KEY_BACKSPACE  = 127,
    KEY_ARROW_UP   = 1000,
    KEY_ARROW_DOWN,
    KEY_ARROW_LEFT,
    KEY_ARROW_RIGHT,
    KEY_HOME,
    KEY_END,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_DELETE,
    /* Ctrl combos */
    CTRL_F         = 6,
    CTRL_G         = 7,
    CTRL_H         = 8,
    CTRL_C_KEY     = 3,
    CTRL_V         = 22,
    CTRL_X         = 24,
    CTRL_Z         = 26,
    CTRL_S         = 19,
    CTRL_O         = 15,
    CTRL_BACKSPACE = 2000,
    /* Selection keys */
    CTRL_ARROW_LEFT   = 2100,
    CTRL_ARROW_RIGHT  = 2101,
    CTRL_ARROW_UP     = 2102,
    CTRL_ARROW_DOWN   = 2103,
    CTRL_SHIFT_LEFT   = 2200,
    CTRL_SHIFT_RIGHT  = 2201,
    CTRL_SHIFT_UP     = 2202,
    CTRL_SHIFT_DOWN   = 2203,
    CTRL_SHIFT_S      = 2300
};

/* ─── Undo Action Types ─── */
typedef enum {
    UNDO_INSERT_CHAR,
    UNDO_DELETE_CHAR,
    UNDO_INSERT_LINE,
    UNDO_DELETE_LINE,
    UNDO_MERGE_LINE,
    UNDO_SPLIT_LINE,
    UNDO_REPLACE_ALL,
    UNDO_DELETE_SELECTION,
    UNDO_PASTE
} UndoType;

/* ─── Line Node (Doubly Linked List) ─── */
typedef struct Line {
    char            *text;      /* UTF-8 text buffer                 */
    int              len;       /* byte length of text               */
    int              cap;       /* allocated capacity                */
    struct Line     *prev;
    struct Line     *next;
} Line;

/* ─── Undo Action ─── */
typedef struct UndoAction {
    UndoType          type;
    int               row;
    int               col;      /* byte offset in line               */
    char             *data;     /* saved text (old content)          */
    int               data_len;
    int               extra_row;
    int               extra_col;
    char             *extra_data;
    int               extra_data_len;
    struct UndoAction *next;    /* stack link                        */
} UndoAction;

/* ─── Search Match ─── */
typedef struct {
    int row;
    int col;       /* byte offset */
    int len;       /* byte length of match */
} SearchMatch;

/* ─── Selection ─── */
typedef struct {
    int active;
    int start_row, start_col;   /* byte offsets */
    int end_row,   end_col;
} Selection;

/* ─── Editor State ─── */
typedef struct {
    Line       *head;           /* first line                        */
    int         num_lines;
    int         cursor_row;     /* 0-indexed                         */
    int         cursor_col;     /* byte offset in line               */
    int         scroll_row;     /* first visible line (0-indexed)    */
    int         scroll_col;
    int         screen_rows;
    int         screen_cols;
    char        filename[MAX_FILENAME];
    int         modified;
    int         running;

    /* Selection */
    Selection   sel;

    /* Clipboard */
    char       *clipboard;
    int         clipboard_len;

    /* Search */
    SearchMatch *matches;
    int          match_count;
    int          current_match;
    int          search_active;
    char         search_query[MAX_SEARCH_LEN];

    /* Undo stack */
    UndoAction  *undo_stack;

    /* Original terminal settings */
    struct termios orig_termios;
} Editor;

/* ─── Function Prototypes ─── */

/* Terminal */
void   enable_raw_mode(Editor *E);
void   disable_raw_mode(Editor *E);
int    read_key(void);
void   get_terminal_size(int *rows, int *cols);

/* Line management */
Line  *create_line(void);
void   free_line(Line *line);
Line  *get_line_at(Editor *E, int index);
void   insert_line_after(Editor *E, Line *after, Line *new_line);
void   remove_line(Editor *E, Line *line);

/* Text operations */
void   line_insert_char(Line *line, int pos, const char *ch, int ch_len);
void   line_delete_char(Line *line, int pos, int ch_len);
void   line_insert_text(Line *line, int pos, const char *text, int text_len);

/* Cursor helpers */
int    utf8_char_len(unsigned char c);
int    prev_utf8_char_start(const char *text, int pos);
int    next_utf8_char_end(const char *text, int len, int pos);
int    byte_to_display_col(const char *text, int byte_pos);
int    word_start_before(const char *text, int pos);
int    word_end_after(const char *text, int len, int pos);

/* Rendering */
void   render_screen(Editor *E);
void   render_toolbox(Editor *E);
void   set_cursor_position(int row, int col);

/* File operations */
void   file_browser(Editor *E, char *result_path, int save_mode);
void   open_file(Editor *E);
void   save_file(Editor *E);
void   save_as_file(Editor *E);

/* Selection */
void   start_selection(Editor *E);
void   update_selection(Editor *E);
void   clear_selection(Editor *E);
char  *get_selected_text(Editor *E, int *out_len);
void   delete_selection(Editor *E);

/* Clipboard */
void   copy_selection(Editor *E);
void   cut_selection(Editor *E);
void   paste(Editor *E);

/* Search & Replace */
void   search_text(Editor *E);
void   navigate_matches(Editor *E);
void   replace_all(Editor *E);
void   clear_search(Editor *E);

/* Undo */
void   push_undo(Editor *E, UndoType type, int row, int col,
                 const char *data, int data_len,
                 int extra_row, int extra_col,
                 const char *extra_data, int extra_data_len);
void   undo(Editor *E);
void   free_undo_stack(Editor *E);

/* Editor lifecycle */
void   init_editor(Editor *E);
void   free_editor(Editor *E);
void   process_keypress(Editor *E);
void   try_exit(Editor *E);

#endif /* EDITOR_H */
