#include "editor.h"

static void	load_dir_entries(DIR *d, char (*ent)[256], int *is_dir, int *cnt, char *cwd)
{
	struct dirent	*e;
	struct stat		st;
	char			fpath[MAX_PATH_LEN];

	*cnt = 0;
	strcpy(ent[*cnt], "..");
	is_dir[*cnt] = 1;
	(*cnt)++;
	while ((e = readdir(d)) != NULL && *cnt < 512)
	{
		if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
			continue ;
		strcpy(ent[*cnt], e->d_name);
		snprintf(fpath, sizeof(fpath), "%s/%s", cwd, e->d_name);
		is_dir[*cnt] = (stat(fpath, &st) == 0 && S_ISDIR(st.st_mode));
		(*cnt)++;
	}
}

static int	handle_browser_key(int key, int *sel, int cnt)
{
	if (key == KEY_ARROW_UP && *sel > 0)
		(*sel)--;
	else if (key == KEY_ARROW_DOWN && *sel < cnt - 1)
		(*sel)++;
	else if (key == KEY_ENTER || key == KEY_ESC || key == 'n')
		return (key);
	return (0);
}

static int	handle_browser_enter(char (*ent)[256], int *is_dir, int sel, char *cwd, char *rp)
{
	char	np[MAX_PATH_LEN];
	char	*sl;

	if (is_dir[sel])
	{
		if (strcmp(ent[sel], "..") == 0)
		{
			sl = strrchr(cwd, '/');
			if (sl && sl != cwd) *sl = '\0';
			else strcpy(cwd, "/");
		}
		else
		{
			snprintf(np, sizeof(np), "%s/%s", cwd, ent[sel]);
			strcpy(cwd, np);
		}
		return (1);
	}
	snprintf(rp, MAX_PATH_LEN, "%s/%s", cwd, ent[sel]);
	return (2);
}

static int	browse_loop_action(int k, char (*ent)[256], int *dir, int *sel, char *cwd, char *rp, int sm)
{
	char	fn[256];

	if (k == KEY_ESC)
	{
		rp[0] = '\0';
		return (1);
	}
	if (k == KEY_ENTER)
	{
		if (handle_browser_enter(ent, dir, *sel, cwd, rp) == 2)
			return (1);
		*sel = 0;
	}
	if (sm && k == 'n')
	{
		if (prompt_input("Filename:", fn, 256) && fn[0])
		{
			snprintf(rp, MAX_PATH_LEN, "%s/%s", cwd, fn);
			return (1);
		}
	}
	return (0);
}

void	file_browser(char *result_path, int save_mode)
{
	char	cwd[MAX_PATH_LEN];
	int		sel;
	DIR		*d;
	char	ent[512][256];
	int		is_dir[512];
	int		cnt;

	if (!getcwd(cwd, sizeof(cwd))) strcpy(cwd, ".");
	sel = 0;
	while (1)
	{
		d = opendir(cwd);
		if (!d) { result_path[0] = '\0'; return ; }
		load_dir_entries(d, ent, is_dir, &cnt, cwd);
		closedir(d);
		if (sel >= cnt) sel = cnt - 1;
		if (sel < 0) sel = 0;
		render_browser_screen(cwd, ent, is_dir, cnt, sel, save_mode);
		if (browse_loop_action(handle_browser_key(read_key(), &sel, cnt), ent, is_dir, &sel, cwd, result_path, save_mode))
			return ;
	}
}
