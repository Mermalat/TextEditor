#include "editor.h"

void	disable_raw_mode(void)
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_ed.orig_termios);
	write(STDOUT_FILENO, "\033[?25h", 6);
	write(STDOUT_FILENO, "\033[0 q", 5);
}

void	enable_raw_mode(void)
{
	struct termios	raw;

	if (tcgetattr(STDIN_FILENO, &g_ed.orig_termios) == -1)
		die("tcgetattr");
	raw = g_ed.orig_termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
		die("tcsetattr");
	write(STDOUT_FILENO, "\033[5 q", 5);
}

void	get_terminal_size(int *rows, int *cols)
{
	struct winsize	ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
	{
		*rows = 24;
		*cols = 80;
	}
	else
	{
		*rows = ws.ws_row;
		*cols = ws.ws_col;
	}
}
