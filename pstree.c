/* pstree.c - print process list in tree
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdlib.h>
#include "defs.h"		/* From the crash source top-level directory */

void pstree_init(void);		/* constructor function */
void pstree_fini(void);		/* destructor function (optional) */

void cmd_pstree(void);		/* Declare the commands and their help data. */
char *help_pstree[];

static struct command_table_entry command_table[] = {
	{"pstree", cmd_pstree, help_pstree, 0},	/* One or more commands, */
	{NULL},			/* terminated by NULL, */
};

char *line_type[5] = { "    ", "-+- ", " |- ", " `- ", " |  " };

enum LINE_TYPE {
	LINE_SPACE,
	LINE_FIRST,
	LINE_BRANCH,
	LINE_LAST,
	LINE_VERT
};

#define MAX_BRANCH  1024
/* each index contains the width from the previous to the current */
int branch_locations[MAX_BRANCH];
int curr_depth;
enum LINE_TYPE branch_bar[MAX_BRANCH];

#define MAX_PID 1024
struct task_context *pid_list[MAX_PID];
int pid_cnt = 0;

int print_pid = 0;
int print_group = 0;
int print_status = 0;

static void print_pid_tree(ulong task);
static void print_branch(int first);
static void print_task(ulong task, ulong * tgid_list, ulong * tgid_count);
static void child_list(ulong task);

void __attribute__ ((constructor))
    pstree_init(void)
{				/* Register the command set. */
	register_extension(command_table);
}

/* 
 *  This function is called if the shared object is unloaded. 
 *  If desired, perform any cleanups here. 
 */
void __attribute__ ((destructor))
    pstree_fini(void)
{
}

/* 
 *  Arguments are passed to the command functions in the global args[argcnt]
 *  array.  See getopt(3) for info on dash arguments.  Check out defs.h and
 *  other crash commands for usage of the myriad of utility routines available
 *  to accomplish what your task.
 */
void cmd_pstree(void)
{
	struct task_context *tc;
	int c;
	int i;
	int arg_cnt;
	pid_t tmp_pid;

	print_pid = print_group = 0;

	while ((c = getopt(argcnt, args, "pgs")) != EOF) {
		switch (c) {
		case 'p':
			print_pid = 1;
			break;
		case 'g':
			print_group = 1;
			break;
		case 's':
			print_status = 1;
			break;
		default:
			argerrs++;
			break;
		}
	}

	if (argerrs)
		cmd_usage(pc->curcmd, SYNOPSIS);

	tc = FIRST_CONTEXT();
	pid_cnt = 0;
	pid_list[pid_cnt] = tc;
	arg_cnt = 0;
	while (args[optind] && pid_cnt < MAX_PID - 1) {
		arg_cnt++;
		tmp_pid = atoi(args[optind++]);
		tc = pid_to_context(tmp_pid);
		if (tc == NULL) {
			fprintf(fp, "PID %ul does not exist\n", tmp_pid]);
			continue;
		}
		pid_list[pid_cnt++] = tc;
	}
	if (pid_cnt == 0 && arg_cnt == 0)
		pid_cnt++;

	fprintf(fp, "Total # of processes in the system : %ul\n", RUNNING_TASKS());
	for (i = 0; i < pid_cnt; i++) {
		tc = pid_list[i];
		print_pid_tree(tc->task);
		fprintf(fp, "\n\n");
	}
}

static void print_pid_tree(ulong task)
{
	curr_depth = 0;
	print_branch(1);
	print_task(task, NULL, NULL);
	child_list(task);
}

static void print_branch(int first)
{
	int i, j;

	if (first && curr_depth > 0) {
		fprintf(fp, "%s", line_type[LINE_FIRST]);
		return;
	}

	for (i = 0; i < curr_depth; i++) {
		for (j = 0; j < branch_locations[i]; j++)
			fprintf(fp, " ");
		fprintf(fp, "%s", line_type[branch_bar[i]]);
	}
}

