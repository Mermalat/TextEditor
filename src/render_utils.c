#include "editor.h"

void	print_padding(int len)
{
	int	i;

	i = 0;
	while (i++ < len)
		write(STDOUT_FILENO, " ", 1);
}

void	render_status_bar(void)
{
	t_line	*c;
	char	status[512];
	int		sln;
	int		dcol;
	char	*f;

	set_cursor_position(g_ed.screen_rows, 1);
	write(STDOUT_FILENO, "\033[K\033[7m", 7);
	c = get_line_at(g_ed.cursor_row);
	dcol = 0;
	if (c)
		dcol = byte_to_display_col(c->text, g_ed.cursor_col);
	f = "[New File]";
	if (g_ed.filename[0])
		f = g_ed.filename;
	sln = snprintf(status, sizeof(status), " %s%s | Line: %d/%d | Col: %d ",
			f, g_ed.modified ? " [*]" : "", g_ed.cursor_row + 1,
			g_ed.num_lines, dcol + 1);
	write(STDOUT_FILENO, status, sln);
	if (g_ed.screen_cols - sln > 0)
		print_padding(g_ed.screen_cols - sln);
	write(STDOUT_FILENO, "\033[0m", 4);
	set_cursor_position(TOOLBOX_HEIGHT + 1 + (g_ed.cursor_row - g_ed.scroll_row),
		LINE_NUM_WIDTH + 1 + dcol);
}

static void	prt_char(t_line *ln, int idx, int *by)
{
	int	cl;
	int	sel;
	int	srch;

	cl = utf8_char_len((unsigned char)ln->text[*by]);
	sel = is_in_selection(idx, *by);
	srch = is_search_match(idx, *by);
	if (srch == 2)
		write(STDOUT_FILENO, "\033[1;37;42m", 10);
	else if (srch == 1)
		write(STDOUT_FILENO, "\033[44m", 5);
	else if (sel)
		write(STDOUT_FILENO, "\033[43m", 5);
	write(STDOUT_FILENO, ln->text + *by, cl);
	if (sel || srch)
		write(STDOUT_FILENO, "\033[0m", 4);
	*by += cl;
}

static void	prt_line(t_line *line, int idx)
{
	char	nbuf[32];
	int		nln;
	int		b;

	nln = snprintf(nbuf, sizeof(nbuf), "\033[33m%4d\033[90m|\033[0m", idx + 1);
	write(STDOUT_FILENO, nbuf, nln);
	b = 0;
	while (b < line->len)
		prt_char(line, idx, &b);
}

void	render_lines(void)
{
	t_line	*ln;
	int		i;
	int		sr;
	int		tr;

	tr = g_ed.screen_rows - TOOLBOX_HEIGHT - 1;
	ln = g_ed.head;
	i = -1;
	while (ln && ++i < g_ed.scroll_row)
		ln = ln->next;
	sr = -1;
	while (++sr < tr)
	{
		set_cursor_position(TOOLBOX_HEIGHT + 1 + sr, 1);
		write(STDOUT_FILENO, "\033[K", 3);
		if (ln)
		{
			prt_line(ln, i);
			ln = ln->next;
		}
		else
			write(STDOUT_FILENO, "\033[90m   ~\033[0m", 11);
		i++;
	}
}
