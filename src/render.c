#include "editor.h"

void	set_cursor_position(int row, int col)
{
	char	buf[32];
	int		n;

	n = snprintf(buf, sizeof(buf), "\033[%d;%dH", row, col);
	write(STDOUT_FILENO, buf, n);
}

static void	draw_tb_itm(const char *i, int *c, int *r)
{
	int	dlen;

	dlen = byte_to_display_col(i, strlen(i));
	if (*c + dlen + 3 > g_ed.screen_cols && *c > 1)
	{
		print_padding(g_ed.screen_cols - *c + 1);
		(*r)++;
		set_cursor_position(*r, 1);
		write(STDOUT_FILENO, "\033[K\033[1;37;48;5;208m", 19);
		*c = 1;
	}
	write(STDOUT_FILENO, " ", 1);
	write(STDOUT_FILENO, i, strlen(i));
	write(STDOUT_FILENO, " |", 2);
	*c += dlen + 3;
}

void	render_toolbox(void)
{
	const char	*itr[11] = {"Opn(^O)", "Sv(^S)", "SvAs(^Sf+S)", "Fnd(^F)", "Go(^G)", "Rpl(^H)", "Cp(^C)", "Ct(^X)", "Pst(^V)", "Ud(^Z)", "Ext(ESC)"};
	int			c;
	int			r;
	int			i;

	set_cursor_position(1, 1);
	write(STDOUT_FILENO, "\033[K\033[1;37;48;5;208m", 19);
	c = 1; r = 1; i = -1;
	while (++i < 11)
		draw_tb_itm(itr[i], &c, &r);
	print_padding(g_ed.screen_cols - c + 1);
	write(STDOUT_FILENO, "\033[0m", 4);
	set_cursor_position(r + 1, 1);
	write(STDOUT_FILENO, "\033[K\033[38;5;208m", 15);
	i = -1;
	while (++i < g_ed.screen_cols)
		write(STDOUT_FILENO, "-", 1);
	write(STDOUT_FILENO, "\033[0m", 4);
}

void	render_screen(void)
{
	int	t_r;

	get_terminal_size(&g_ed.screen_rows, &g_ed.screen_cols);
	t_r = g_ed.screen_rows - TOOLBOX_HEIGHT - 1;
	if (g_ed.cursor_row < g_ed.scroll_row)
		g_ed.scroll_row = g_ed.cursor_row;
	if (g_ed.cursor_row >= g_ed.scroll_row + t_r)
		g_ed.scroll_row = g_ed.cursor_row - t_r + 1;
	write(STDOUT_FILENO, "\033[?25l", 6);
	render_toolbox();
	render_lines();
	render_status_bar();
	write(STDOUT_FILENO, "\033[?25h", 6);
}
