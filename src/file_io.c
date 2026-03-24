#include "editor.h"

static void	free_all_lines(void)
{
	t_line	*l;
	t_line	*nx;

	l = g_ed.head;
	while (l)
	{
		nx = l->next;
		free_line(l);
		l = nx;
	}
	g_ed.head = NULL;
	g_ed.num_lines = 0;
	free_undo_stack();
	clear_search();
	clear_selection();
}

static void	read_file_lines(FILE *f)
{
	char	buf[4096];
	t_line	*p;
	t_line	*nl;
	int		b;

	p = NULL;
	while (fgets(buf, sizeof(buf), f))
	{
		nl = create_line();
		b = strlen(buf);
		if (b > 0 && buf[b - 1] == '\n')
			buf[--b] = '\0';
		if (b > 0 && buf[b - 1] == '\r')
			buf[--b] = '\0';
		line_insert_text(nl, 0, buf, b);
		insert_line_after(p, nl);
		p = nl;
	}
}

void	open_file(void)
{
	char	path[MAX_PATH_LEN];
	FILE	*f;

	file_browser(path, 0);
	if (path[0] == '\0')
		return ;
	f = fopen(path, "r");
	if (!f)
		return ;
	free_all_lines();
	read_file_lines(f);
	fclose(f);
	if (!g_ed.head)
	{
		g_ed.head = create_line();
		g_ed.num_lines = 1;
	}
	strcpy(g_ed.filename, path);
	g_ed.cursor_row = 0; g_ed.cursor_col = 0;
	g_ed.scroll_row = 0; g_ed.modified = 0;
}

static void	write_file_to_disk(const char *path)
{
	FILE	*f;
	t_line	*l;

	f = fopen(path, "w");
	if (!f)
		return ;
	l = g_ed.head;
	while (l)
	{
		fwrite(l->text, 1, l->len, f);
		fputc('\n', f);
		l = l->next;
	}
	fclose(f);
	strcpy(g_ed.filename, path);
	g_ed.modified = 0;
}

void	save_file(void)
{
	if (g_ed.filename[0])
		write_file_to_disk(g_ed.filename);
	else
		save_as_file();
}

void	save_as_file(void)
{
	char	path[MAX_PATH_LEN];

	file_browser(path, 1);
	if (path[0] == '\0')
		return ;
	write_file_to_disk(path);
}

void	handle_file_ops(int key)
{
	if (key == CTRL_O)
		open_file();
	else if (key == CTRL_S)
		save_file();
	else if (key == CTRL_SHIFT_S)
		save_as_file();
}
