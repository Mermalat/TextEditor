#include "editor.h"

void	try_exit(void)
{
	int			key;
	const char	*msg;

	if (g_ed.modified)
	{
		set_cursor_position(g_ed.screen_rows, 1);
		write(STDOUT_FILENO, "\033[K", 3);
		msg = "\033[7m Save changes? (y/n/ESC) \033[0m";
		write(STDOUT_FILENO, msg, strlen(msg));
		while (1)
		{
			key = read_key();
			if (key == 'y' || key == 'Y')
			{
				save_file();
				g_ed.running = 0;
				return ;
			}
			else if (key == 'n' || key == 'N')
			{
				g_ed.running = 0;
				return ;
			}
			else if (key == KEY_ESC)
				return ;
		}
	}
	g_ed.running = 0;
}

static void	handle_regular_char(int key)
{
	t_line	*c;
	char	b[8];
	int		cl;
	int		j;

	if (key >= 32)
	{
		if (g_ed.sel.active) delete_selection();
		c = get_line_at(g_ed.cursor_row);
		cl = utf8_char_len((unsigned char)key); b[0] = (char)key;
		if (cl > 1)
		{
			j = 0;
			while (++j < cl)
			{
				if (read(STDIN_FILENO, &b[j], 1) != 1) b[j] = 0;
			}
		}
		b[cl] = '\0';
		push_undo(UNDO_INSERT_CHAR, (t_pos){g_ed.cursor_row, g_ed.cursor_col},
			(t_str){b, cl}, (t_pos){0, 0});
		line_insert_char(c, g_ed.cursor_col, b, cl);
		g_ed.cursor_col += cl; g_ed.modified = 1;
	}
}

static void	handle_enter(void)
{
	t_line	*c;
	t_line	*n;

	if (g_ed.sel.active)
		delete_selection();
	c = get_line_at(g_ed.cursor_row);
	push_undo(UNDO_SPLIT_LINE, (t_pos){g_ed.cursor_row, g_ed.cursor_col},
		(t_str){NULL, 0}, (t_pos){0, 0});
	n = create_line();
	if (g_ed.cursor_col < c->len)
	{
		line_insert_text(n, 0, c->text + g_ed.cursor_col,
			c->len - g_ed.cursor_col);
		c->len = g_ed.cursor_col;
		c->text[c->len] = '\0';
	}
	insert_line_after(c, n);
	g_ed.cursor_row++;
	g_ed.cursor_col = 0;
	g_ed.modified = 1;
}

void	process_keypress(void)
{
	int		k;
	t_line	*c;

	k = read_key(); c = get_line_at(g_ed.cursor_row);
	if (!c) return ;
	if (k != CTRL_F && k != CTRL_G && g_ed.search_active &&
		k != KEY_ARROW_UP && k != KEY_ARROW_DOWN &&
		k != KEY_ARROW_LEFT && k != KEY_ARROW_RIGHT)
		clear_search();
	if (k == KEY_ESC) try_exit();
	else if (k == CTRL_O || k == CTRL_S || k == CTRL_SHIFT_S)
		handle_file_ops(k);
	else if (k == CTRL_F || k == CTRL_G || k == CTRL_H)
		handle_search_ops(k);
	else if (k == CTRL_C_KEY || k == CTRL_X || k == CTRL_V)
		handle_clipboard_ops(k);
	else if (k == CTRL_Z) undo();
	else if (k == KEY_BACKSPACE || k == KEY_DELETE || k == CTRL_BACKSPACE)
		handle_deletion(k);
	else if (k == KEY_ENTER) handle_enter();
	else if (k >= KEY_ARROW_UP && k <= KEY_PAGE_DOWN) handle_movement(k);
	else if (k >= CTRL_ARROW_LEFT && k <= CTRL_SHIFT_DOWN)
		handle_selection_movement(k);
	else handle_regular_char(k);
}
