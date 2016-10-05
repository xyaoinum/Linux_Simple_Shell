#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

const int max_num_args = 10;

struct node {
	char *data;
	struct node *next;
};


struct list {
	struct node *first;
	struct node *last;
	size_t length;
};


void list_add(struct list *v, char *c)
{

	if (c == NULL)
		return;

	if (c[0] != '/') {
		printf("error: invalid path name\n");
		return;
	}

	struct node *n1 = v->first;

	while (n1 != NULL) {
		if (strcmp(n1->data, c) == 0)
			return;

		n1 = n1->next;
	}

	char *tmp_c = malloc(sizeof(char) * (strlen(c) + 1));

	if (tmp_c == NULL) {
		printf("error: %s\n", strerror(errno));
		return;
	}

	strcpy(tmp_c, c);

	struct node *n;

	n = (struct node *) malloc(sizeof(struct node));

	if (n == NULL) {
		printf("error: %s\n", strerror(errno));
		return;
	}

	n->data = tmp_c;
	n->next = NULL;

	if (v->length == 0) {
		v->first = n;
		v->last = n;
	} else {
		v->last->next = n;
		v->last = n;
	}

	v->length++;
	return;

}

void list_remove(struct list *v, char *c)
{
	struct node *n1 = v->first;

	if (n1 == NULL)
		return;

	struct node *n2 = n1->next;

	while (n2 != NULL) {

		if (strcmp(n2->data, c) == 0) {
			if (n2 == v->last)
				v->last = n1;

			free(n2->data);
			n2 = n2->next;
			free(n1->next);
			n1->next = n2;
			v->length--;
			break;
		}

		n1 = n1->next;
		n2 = n2->next;

	}

	if (strcmp(v->first->data, c) == 0) {
		free(v->first->data);

		if (v->last == v->first) {
			free(v->first);
			v->first = NULL;
			v->last = NULL;
		} else {
			struct node *tmp_n = v->first;

			v->first = v->first->next;
			free(tmp_n);
		}

		v->length--;

	}

}

void list_show(struct list *v)
{
	struct node *n = v->first;

	while (n != NULL) {
		if (n->next == NULL)
			printf("%s", n->data);
		else
			printf("%s:", n->data);

		n = n->next;
	}

	printf("\n");
}

void list_destruct(struct list *v)
{
	struct node *n = v->first;

	while (n != NULL) {
		free(n->data);
		struct node *tmp_n = n;

		n = n->next;
		free(tmp_n);
	}

	v->first = NULL;
	v->last = NULL;
	v->length = 0;
}


struct history {
	char *data[100];
	size_t length;
	int pos;
};

void history_create(struct history *h)
{
	int i;

	for (i = 0; i < 100; i++)
		h->data[i] = NULL;

	h->length = 0;
	h->pos = 0;
}

int history_add(struct history *h, char *c)
{
	char *tmp_c = malloc(sizeof(char) * (strlen(c) + 1));

	if (tmp_c == NULL) {
		printf("error: %s\n", strerror(errno));
		return -1;
	}

	strcpy(tmp_c, c);

	if (h->data[h->pos] != NULL)
		free(h->data[h->pos]);

	h->data[h->pos] = tmp_c;
	h->pos = (h->pos + 1) % 100;

	if (h->length < 100)
		h->length++;

	return 0;
}

char *history_get(struct history *h, int pos)
{
	if (h->length < 100) {
		if (pos >= 0 && pos < h->pos)
			return h->data[pos];
		return NULL;
	}

	if (pos < 0 || pos >= 100)
		return NULL;
	return h->data[(h->pos + pos) % 100];
}

void history_show(struct history *h)
{
	int i;

	if (h->length < 100) {
		for (i = 0; i < h->length; i++)
			printf("%d %s\n", i, h->data[i]);
	} else {
		for (i = 0; i < h->length; i++)
			printf("%d %s\n", i, h->data[(h->pos + i) % 100]);
	}

}

void history_clear(struct history *h)
{
	int i;

	for (i = 0; i < h->length; i++)
		free(h->data[i]);

	for (i = 0; i < 100; i++)
		h->data[i] = NULL;

	h->pos = 0;
	h->length = 0;
}

void restore(char *c, size_t len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (c[i] == '\0')
			c[i] = ' ';
	}

}

