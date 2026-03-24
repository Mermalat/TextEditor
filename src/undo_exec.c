#include "editor.h"

static void	undo_split_line(t_undo_action *a)
{
	t_line	*cur;
	t_line	*nxt;

	cur = get_line_at(a->row);
	nxt = get_line_at(a->row + 1);
	if (cur && nxt)
	{
		line_insert_text(cur, cur->len, nxt->text, nxt->len);
		remove_line(nxt);
		free_line(nxt);
	}
	g_ed.cursor_row = a->row;
	g_ed.cursor_col = a->col;
}

static void	undo_merge_line(t_undo_action *a)
{
	t_line	*ln;
	t_line	*nl;
	int		sp;

	ln = get_line_at(a->row);
	if (!ln)
		return ;
	nl = create_line();
	sp = a->col;
	if (sp < ln->len)
	{
		line_insert_text(nl, 0, ln->text + sp, ln->len - sp);
		ln->len = sp;
		ln->text[ln->len] = '\0';
	}
	insert_line_after(ln, nl);
	g_ed.cursor_row = a->row + 1;
	g_ed.cursor_col = 0;
}

static void	undo_del_sel_char(t_undo_action *a, int *r, int *c, int *i)
{
	t_line	*cl;
	int		clen;

	clen = utf8_char_len((unsigned char)a->data[*i]);
	cl = get_line_at(*r);
	line_insert_char(cl, *c, &a->data[*i], clen);
	*c += clen;
	*i += clen - 1;
}

static void	undo_delete_selection(t_undo_action *a)
{
	int		r;
	int		c;
	int		i;
	t_line	*cl;
	t_line	*nl;

	if (!a->data) return ;
	r = a->row; c = a->col; i = -1;
	while (++i < a->data_len)
	{
		if (a->data[i] == '\n')
		{
			cl = get_line_at(r);
			nl = create_line();
			if (c < cl->len)
			{
				line_insert_text(nl, 0, cl->text + c, cl->len - c);
				cl->len = c; cl->text[cl->len] = '\0';
			}
			insert_line_after(cl, nl);
			r++; c = 0;
		}
		else
			undo_del_sel_char(a, &r, &c, &i);
	}
	g_ed.cursor_row = a->row; g_ed.cursor_col = a->col;
}

static void	undo_paste(t_undo_action *a)
{
	t_line	*sl;
	t_line	*el;
	int		i;

	if (!a->data) return ;
	if (a->row == a->extra_row)
	{
		sl = get_line_at(a->row);
		if (sl && a->extra_col > a->col)
		{
			memmove(sl->text + a->col, sl->text + a->extra_col, sl->len - a->extra_col);
			sl->len -= (a->extra_col - a->col); sl->text[sl->len] = '\0';
		}
	}
	else
	{
		sl = get_line_at(a->row); el = get_line_at(a->extra_row);
		if (sl && el)
		{
			sl->len = a->col; sl->text[sl->len] = '\0';
			line_insert_text(sl, sl->len, el->text + a->extra_col, el->len - a->extra_col);
			i = a->extra_row;
			while (i > a->row)
			{
				el = get_line_at(a->row + 1);
				if (el) { remove_line(el); free_line(el); }
				i--;
			}
		}
	}
	g_ed.cursor_row = a->row; g_ed.cursor_col = a->col;
}

void	execute_complex_undo(t_undo_action *a)
{
	if (a->type == UNDO_SPLIT_LINE)
		undo_split_line(a);
	else if (a->type == UNDO_MERGE_LINE)
		undo_merge_line(a);
	else if (a->type == UNDO_DELETE_SELECTION)
		undo_delete_selection(a);
	else if (a->type == UNDO_PASTE)
		undo_paste(a);
}
