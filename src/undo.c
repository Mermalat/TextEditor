#include "editor.h"

static char	*dup_data(const char *data, int len)
{
	char	*res;

	if (!data || len <= 0)
		return (NULL);
	res = malloc(len + 1);
	if (!res)
		return (NULL);
	memcpy(res, data, len);
	res[len] = '\0';
	return (res);
}

void	push_undo(t_undo_type t, t_pos p, t_str s, t_pos p_ex)
{
	t_undo_action	*a;

	a = malloc(sizeof(t_undo_action));
	if (!a)
		return ;
	a->type = t;
	a->row = p.r;
	a->col = p.c;
	a->data_len = s.len;
	a->data = dup_data(s.s, s.len);
	a->extra_row = p_ex.r;
	a->extra_col = p_ex.c;
	a->extra_data_len = 0;
	a->extra_data = NULL;
	a->next = g_ed.undo_stack;
	g_ed.undo_stack = a;
}

void	free_undo_stack(void)
{
	t_undo_action	*a;

	while (g_ed.undo_stack)
	{
		a = g_ed.undo_stack;
		g_ed.undo_stack = a->next;
		free(a->data);
		free(a->extra_data);
		free(a);
	}
}

static void	execute_simple_undo(t_undo_action *a)
{
	t_line	*line;

	if (a->type == UNDO_INSERT_CHAR)
	{
		line = get_line_at(a->row);
		if (line)
			line_delete_char(line, a->col, a->data_len);
		g_ed.cursor_row = a->row;
		g_ed.cursor_col = a->col;
	}
	else if (a->type == UNDO_DELETE_CHAR)
	{
		line = get_line_at(a->row);
		if (line && a->data)
			line_insert_text(line, a->col, a->data, a->data_len);
		g_ed.cursor_row = a->row;
		g_ed.cursor_col = a->col + a->data_len;
	}
}

void	undo(void)
{
	t_undo_action	*a;

	a = g_ed.undo_stack;
	if (!a)
		return ;
	g_ed.undo_stack = a->next;
	if (a->type == UNDO_INSERT_CHAR || a->type == UNDO_DELETE_CHAR)
		execute_simple_undo(a);
	else
		execute_complex_undo(a);
	g_ed.modified = 1;
	free(a->data);
	free(a->extra_data);
	free(a);
}
