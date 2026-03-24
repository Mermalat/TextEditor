#include "editor.h"

void	search_text(void)
{
	char	query[MAX_SEARCH_LEN];

	if (!prompt_input("Find:", query, MAX_SEARCH_LEN))
	{
		clear_search();
		return ;
	}
	strcpy(g_ed.search_query, query);
	find_matches(query);
	g_ed.search_active = 1;
	g_ed.current_match = -1;
	if (g_ed.match_count > 0)
		g_ed.current_match = 0;
}

static void	handle_nav_key(int key, int *run)
{
	if (key == KEY_ARROW_RIGHT || key == KEY_ARROW_DOWN)
		g_ed.current_match = (g_ed.current_match + 1) % g_ed.match_count;
	else if (key == KEY_ARROW_LEFT || key == KEY_ARROW_UP)
		g_ed.current_match = (g_ed.current_match - 1 + g_ed.match_count)
			% g_ed.match_count;
	else if (key == KEY_ESC || key == KEY_ENTER)
		*run = 0;
}

void	navigate_matches(void)
{
	char	query[MAX_SEARCH_LEN];
	int		run;

	if (!prompt_input("Go to:", query, MAX_SEARCH_LEN))
	{
		clear_search();
		return ;
	}
	strcpy(g_ed.search_query, query);
	find_matches(query);
	g_ed.search_active = 1;
	if (g_ed.match_count == 0)
	{
		g_ed.current_match = -1;
		return ;
	}
	g_ed.current_match = 0;
	run = 1;
	while (run)
	{
		g_ed.cursor_row = g_ed.matches[g_ed.current_match].row;
		g_ed.cursor_col = g_ed.matches[g_ed.current_match].col;
		render_screen();
		handle_nav_key(read_key(), &run);
	}
}

static void	do_replace_in_line(t_line *l, char *old_txt, char *nw_txt)
{
	char	*p;
	int		old_len;
	int		new_len;
	int		off;
	int		pos;

	old_len = strlen(old_txt);
	new_len = strlen(nw_txt);
	off = 0;
	p = strstr(l->text + off, old_txt);
	while (p != NULL)
	{
		pos = (int)(p - l->text);
		memmove(l->text + pos, l->text + pos + old_len,
			l->len - pos - old_len);
		l->len -= old_len;
		ensure_capacity(l, new_len);
		memmove(l->text + pos + new_len, l->text + pos, l->len - pos);
		memcpy(l->text + pos, nw_txt, new_len);
		l->len += new_len;
		l->text[l->len] = '\0';
		off = pos + new_len;
		p = strstr(l->text + off, old_txt);
	}
}

void	replace_all(void)
{
	char	old_txt[MAX_SEARCH_LEN];
	char	nw_txt[MAX_SEARCH_LEN];
	t_line	*l;

	if (!prompt_input("Find:", old_txt, MAX_SEARCH_LEN))
		return ;
	if (!prompt_input("Replace with:", nw_txt, MAX_SEARCH_LEN))
		return ;
	l = g_ed.head;
	while (l)
	{
		do_replace_in_line(l, old_txt, nw_txt);
		l = l->next;
	}
	g_ed.modified = 1;
	clear_search();
}
