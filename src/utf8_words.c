#include "editor.h"

int	word_start_before(const char *text, int pos)
{
	int	pp;

	if (pos <= 0)
		return (0);
	pos = prev_utf8_char_start(text, pos);
	while (pos > 0 && text[pos] == ' ')
		pos = prev_utf8_char_start(text, pos);
	while (pos > 0)
	{
		pp = prev_utf8_char_start(text, pos);
		if (text[pp] == ' ')
			break ;
		pos = pp;
	}
	return (pos);
}

int	word_end_after(const char *text, int len, int pos)
{
	if (pos >= len)
		return (len);
	while (pos < len && text[pos] == ' ')
		pos = next_utf8_char_end(text, len, pos);
	while (pos < len && text[pos] != ' ')
		pos = next_utf8_char_end(text, len, pos);
	return (pos);
}
