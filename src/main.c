#include "editor.h"
#include <signal.h>

t_editor	g_ed;

void	die(const char *s)
{
	write(STDOUT_FILENO, "\033[2J\033[H", 7);
	perror(s);
	exit(1);
}

static void	handle_sigwinch(int sig)
{
	(void)sig;
	get_terminal_size(&g_ed.screen_rows, &g_ed.screen_cols);
}

void	init_editor(void)
{
	memset(&g_ed, 0, sizeof(t_editor));
	g_ed.head = create_line();
	g_ed.num_lines = 1;
	g_ed.running = 1;
	g_ed.current_match = -1;
	get_terminal_size(&g_ed.screen_rows, &g_ed.screen_cols);
}

void	free_editor(void)
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
	free_undo_stack();
	free(g_ed.clipboard);
	free(g_ed.matches);
}

int	main(void)
{
	setlocale(LC_ALL, "");
	init_editor();
	enable_raw_mode();
	signal(SIGWINCH, handle_sigwinch);
	while (g_ed.running)
	{
		render_screen();
		process_keypress();
	}
	disable_raw_mode();
	write(STDOUT_FILENO, "\033[2J\033[H", 7);
	free_editor();
	return (0);
}
