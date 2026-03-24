#include "editor.h"

static void	print_browser_header(char *cwd)
{
	char	h[MAX_PATH_LEN + 80];
	int		l;

	write(STDOUT_FILENO, "\033[2J\033[H", 7);
	l = snprintf(h, sizeof(h), "\033[1;36m === File Browser ===\033[0m\r\n\033[33m %s\033[0m\r\n\r\n", cwd);
	write(STDOUT_FILENO, h, l);
}

static void	print_browser_entry(char *ent, int is_dir, int is_sel)
{
	char	b[512];
	int		l;

	if (is_sel)
	{
		if (is_dir)
			l = snprintf(b, sizeof(b), " \033[7;34m > [%s/]\033[0m\r\n", ent);
		else
			l = snprintf(b, sizeof(b), " \033[7m > %s\033[0m\r\n", ent);
	}
	else
	{
		if (is_dir)
			l = snprintf(b, sizeof(b), "   \033[34m[%s/]\033[0m\r\n", ent);
		else
			l = snprintf(b, sizeof(b), "   %s\r\n", ent);
	}
	write(STDOUT_FILENO, b, l);
}

void	render_browser_screen(char *cwd, char ent[512][256], int *is_dir, int cnt, int sel, int sm)
{
	int			tr;
	int			scroll;
	int			i;
	const char	*instr;

	print_browser_header(cwd);
	tr = g_ed.screen_rows - 6;
	if (sel >= tr) scroll = sel - tr + 1;
	else scroll = 0;
	i = scroll;
	while (i < cnt && i < scroll + tr)
	{
		print_browser_entry(ent[i], is_dir[i], i == sel);
		i++;
	}
	instr = "\r\n\033[90m Up/Down: Navigate | Enter: Select | ESC: Cancel";
	write(STDOUT_FILENO, instr, strlen(instr));
	if (sm)
		write(STDOUT_FILENO, " | n: New filename\033[0m", 22);
	write(STDOUT_FILENO, "\033[0m\r\n", 6);
}
