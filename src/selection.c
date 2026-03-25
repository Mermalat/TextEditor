#include "editor.h"

static void	append_to_buf(t_buf *b, t_line *l, int *bounds)
{
	int	seg;

	seg = bounds[1] - bounds[0];
	if (b->len + seg + 2 > b->cap)
	{
		b->cap = (b->len + seg + 2) * 2;
		b->s = realloc(b->s, b->cap);
	}
	memcpy(b->s + b->len, l->text + bounds[0], seg);
	b->len += seg;
	if (bounds[2] < bounds[3])
	{
		b->s[b->len] = '\n';
		(b->len)++;
	}
}

char	*get_selected_text(int *out_len)
{
	int		b[4];
	int		r;
	t_buf	buf;
	t_line	*l;
	int		bd[4];

	if (!g_ed.sel.active)
	{
		*out_len = 0;
		return (NULL);
	}
	normalize_selection(&b[0], &b[1], &b[2], &b[3]);
	buf.cap = 1024; buf.len = 0; buf.s = malloc(buf.cap);
	r = b[0];
	while (r <= b[2])
	{
		l = get_line_at(r);
		if (l)
		{
			bd[0] = (r == b[0]) ? b[1] : 0;
			bd[1] = (r == b[2]) ? b[3] : l->len;
			bd[2] = r; bd[3] = b[2];
			append_to_buf(&buf, l, bd);
		}
		r++;
	}
	buf.s[buf.len] = '\0';
	*out_len = buf.len;
	return (buf.s);
}

static const char	g_b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void	osc52_write_chunk(unsigned char *n3, int chunk)
{
	unsigned char	n4[4];
	int				j;

	n4[0] = (n3[0] & 0xfc) >> 2;
	n4[1] = ((n3[0] & 0x03) << 4) + ((n3[1] & 0xf0) >> 4);
	n4[2] = ((n3[1] & 0x0f) << 2) + ((n3[2] & 0xc0) >> 6);
	n4[3] = n3[2] & 0x3f;
	if (chunk == 1)
	{
		n4[2] = 64;
		n4[3] = 64;
	}
	else if (chunk == 2)
		n4[3] = 64;
	j = -1;
	while (++j < 4)
	{
		if (n4[j] == 64)
			write(STDOUT_FILENO, "=", 1);
		else
			write(STDOUT_FILENO, &g_b64[n4[j]], 1);
	}
}

static void	osc52_copy(const char *data, int len)
{
	int				i;
	unsigned char	n3[3];

	write(STDOUT_FILENO, "\033]52;c;", 7);
	i = 0;
	while (i < len)
	{
		n3[0] = data[i++];
		n3[1] = 0;
		if (i < len)
			n3[1] = data[i++];
		n3[2] = 0;
		if (i < len)
			n3[2] = data[i++];
		osc52_write_chunk(n3, len - (i - 3));
	}
	write(STDOUT_FILENO, "\a", 1);
}

void	copy_selection(void)
{
	if (!g_ed.sel.active)
		return ;
	free(g_ed.clipboard);
	g_ed.clipboard = get_selected_text(&g_ed.clipboard_len);
	if (g_ed.clipboard)
		osc52_copy(g_ed.clipboard, g_ed.clipboard_len);
}

static void	del_sel_multiline(int sr, int sc, int er, int ec)
{
	t_line	*sl;
	t_line	*el;
	int		i;

	sl = get_line_at(sr);
	el = get_line_at(er);
	if (sl && el)
	{
		sl->len = sc;
		sl->text[sl->len] = '\0';
		line_insert_text(sl, sl->len, el->text + ec, el->len - ec);
		i = er;
		while (i > sr)
		{
			el = get_line_at(sr + 1);
			if (el)
			{
				remove_line(el);
				free_line(el);
			}
			i--;
		}
	}
}

void	delete_selection(void)
{
	int		bd[4];
	char	*st;
	t_line	*l;
	int		st_len;

	if (!g_ed.sel.active)
		return ;
	normalize_selection(&bd[0], &bd[1], &bd[2], &bd[3]);
	st = get_selected_text(&st_len);
	push_undo(UNDO_DELETE_SELECTION, (t_pos){bd[0], bd[1]},
		(t_str){st, st_len}, (t_pos){bd[2], bd[3]});
	free(st);
	if (bd[0] == bd[2])
	{
		l = get_line_at(bd[0]);
		if (l && bd[3] > bd[1])
		{
			memmove(l->text + bd[1], l->text + bd[3], l->len - bd[3]);
			l->len -= (bd[3] - bd[1]);
			l->text[l->len] = '\0';
		}
	}
	else
		del_sel_multiline(bd[0], bd[1], bd[2], bd[3]);
	g_ed.cursor_row = bd[0]; g_ed.cursor_col = bd[1]; g_ed.sel.active = 0; g_ed.modified = 1;
}

void	cut_selection(void)
{
	copy_selection();
	delete_selection();
}
