#include "editor.h"

static int	parse_modifier_seq(void)
{
	char	mod;
	char	dir;

	if (read(STDIN_FILENO, &mod, 1) != 1) return (KEY_ESC);
	if (read(STDIN_FILENO, &dir, 1) != 1) return (KEY_ESC);
	if (mod == '5')
	{
		if (dir == 'A') return (CTRL_ARROW_UP);
		if (dir == 'B') return (CTRL_ARROW_DOWN);
		if (dir == 'C') return (CTRL_ARROW_RIGHT);
		if (dir == 'D') return (CTRL_ARROW_LEFT);
	}
	else if (mod == '6')
	{
		if (dir == 'A') return (CTRL_SHIFT_UP);
		if (dir == 'B') return (CTRL_SHIFT_DOWN);
		if (dir == 'C') return (CTRL_SHIFT_RIGHT);
		if (dir == 'D') return (CTRL_SHIFT_LEFT);
	}
	return (KEY_ESC);
}

static int	parse_tilde_or_mod(char seq1)
{
	char	s2;

	if (read(STDIN_FILENO, &s2, 1) != 1) return (KEY_ESC);
	if (seq1 == '1' && s2 == ';')
		return (parse_modifier_seq());
	if (seq1 == '1' && s2 == '~')
		return (KEY_HOME);
	if (s2 == '~')
	{
		if (seq1 == '3') return (KEY_DELETE);
		if (seq1 == '4' || seq1 == '8') return (KEY_END);
		if (seq1 == '5') return (KEY_PAGE_UP);
		if (seq1 == '6') return (KEY_PAGE_DOWN);
		if (seq1 == '7') return (KEY_HOME);
	}
	return (KEY_ESC);
}

static int	parse_escape(void)
{
	char	seq[2];

	if (read(STDIN_FILENO, &seq[0], 1) != 1) return (KEY_ESC);
	if (read(STDIN_FILENO, &seq[1], 1) != 1) return (KEY_ESC);
	if (seq[0] == '[')
	{
		if ((seq[1] >= '0' && seq[1] <= '9') || seq[1] == '1')
			return (parse_tilde_or_mod(seq[1]));
		if (seq[1] == 'A') return (KEY_ARROW_UP);
		if (seq[1] == 'B') return (KEY_ARROW_DOWN);
		if (seq[1] == 'C') return (KEY_ARROW_RIGHT);
		if (seq[1] == 'D') return (KEY_ARROW_LEFT);
		if (seq[1] == 'H') return (KEY_HOME);
		if (seq[1] == 'F') return (KEY_END);
	}
	else if (seq[0] == 'O')
	{
		if (seq[1] == 'H') return (KEY_HOME);
		if (seq[1] == 'F') return (KEY_END);
	}
	return (KEY_ESC);
}

int	read_key(void)
{
	int		nread;
	char	c;

	while (1)
	{
		nread = read(STDIN_FILENO, &c, 1);
		if (nread == 1)
			break ;
		if (nread == -1 && errno != EAGAIN)
			die("read");
	}
	if (c == '\x1b')
		return (parse_escape());
	if (c == 8)
		return (CTRL_H);
	return ((int)(unsigned char)c);
}
