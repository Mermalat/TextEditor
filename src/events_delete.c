#include "editor.h"

static void	handle_backspace(t_line *cur)
{
	int		pp;
	int		cl;
	t_line	*pl;
	int		mc;

	if (g_ed.sel.active) { delete_selection(); return ; }
	if (g_ed.cursor_col > 0)
	{
		pp = prev_utf8_char_start(cur->text, g_ed.cursor_col);
		cl = g_ed.cursor_col - pp;
		push_undo(UNDO_DELETE_CHAR, (t_pos){g_ed.cursor_row, pp}, (t_str){cur->text + pp, cl}, (t_pos){0, 0});
		line_delete_char(cur, pp, cl); g_ed.cursor_col = pp; g_ed.modified = 1;
	}
	else if (g_ed.cursor_row > 0)
	{
		pl = get_line_at(g_ed.cursor_row - 1); mc = pl->len;
		push_undo(UNDO_MERGE_LINE, (t_pos){g_ed.cursor_row - 1, mc}, (t_str){NULL, 0}, (t_pos){0, 0});
		line_insert_text(pl, pl->len, cur->text, cur->len);
		remove_line(cur); free_line(cur);
		g_ed.cursor_row--; g_ed.cursor_col = mc; g_ed.modified = 1;
	}
}

static void	handle_delete(t_line *cur)
{
	int		cl;
	t_line	*nl;

	if (g_ed.sel.active) { delete_selection(); return ; }
	if (g_ed.cursor_col < cur->len)
	{
		cl = utf8_char_len((unsigned char)cur->text[g_ed.cursor_col]);
		push_undo(UNDO_DELETE_CHAR, (t_pos){g_ed.cursor_row, g_ed.cursor_col},
			(t_str){cur->text + g_ed.cursor_col, cl}, (t_pos){0, 0});
		line_delete_char(cur, g_ed.cursor_col, cl);
		g_ed.modified = 1;
	}
	else if (cur->next)
	{
		nl = cur->next;
		push_undo(UNDO_MERGE_LINE, (t_pos){g_ed.cursor_row, cur->len}, (t_str){NULL, 0}, (t_pos){0, 0});
		line_insert_text(cur, cur->len, nl->text, nl->len);
		remove_line(nl); free_line(nl);
		g_ed.modified = 1;
	}
}

static void	handle_ctrl_backspace(t_line *cur)
{
	int	nc;
	int	dl;

	if (g_ed.cursor_col > 0)
	{
		nc = word_start_before(cur->text, g_ed.cursor_col);
		dl = g_ed.cursor_col - nc;
		push_undo(UNDO_DELETE_CHAR, (t_pos){g_ed.cursor_row, nc},
			(t_str){cur->text + nc, dl}, (t_pos){0, 0});
		memmove(cur->text + nc, cur->text + g_ed.cursor_col,
			cur->len - g_ed.cursor_col);
		cur->len -= dl;
		cur->text[cur->len] = '\0';
		g_ed.cursor_col = nc;
		g_ed.modified = 1;
	}
}

void	handle_deletion(int key)
{
	t_line	*cur;

	cur = get_line_at(g_ed.cursor_row);
	if (key == KEY_BACKSPACE)
		handle_backspace(cur);
	else if (key == KEY_DELETE)
		handle_delete(cur);
	else if (key == CTRL_BACKSPACE)
		handle_ctrl_backspace(cur);
}
