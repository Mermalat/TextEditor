#include "editor.h"

void	clear_selection(void)
{
	g_ed.sel.active = 0;
}

void	start_selection(void)
{
	if (!g_ed.sel.active)
	{
		g_ed.sel.active = 1;
		g_ed.sel.start_row = g_ed.cursor_row;
		g_ed.sel.start_col = g_ed.cursor_col;
	}
	g_ed.sel.end_row = g_ed.cursor_row;
	g_ed.sel.end_col = g_ed.cursor_col;
}

void	update_selection(void)
{
	g_ed.sel.end_row = g_ed.cursor_row;
	g_ed.sel.end_col = g_ed.cursor_col;
}

void	normalize_selection(int *sr, int *sc, int *er, int *ec)
{
	t_selection	*s;

	s = &g_ed.sel;
	if (s->start_row < s->end_row || (s->start_row == s->end_row
			&& s->start_col <= s->end_col))
	{
		*sr = s->start_row;
		*sc = s->start_col;
		*er = s->end_row;
		*ec = s->end_col;
	}
	else
	{
		*sr = s->end_row;
		*sc = s->end_col;
		*er = s->start_row;
		*ec = s->start_col;
	}
}

int	is_in_selection(int row, int byte_col)
{
	int	sr;
	int	sc;
	int	er;
	int	ec;

	if (!g_ed.sel.active)
		return (0);
	normalize_selection(&sr, &sc, &er, &ec);
	if (row < sr || row > er)
		return (0);
	if (row == sr && row == er)
		return (byte_col >= sc && byte_col < ec);
	if (row == sr)
		return (byte_col >= sc);
	if (row == er)
		return (byte_col < ec);
	return (1);
}