static void print_task(ulong task, ulong * tgid_list, ulong * tgid_count)
{
	struct task_context *tc;
	char command[TASK_COMM_LEN + 20];	// Command + PID
	char pid_str[20];
	ulong tgid;
	ulong tcnt = 0;
	char tgid_str[20];
	char task_state_str[8], task_state[4];

	strcpy(tgid_str, "");
	tc = task_to_context(task);
	if (print_group && tgid_list != NULL && tgid_count != NULL) {
		tgid = task_tgid(task);
		tcnt = tgid_count[find_tgid(tgid_list, tgid, RUNNING_TASKS())];
		if (tcnt > 1) {
			sprintf(tgid_str, "<%ul>", tcnt);
		}
	}

	if (print_pid)
		sprintf(pid_str, " [%ul]", print_group ? tgid : tc->pid);

	if (print_status) {
		task_state_string(task, task_state, 0);
		sprintf(task_state_str, "[%s]", task_state);
	} else {
		strcpy(task_state_str, "");
	}

	sprintf(command, "%s%s%s%s ", tgid_str, tc->comm, pid_str,
		task_state_str);
	branch_locations[curr_depth] = strlen(command);
	fprintf(fp, command);
}

int find_tgid(ulong * tgid_list, ulong tgid, int max_cnt)
{
	int i;
	for (i = 0; i < max_cnt; i++) {
		if ((tgid_list[i] == tgid) || (tgid_list[i] == 0))
			return i;
	}
	return i;
}

static void child_list(ulong task)
{
	int i, first = 1;
	struct task_context *tc;
	ulong *task_list;
	ulong *tgid_list;	// thread group id
	ulong *tgid_count;	// thread group count
	int task_count = 0;
	int tgid, tgid_idx;
	int running_tasks;

	running_tasks = RUNNING_TASKS();

	task_list = (ulong *) malloc(sizeof(ulong) * running_tasks);
	if (task_list == NULL)
		return;
	tgid_list = (ulong *) calloc(running_tasks, sizeof(ulong));
	if (tgid_list == NULL) {
		free(task_list);
		return;
	}
	tgid_count = (ulong *) calloc(running_tasks, sizeof(ulong));
	if (tgid_count == NULL) {
		free(task_list);
		free(tgid_list);
		return;
	}

	tc = FIRST_CONTEXT();
	for (i = 0; i < RUNNING_TASKS(); i++, tc++) {
		if (tc->ptask == task && tc->task != task) {
			if (print_group) {	// compress thread
				tgid = task_tgid(tc->task);
				tgid_idx =
				    find_tgid(tgid_list, tgid, running_tasks);
				if (tgid_idx == running_tasks) {
					break;	// something went wrong.
				}
				tgid_list[tgid_idx] = tgid;
				tgid_count[tgid_idx]++;
				if (tgid_count[tgid_idx] == 1)
					task_list[task_count++] = tc->task;
			} else {	// print all threads
				task_list[task_count++] = tc->task;
			}
		}
	}

	curr_depth++;
	branch_bar[curr_depth - 1] = '|';
	for (i = 0; i < task_count; i++) {
		if (i == (task_count - 1))
			branch_bar[curr_depth - 1] = LINE_LAST;
		else
			branch_bar[curr_depth - 1] = LINE_BRANCH;

		print_branch(first);
		print_task(task_list[i], tgid_list, tgid_count);

		if (i == (task_count - 1))
			branch_bar[curr_depth - 1] = LINE_SPACE;
		else
			branch_bar[curr_depth - 1] = LINE_VERT;

		child_list(task_list[i]);
		if (i != (task_count - 1))
			fprintf(fp, "\n");
		first = 0;
	}
	curr_depth--;

	free(task_list);
	free(tgid_list);
	free(tgid_count);
}

/* 
 *  The optional help data is simply an array of strings in a defined format.
 *  For example, the "help pstree" command will use the help_pstree[] string
 *  array below to create a help page that looks like this:
 * 
 *    NAME
 *      pstree - print process list in tree
 *
 *    SYNOPSIS
 *      pstree [-p][-g][-s][pid] ...
 *
 *    DESCRIPTION
 *      This command prints process list in tree
 *
 *      The list can be modified by the following options
 *
 *      -p  print process ID",
 *      -g  print thread group instead of each threads",
 *      -s  print task status",
 *
 *    EXAMPLE
 *      Print out process list
 *
 *        crash> pstree
 *        init --+-- swapd
 *               +-- httpd
 *               +-...
 *
 */

char *help_pstree[] = {
	"pstree",		/* command name */
	"print process list in tree",	/* short description */
	"[-p][-g][-s] [pid] ...",	/* argument synopsis, or " " if none */

	"  This command prints process list in tree",
	"",
	"  The list can be modified by the following options",
	"",
	"    -p  print process ID",
	"    -g  print thread group instead of each threads",
	"    -s  print task status",
	"\nEXAMPLE",
	"  Print out process list\n",
	"    crash> pstree",
	"    init --+-- swapd",
	"           +-- httpd",
	"           +-...",
	NULL
};
