#include "editor.h"

t_line	*create_line(void)
{
	t_line	*l;

	l = malloc(sizeof(t_line));
	if (!l)
		return (NULL);
	l->cap = INITIAL_LINE_CAP;
	l->text = malloc(l->cap);
	if (!l->text)
	{
		free(l);
		return (NULL);
	}
	l->text[0] = '\0';
	l->len = 0;
	l->prev = NULL;
	l->next = NULL;
	return (l);
}

void	free_line(t_line *line)
{
	if (line)
	{
		free(line->text);
		free(line);
	}
}

t_line	*get_line_at(int index)
{
	t_line	*l;
	int		i;

	l = g_ed.head;
	i = 0;
	while (i < index && l)
	{
		l = l->next;
		i++;
	}
	return (l);
}

void	insert_line_after(t_line *after, t_line *new_line)
{
	if (!after)
	{
		new_line->next = g_ed.head;
		if (g_ed.head)
			g_ed.head->prev = new_line;
		g_ed.head = new_line;
	}
	else
	{
		new_line->next = after->next;
		new_line->prev = after;
		if (after->next)
			after->next->prev = new_line;
		after->next = new_line;
	}
	g_ed.num_lines++;
}

void	remove_line(t_line *line)
{
	if (line->prev)
		line->prev->next = line->next;
	else
		g_ed.head = line->next;
	if (line->next)
		line->next->prev = line->prev;
	g_ed.num_lines--;
}
