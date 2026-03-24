#ifndef EDITOR_H
# define EDITOR_H

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <termios.h>
# include <sys/ioctl.h>
# include <ctype.h>
# include <dirent.h>
# include <sys/stat.h>
# include <locale.h>
# include <wchar.h>
# include <errno.h>

# define TOOLBOX_HEIGHT		3
# define LINE_NUM_WIDTH		5
# define INITIAL_LINE_CAP	128
# define CLIPBOARD_CAP		4096
# define MAX_SEARCH_LEN		256
# define MAX_FILENAME		512
# define MAX_PATH_LEN		1024

enum e_key {
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

typedef enum e_undo_type {
	UNDO_INSERT_CHAR,
	UNDO_DELETE_CHAR,
	UNDO_INSERT_LINE,
	UNDO_DELETE_LINE,
	UNDO_MERGE_LINE,
	UNDO_SPLIT_LINE,
	UNDO_REPLACE_ALL,
	UNDO_DELETE_SELECTION,
	UNDO_PASTE
}	t_undo_type;

typedef struct s_line {
	char			*text;
	int				len;
	int				cap;
	struct s_line	*prev;
	struct s_line	*next;
}	t_line;

typedef struct s_pos {
	int	r;
	int	c;
}	t_pos;

typedef struct s_str {
	const char	*s;
	int			len;
}	t_str;

typedef struct s_buf {
	char	*s;
	int		len;
	int		cap;
}	t_buf;

typedef struct s_undo_action {
	t_undo_type				type;
	int						row;
	int						col;
	char					*data;
	int						data_len;
	int						extra_row;
	int						extra_col;
	char					*extra_data;
	int						extra_data_len;
	struct s_undo_action	*next;
}	t_undo_action;

typedef struct s_search_match {
	int	row;
	int	col;
	int	len;
}	t_search_match;

typedef struct s_selection {
	int	active;
	int	start_row;
	int	start_col;
	int	end_row;
	int	end_col;
}	t_selection;

typedef struct s_editor {
	t_line			*head;
	int				num_lines;
	int				cursor_row;
	int				cursor_col;
	int				scroll_row;
	int				scroll_col;
	int				screen_rows;
	int				screen_cols;
	char			filename[MAX_FILENAME];
	int				modified;
	int				running;
	t_selection		sel;
	char			*clipboard;
	int				clipboard_len;
	t_search_match	*matches;
	int				match_count;
	int				current_match;
	int				search_active;
	char			search_query[MAX_SEARCH_LEN];
	t_undo_action	*undo_stack;
	struct termios	orig_termios;
}	t_editor;

extern t_editor	g_ed;

/* Terminal */
void	enable_raw_mode(void);
void	disable_raw_mode(void);
int		read_key(void);
void	get_terminal_size(int *rows, int *cols);
void	die(const char *s);

/* Line management */
t_line	*create_line(void);
void	free_line(t_line *line);
t_line	*get_line_at(int index);
void	insert_line_after(t_line *after, t_line *new_line);
void	remove_line(t_line *line);
void	line_insert_char(t_line *line, int pos, const char *ch, int ch_len);
void	line_delete_char(t_line *line, int pos, int ch_len);
void	line_insert_text(t_line *line, int pos, const char *text, int text_len);
void	ensure_capacity(t_line *line, int needed);

/* UTF-8 Utilities */
int		utf8_char_len(unsigned char c);
int		prev_utf8_char_start(const char *text, int pos);
int		next_utf8_char_end(const char *text, int len, int pos);
int		byte_to_display_col(const char *text, int byte_pos);
int		display_to_byte_col(const char *text, int len, int dcol);
int		word_start_before(const char *text, int pos);
int		word_end_after(const char *text, int len, int pos);

/* Rendering */
void	render_screen(void);
void	render_toolbox(void);
void	set_cursor_position(int row, int col);
void	render_status_bar(void);
void	render_lines(void);

void	print_padding(int len);

/* File I/O */
void	open_file(void);
void	save_file(void);
void	save_as_file(void);
void	file_browser(char *result_path, int save_mode);
void	render_browser_screen(char *cwd, char entries[512][256], int *is_dir, int count, int selected, int save_mode);

/* Prompt */
int		prompt_input(const char *prompt, char *buf, int buf_size);

/* Selection */
void	start_selection(void);
void	update_selection(void);
void	clear_selection(void);
char	*get_selected_text(int *out_len);
void	delete_selection(void);
void	normalize_selection(int *sr, int *sc, int *er, int *ec);
int		is_in_selection(int row, int byte_col);

/* Clipboard */
void	copy_selection(void);
void	cut_selection(void);
void	paste(void);

/* Search */
void	search_text(void);
void	navigate_matches(void);
void	replace_all(void);
void	clear_search(void);
void	find_matches(const char *query);
int		is_search_match(int row, int byte_col);

/* Undo */
void	push_undo(t_undo_type t, t_pos p, t_str s, t_pos p_ex);
void	undo(void);
void	free_undo_stack(void);
void	execute_complex_undo(t_undo_action *a);

/* Events & Main */
void	process_keypress(void);
void	try_exit(void);
void	init_editor(void);
void	free_editor(void);

/* Event Dispatchers */
void	handle_file_ops(int key);
void	handle_search_ops(int key);
void	handle_clipboard_ops(int key);
void	handle_movement(int key);
void	handle_selection_movement(int key);
void	handle_deletion(int key);

#endif