int main(int argc, char **argv)
{

	struct list path_list = {NULL, NULL, 0};

	struct history command_history;

	history_create(&command_history);

	char *line = NULL;
	size_t block_len = 0;
	ssize_t len;
	pid_t pid = -1;
	int from_history = 0;

	while (from_history ||
			(printf("$") &&
			 getline(&line, &block_len, stdin) > 0)) {

		from_history = 0;
		len = strlen(line);

		if (line[len - 1] == '\n') {
			line[len - 1] = '\0';
			len--;
		}

		char *command;

		command = strtok(line, " ");

		if (command == NULL)
			continue;

		char *args[max_num_args + 2];

		args[0] = command;

		int i;

		for (i = 1; i < max_num_args + 2; i++)
			args[i] = NULL;

		int arg_cnt = 0;

		for (i = 1; i < max_num_args + 2; i++) {
			args[i] = strtok(NULL, " ");

			if (args[i] == NULL)
				break;

			arg_cnt++;
		}

		if (arg_cnt > max_num_args) {
			printf("error: too many arguments (maximum: %d)\n",
				   max_num_args + 1);
			continue;
		}


		if (strcmp(command, "exit") == 0) {
			if (arg_cnt != 0) {
				printf("error: too few / too many arguments\n");
				restore(line, len);
				if (history_add(&command_history, line) == -1) {
					free(line);
					list_destruct(&path_list);
					history_clear(&command_history);
					exit(1);
				}
				continue;
			}

			break;
		}

		if (strcmp(command, "pwd") == 0) {
			if (arg_cnt != 0) {
				printf("error: too few / too many arguments\n");
				restore(line, len);
				if (history_add(&command_history, line) == -1) {
					free(line);
					list_destruct(&path_list);
					history_clear(&command_history);
					exit(1);
				}
				continue;
			}

			int success = 0;
			char *pwd_buf = NULL;
			int buf_size = 256;

			for (; !success; buf_size *= 2) {
				int allo_size = sizeof(char) * buf_size;

				pwd_buf = realloc(pwd_buf, allo_size);

				if (pwd_buf == NULL) {
					printf("error: %s\n", strerror(errno));
					break;
				}

				if (getcwd(pwd_buf, buf_size) == NULL) {
					if (errno != ERANGE) {
						printf("error: %s\n",
							   strerror(errno));
						break;
					}
				} else {
					printf("%s\n", pwd_buf);
					success = 1;
				}
			}

			if (pwd_buf != NULL)
				free(pwd_buf);

			restore(line, len);
			if (history_add(&command_history, line) == -1) {
				free(line);
				list_destruct(&path_list);
				history_clear(&command_history);
				exit(1);
			}
			continue;
		}

		if (strcmp(command, "cd") == 0) {
			if (arg_cnt != 1)
				printf("error: too few / too many arguments\n");
			else if (chdir(args[1]) != 0)
				printf("error: %s\n", strerror(errno));

			restore(line, len);
			if (history_add(&command_history, line) == -1) {
				free(line);
				list_destruct(&path_list);
				history_clear(&command_history);
				exit(1);
			}
			continue;
		}

		if (strcmp(command, "path") == 0) {
			if (arg_cnt == 0) {
				list_show(&path_list);
				restore(line, len);
				if (history_add(&command_history, line) == -1) {
					free(line);
					list_destruct(&path_list);
					history_clear(&command_history);
					exit(1);
				}
				continue;
			}

			if (arg_cnt != 2) {
				printf("error: too few / too many arguments\n");
				restore(line, len);
				if (history_add(&command_history, line) == -1) {
					free(line);
					list_destruct(&path_list);
					history_clear(&command_history);
					exit(1);
				}
				continue;
			}

			if (strcmp(args[1], "+") == 0) {
				list_add(&path_list, args[2]);
				restore(line, len);
				if (history_add(&command_history, line) == -1) {
					free(line);
					list_destruct(&path_list);
					history_clear(&command_history);
					exit(1);
				}
				continue;
			}

			if (strcmp(args[1], "-") == 0) {
				list_remove(&path_list, args[2]);
				restore(line, len);
				if (history_add(&command_history, line) == -1) {
					free(line);
					list_destruct(&path_list);
					history_clear(&command_history);
					exit(1);
				}
				continue;
			}

			printf("error: incorrect command arguments\n");
			restore(line, len);
			if (history_add(&command_history, line) == -1) {
				free(line);
				list_destruct(&path_list);
				history_clear(&command_history);
				exit(1);
			}
			continue;
		}

		if (strcmp(command, "history") == 0) {
			if (arg_cnt == 0) {
				history_show(&command_history);
				continue;
			}

			if (arg_cnt != 1) {
				printf("error: too few / too many arguments\n");
				continue;
			}

			if (strcmp(args[1], "-c") == 0) {
				history_clear(&command_history);
				continue;
			}

			int pos = atoi(args[1]);
			char *c = history_get(&command_history, pos);

			if (c == NULL) {
				printf("error: incorrect history command\n");
				continue;
			}


			strcpy(line, c);
			from_history = 1;
			continue;
		}


		pid = fork();

		int status;

		if (pid < 0) {
			printf("error: %s\n", strerror(errno));
			restore(line, len);
			if (history_add(&command_history, line) == -1) {
				free(line);
				list_destruct(&path_list);
				history_clear(&command_history);
				exit(1);
			}
		} else if (pid == 0) {
			if (strstr(command, "/") == NULL) {
				struct node *n = path_list.first;

				while (n != NULL) {
					int tmp_l = strlen(n->data);

					tmp_l += strlen(command);
					tmp_l += 2;

					int allo_size = sizeof(char) * tmp_l;

					char *tmp_c = malloc(allo_size);

					strcpy(tmp_c, n->data);
					strcat(tmp_c, "/");
					strcat(tmp_c, command);

					if (access(tmp_c, F_OK) != -1) {
						args[0] = tmp_c;
						execvp(tmp_c, args);
						char *tmp_e = strerror(errno);

						printf("error: %s\n", tmp_e);
						free(line);
						list_destruct(&path_list);
						history_clear(&command_history);
						exit(1);
					}

					n = n->next;
					free(tmp_c);
				}

				printf("error: command not found\n");
				free(line);
				list_destruct(&path_list);
				history_clear(&command_history);
				exit(1);

			} else {
				execvp(command, args);
				printf("error: %s\n", strerror(errno));
				free(line);
				list_destruct(&path_list);
				history_clear(&command_history);
				exit(1);
			}
		} else {
			while (wait(&status) != -1)
				;

			if (errno != ECHILD)
				printf("error: %s\n", strerror(errno));

			restore(line, len);
			if (history_add(&command_history, line) == -1) {
				free(line);
				list_destruct(&path_list);
				history_clear(&command_history);
				exit(1);
			}
		}

	}

	free(line);
	list_destruct(&path_list);
	history_clear(&command_history);



	return 0;
}
