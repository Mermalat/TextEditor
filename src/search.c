#include "editor.h"

void	clear_search(void)
{
	free(g_ed.matches);
	g_ed.matches = NULL;
	g_ed.match_count = 0;
	g_ed.current_match = -1;
	g_ed.search_active = 0;
	g_ed.search_query[0] = '\0';
}

static void	add_match(int row, int col, int qlen, int *cap)
{
	if (g_ed.match_count >= *cap)
	{
		*cap *= 2;
		g_ed.matches = realloc(g_ed.matches, *cap * sizeof(t_search_match));
	}
	g_ed.matches[g_ed.match_count].row = row;
	g_ed.matches[g_ed.match_count].col = col;
	g_ed.matches[g_ed.match_count].len = qlen;
	g_ed.match_count++;
}

void	find_matches(const char *query)
{
	int		cap;
	int		qlen;
	t_line	*l;
	int		row;
	char	*p;

	clear_search();
	cap = 64;
	g_ed.matches = malloc(cap * sizeof(t_search_match));
	qlen = strlen(query);
	l = g_ed.head;
	row = 0;
	while (l)
	{
		p = l->text;
		while ((p = strstr(p, query)) != NULL)
		{
			add_match(row, (int)(p - l->text), qlen, &cap);
			p += qlen;
		}
		l = l->next;
		row++;
	}
}

int	is_search_match(int row, int byte_col)
{
	int	i;

	if (!g_ed.search_active)
		return (0);
	i = -1;
	while (++i < g_ed.match_count)
	{
		if (g_ed.matches[i].row == row && byte_col >= g_ed.matches[i].col
			&& byte_col < g_ed.matches[i].col + g_ed.matches[i].len)
		{
			if (i == g_ed.current_match)
				return (2);
			return (1);
		}
	}
	return (0);
}
