#include "editor.h"

static void	handle_mov_arrow(int key, t_line *cur)
{
	if (key == KEY_ARROW_LEFT)
	{
		if (g_ed.cursor_col > 0)
			g_ed.cursor_col = prev_utf8_char_start(cur->text, g_ed.cursor_col);
		else if (g_ed.cursor_row > 0)
		{
			g_ed.cursor_row--; cur = get_line_at(g_ed.cursor_row); g_ed.cursor_col = cur->len;
		}
	}
	else if (key == KEY_ARROW_RIGHT)
	{
		if (g_ed.cursor_col < cur->len)
			g_ed.cursor_col = next_utf8_char_end(cur->text, cur->len, g_ed.cursor_col);
		else if (g_ed.cursor_row < g_ed.num_lines - 1)
		{
			g_ed.cursor_row++; g_ed.cursor_col = 0;
		}
	}
	else if (key == KEY_ARROW_UP && g_ed.cursor_row > 0)
	{
		g_ed.cursor_row--; cur = get_line_at(g_ed.cursor_row);
		if (g_ed.cursor_col > cur->len) g_ed.cursor_col = cur->len;
	}
	else if (key == KEY_ARROW_DOWN && g_ed.cursor_row < g_ed.num_lines - 1)
	{
		g_ed.cursor_row++; cur = get_line_at(g_ed.cursor_row);
		if (g_ed.cursor_col > cur->len) g_ed.cursor_col = cur->len;
	}
}

static void	handle_mov_page(int key, t_line *cur)
{
	int	t_r;

	if (key == KEY_PAGE_UP)
	{
		t_r = g_ed.screen_rows - TOOLBOX_HEIGHT - 1; g_ed.cursor_row -= t_r;
		if (g_ed.cursor_row < 0) g_ed.cursor_row = 0;
		cur = get_line_at(g_ed.cursor_row);
		if (g_ed.cursor_col > cur->len) g_ed.cursor_col = cur->len;
	}
	else if (key == KEY_PAGE_DOWN)
	{
		t_r = g_ed.screen_rows - TOOLBOX_HEIGHT - 1; g_ed.cursor_row += t_r;
		if (g_ed.cursor_row >= g_ed.num_lines) g_ed.cursor_row = g_ed.num_lines - 1;
		cur = get_line_at(g_ed.cursor_row);
		if (g_ed.cursor_col > cur->len) g_ed.cursor_col = cur->len;
	}
	else if (key == KEY_HOME)
		g_ed.cursor_col = 0;
	else if (key == KEY_END)
		g_ed.cursor_col = cur->len;
}

void	handle_movement(int key)
{
	t_line	*cur;

	cur = get_line_at(g_ed.cursor_row);
	clear_selection();
	handle_mov_arrow(key, cur);
	handle_mov_page(key, cur);
}

static void	handle_sel_arrow(int key, t_line *cur)
{
	if (key == CTRL_ARROW_LEFT)
	{
		if (g_ed.cursor_col > 0) g_ed.cursor_col = prev_utf8_char_start(cur->text, g_ed.cursor_col);
		else if (g_ed.cursor_row > 0)
		{
			g_ed.cursor_row--; cur = get_line_at(g_ed.cursor_row); g_ed.cursor_col = cur->len;
		}
	}
	else if (key == CTRL_ARROW_RIGHT)
	{
		if (g_ed.cursor_col < cur->len) g_ed.cursor_col = next_utf8_char_end(cur->text, cur->len, g_ed.cursor_col);
		else if (g_ed.cursor_row < g_ed.num_lines - 1) { g_ed.cursor_row++; g_ed.cursor_col = 0; }
	}
	else if (key == CTRL_ARROW_UP && g_ed.cursor_row > 0)
	{
		g_ed.cursor_row--; cur = get_line_at(g_ed.cursor_row);
		if (g_ed.cursor_col > cur->len) g_ed.cursor_col = cur->len;
	}
	else if (key == CTRL_ARROW_DOWN && g_ed.cursor_row < g_ed.num_lines - 1)
	{
		g_ed.cursor_row++; cur = get_line_at(g_ed.cursor_row);
		if (g_ed.cursor_col > cur->len) g_ed.cursor_col = cur->len;
	}
}

void	handle_selection_movement(int key)
{
	t_line	*cur;

	cur = get_line_at(g_ed.cursor_row);
	start_selection();
	handle_sel_arrow(key, cur);
	if (key == CTRL_SHIFT_LEFT)
		g_ed.cursor_col = word_start_before(cur->text, g_ed.cursor_col);
	else if (key == CTRL_SHIFT_RIGHT)
		g_ed.cursor_col = word_end_after(cur->text, cur->len, g_ed.cursor_col);
	else if (key == CTRL_SHIFT_UP && g_ed.cursor_row > 0)
	{
		g_ed.cursor_row--; g_ed.cursor_col = 0;
	}
	else if (key == CTRL_SHIFT_DOWN && g_ed.cursor_row < g_ed.num_lines - 1)
	{
		g_ed.cursor_row++; cur = get_line_at(g_ed.cursor_row); g_ed.cursor_col = cur->len;
	}
	update_selection();
}
