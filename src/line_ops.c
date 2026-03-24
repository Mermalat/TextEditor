#include "editor.h"

void	ensure_capacity(t_line *line, int needed)
{
	if (line->len + needed + 1 > line->cap)
	{
		while (line->cap < line->len + needed + 1)
			line->cap *= 2;
		line->text = realloc(line->text, line->cap);
	}
}

void	line_insert_char(t_line *line, int pos, const char *ch, int ch_len)
{
	ensure_capacity(line, ch_len);
	memmove(line->text + pos + ch_len, line->text + pos, line->len - pos);
	memcpy(line->text + pos, ch, ch_len);
	line->len += ch_len;
	line->text[line->len] = '\0';
}

void	line_delete_char(t_line *line, int pos, int ch_len)
{
	if (pos + ch_len > line->len)
		return ;
	memmove(line->text + pos, line->text + pos + ch_len,
		line->len - pos - ch_len);
	line->len -= ch_len;
	line->text[line->len] = '\0';
}

void	line_insert_text(t_line *line, int pos, const char *txt, int txt_len)
{
	ensure_capacity(line, txt_len);
	memmove(line->text + pos + txt_len, line->text + pos, line->len - pos);
	memcpy(line->text + pos, txt, txt_len);
	line->len += txt_len;
	line->text[line->len] = '\0';
}
