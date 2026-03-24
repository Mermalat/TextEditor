#include "editor.h"

int	utf8_char_len(unsigned char c)
{
	if (c < 0x80)
		return (1);
	if ((c & 0xE0) == 0xC0)
		return (2);
	if ((c & 0xF0) == 0xE0)
		return (3);
	if ((c & 0xF8) == 0xF0)
		return (4);
	return (1);
}

int	prev_utf8_char_start(const char *text, int pos)
{
	if (pos <= 0)
		return (0);
	pos--;
	while (pos > 0 && ((unsigned char)text[pos] & 0xC0) == 0x80)
		pos--;
	return (pos);
}

int	next_utf8_char_end(const char *text, int len, int pos)
{
	int	cl;

	if (pos >= len)
		return (len);
	cl = utf8_char_len((unsigned char)text[pos]);
	if (pos + cl > len)
		return (len);
	return (pos + cl);
}

int	byte_to_display_col(const char *text, int byte_pos)
{
	int	col;
	int	i;
	int	cl;

	col = 0;
	i = 0;
	while (i < byte_pos)
	{
		cl = utf8_char_len((unsigned char)text[i]);
		i += cl;
		col++;
	}
	return (col);
}

int	display_to_byte_col(const char *text, int len, int dcol)
{
	int	col;
	int	i;
	int	cl;

	col = 0;
	i = 0;
	while (i < len && col < dcol)
	{
		cl = utf8_char_len((unsigned char)text[i]);
		i += cl;
		col++;
	}
	return (i);
}
