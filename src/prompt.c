#include "editor.h"

static void	setup_prompt(int r, const char *p)
{
	set_cursor_position(r, 1);
	write(STDOUT_FILENO, "\033[K\033[7m", 7);
	write(STDOUT_FILENO, p, strlen(p));
	write(STDOUT_FILENO, "\033[0m ", 5);
}

static void	handle_prompt_backspace(char *buf, int *len)
{
	if (*len > 0)
	{
		*len = prev_utf8_char_start(buf, *len);
		buf[*len] = '\0';
	}
}

static void	handle_prompt_char(char *buf, int *len, int buf_size, int key)
{
	int		cl;
	int		j;
	char	cb;

	cl = utf8_char_len((unsigned char)key);
	if (cl == 1 && *len + 1 < buf_size)
	{
		buf[(*len)++] = (char)key;
		buf[*len] = '\0';
	}
	else if (cl > 1 && *len + cl < buf_size)
	{
		buf[(*len)++] = (char)key;
		j = 0;
		while (++j < cl)
		{
			if (read(STDIN_FILENO, &cb, 1) == 1)
				buf[(*len)++] = cb;
		}
		buf[*len] = '\0';
	}
}

int	prompt_input(const char *prompt, char *buf, int buf_size)
{
	int	len;
	int	key;

	setup_prompt(g_ed.screen_rows, prompt);
	len = 0;
	buf[0] = '\0';
	while (1)
	{
		set_cursor_position(g_ed.screen_rows, strlen(prompt) + 2);
		write(STDOUT_FILENO, "\033[K", 3);
		write(STDOUT_FILENO, buf, len);
		key = read_key();
		if (key == KEY_ENTER)
			return (1);
		if (key == KEY_ESC)
			return (0);
		if (key == KEY_BACKSPACE || key == 127)
			handle_prompt_backspace(buf, &len);
		else if (key >= 32)
			handle_prompt_char(buf, &len, buf_size, key);
	}
}
