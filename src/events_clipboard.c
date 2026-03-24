#include "editor.h"

static void	paste_newline(void)
{
	t_line	*cur;
	t_line	*nl;

	cur = get_line_at(g_ed.cursor_row);
	nl = create_line();
	if (g_ed.cursor_col < cur->len)
	{
		line_insert_text(nl, 0, cur->text + g_ed.cursor_col,
			cur->len - g_ed.cursor_col);
		cur->len = g_ed.cursor_col;
		cur->text[cur->len] = '\0';
	}
	insert_line_after(cur, nl);
	g_ed.cursor_row++;
	g_ed.cursor_col = 0;
}

static void	paste_char(int *i)
{
	int		clen;
	t_line	*cur;

	clen = utf8_char_len((unsigned char)g_ed.clipboard[*i]);
	cur = get_line_at(g_ed.cursor_row);
	line_insert_char(cur, g_ed.cursor_col, &g_ed.clipboard[*i], clen);
	g_ed.cursor_col += clen;
	*i += clen - 1;
}

void	paste(void)
{
	int	sr;
	int	sc;
	int	i;

	if (!g_ed.clipboard || g_ed.clipboard_len == 0)
		return ;
	if (g_ed.sel.active)
		delete_selection();
	sr = g_ed.cursor_row;
	sc = g_ed.cursor_col;
	i = -1;
	while (++i < g_ed.clipboard_len)
	{
		if (g_ed.clipboard[i] == '\n')
			paste_newline();
		else
			paste_char(&i);
	}
	push_undo(UNDO_PASTE, (t_pos){sr, sc},
		(t_str){g_ed.clipboard, g_ed.clipboard_len},
		(t_pos){g_ed.cursor_row, g_ed.cursor_col});
	g_ed.modified = 1;
}

void	handle_clipboard_ops(int key)
{
	if (key == CTRL_C_KEY)
		copy_selection();
	else if (key == CTRL_X)
		cut_selection();
	else if (key == CTRL_V)
		paste();
}

void	handle_search_ops(int key)
{
	if (key == CTRL_F)
		search_text();
	else if (key == CTRL_G)
		navigate_matches();
	else if (key == CTRL_H)
		replace_all();
}
