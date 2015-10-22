/***
 *  dmz - v0.1
 *  Copyright (C) 2015, niemal
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <sysexits.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>


long int this_dmz_id = -1;
const char *dmz_dir = "/tmp/dmz";
int note_time = 0;

typedef struct {
	long int id;
	pid_t pgid;
	char *log_path;
	char *cmd;
} dmz_info;


void
move(long int dmz_id, char *src, char *dst)
{
        int is_dst_file = 0;
        DIR *dir = opendir(dst);

        if (dir == NULL && errno == ENOENT)
                is_dst_file = 1;

        char *dst_target = calloc(512, sizeof(char));
        strcat(dst_target, dst);

        if (!is_dst_file) {
                char *str_dmz_id = calloc(sizeof(dmz_id), sizeof(char));
                snprintf(str_dmz_id, sizeof(str_dmz_id)-1, "%ld", dmz_id);

                if (dst[sizeof(dst)-1] != '/')
                        strcat(dst_target, "/");

                strcat(dst_target, str_dmz_id);
        }

        int in_fd = open(src, O_RDONLY);
        if (in_fd < 0) {
                printf("Incorrect src @ move().\n");
                exit(EXIT_FAILURE);
        }

        int out_fd = open(dst_target, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (out_fd < 0) {
                printf("Incorrect dst_target @ move().\n");
                exit(EXIT_FAILURE);
        }

        char buffer[8192];
        while (1) {
                ssize_t res = read(in_fd, &buffer[0], sizeof(buffer));

                if (!res) break;

                if (res < 0) {
                        printf("Incorrect res @ move().\n");
                        exit(EXIT_FAILURE);
                }

                if (write(out_fd, &buffer[0], res) != res) {
                        printf("Error at write @ move().\n");
                        exit(EXIT_FAILURE);
                }
        }

        if (remove(src) == -1) {
                printf("Unable to remove() @ move().");
                exit(EXIT_FAILURE);
        }
}


char *
get_dmz_log_path(long int dmz_id, char *str_dmz_id)
{
	if (dmz_id != -1) {
		str_dmz_id = calloc(sizeof(dmz_id), sizeof(char));
		snprintf(str_dmz_id, sizeof(str_dmz_id)-1, "%ld", dmz_id);
	}

	char *dmz_log_path = calloc((sizeof(dmz_dir) + 1 + sizeof(str_dmz_id)),
		sizeof(char));

	strcat(dmz_log_path, dmz_dir);
	strcat(dmz_log_path, "/");
	strcat(dmz_log_path, str_dmz_id);

	return dmz_log_path;
}


void
get_this_dmz_id()
{
	mkdir(dmz_dir, 0755);

	if (errno == EEXIST) {
		DIR *dir = opendir(dmz_dir);
		struct dirent *ent = readdir(dir);
		long int parsed_id;

		while (ent != NULL) {
                        if (ent->d_name[0] == '.' || ent->d_type != DT_REG) {
                                ent = readdir(dir);
                                continue;
                        }

			parsed_id = strtol(ent->d_name, NULL, 10);

			if (errno == ERANGE) {
				printf("The dmz_id that was attempted to get was too high or too low.\n");
				exit(EXIT_FAILURE);
			}

			if (this_dmz_id < parsed_id)
				this_dmz_id = parsed_id;

			ent = readdir(dir);
		}
	}

	this_dmz_id++;
}


int
is_valid_log(FILE *log_file, char *log_path)
{
        if (log_file == NULL) {
                printf("The log_file with path [%s] failed to open.\n", log_path);
                exit(EXIT_FAILURE);
        }

        char ch;
        int counter = 0;
        while (ch != EOF) {
                ch = fgetc(log_file);
                if (ch == '\n')
                        counter++;
        }

        if (counter < 2)
                return 0;

        rewind(log_file);

        char line[4096];

        if (fgets(line, 4095, log_file) == NULL) {
                printf("Error when reading from file [%s].\n", log_path);
                exit(EXIT_FAILURE);
        }

        if (strstr(line, "CMD: ") == NULL)
                return 0;
        
        if (fgets(line, 4095, log_file) == NULL) {
                printf("Error when reading from file [%s].\n", log_path);
                exit(EXIT_FAILURE);
        }

        if (strstr(line, "PGID: ") == NULL)
                return 0;

        rewind(log_file);

        return 1;
}


void
get_all_dmz_info(dmz_info *dmz_spawns, unsigned int *size)
{
	mkdir(dmz_dir, 0755);

	if (errno == EEXIST) {
		DIR *dir = opendir(dmz_dir);
		struct dirent *ent = readdir(dir);
                char *log_path;
                FILE *log_file;

		while ((ent = readdir(dir)) != NULL) {
                        log_path = get_dmz_log_path(-1, ent->d_name);
                        log_file = fopen(log_path, "r");

			if (ent->d_type != DT_REG || !is_valid_log(log_file, log_path))
				continue;

                        (*size)++;
                        char line[4096];

                        if (fgets(line, 4095, log_file) == NULL) {
                                printf("Error when reading from file [%s].\n", log_path);
                                exit(EXIT_FAILURE);
                        }

                        line[strcspn(line, "\r\n")] = 0;

			if (*size > sizeof(dmz_spawns)) {
				dmz_info *tmp = realloc(dmz_spawns, sizeof(dmz_info) * (*size));
				if (tmp == NULL) {
					printf("Error in realloc() inside get_all_dmz_info().\n");
					exit(EXIT_FAILURE);
				}
				dmz_spawns = tmp;
			}

                        dmz_spawns[*size-1].log_path = log_path;
			dmz_spawns[*size-1].id = strtol(ent->d_name, NULL, 10);

			if (errno == ERANGE) {
				printf("The dmz_spawns[%d].id was too high or too low.\n", *size);
				exit(EXIT_FAILURE);
			}

			dmz_spawns[*size-1].cmd = calloc(sizeof(line), sizeof(char));
			strcat(dmz_spawns[*size-1].cmd, line);


			if (fgets(line, 4095, log_file) == NULL) {
				printf("Error when reading from file [%s].\n", log_path);
				exit(EXIT_FAILURE);
			}

			char *str_pgid = line + 6;

			if (str_pgid == NULL) {
				printf("str_pgid is NULL.\n");
				exit(EXIT_FAILURE);
			} else if (strlen(str_pgid) > 5) {
				printf("The parsed PID length is above 5, perhaps [%s] was tampered.\n",
                                        dmz_spawns[*size-1].log_path);

				exit(EXIT_FAILURE);
			}

			str_pgid[strcspn(str_pgid, "\r\n")] = 0;
                        dmz_spawns[*size-1].pgid = (pid_t) strtol(str_pgid, NULL, 10);

			fclose(log_file);
		}
	}
}


void
dmz_log(FILE *proc, pid_t pgid, char *cmd)
{
	char *dmz_log_path = get_dmz_log_path(this_dmz_id, NULL);
	FILE *dmz_log_file = fopen(dmz_log_path, "a+");

	if (access(dmz_log_path, F_OK) != -1) {
		fprintf(dmz_log_file, "CMD: %s\nPGID: %d\n---------\n", cmd, pgid);
	} else {
		printf("%s is inaccessible.\n", dmz_log_path);
		exit(EXIT_FAILURE);
	}

	char fget_buff[1001];
	time_t curr_time;
	struct tm *loc_time;

	while (fgets(fget_buff, 1001, proc)) {
		char *out_buff = calloc(1024, sizeof(char));

		if (note_time) {
			curr_time = time(NULL);
			loc_time = localtime(&curr_time);
			char *str_time = calloc(23, sizeof(char));

			strftime(str_time, 22, "[%d/%m/%y - %T] ", loc_time);
			strcat(out_buff, str_time);
		}

		strcat(out_buff, fget_buff);
		fprintf(dmz_log_file, "%s", out_buff);

                free(out_buff);
	}

        fclose(dmz_log_file);

        dmz_log_file = fopen(dmz_log_path, "r+");
        fflush(dmz_log_file);
        fputs("TERMINATED\n", dmz_log_file);

	fclose(dmz_log_file);
	pclose(proc);
}


void
init_dmz(char *cmd)
{
	printf("CMD: %s\n", cmd);
	get_this_dmz_id();
	printf("ID: %ld\n", this_dmz_id);

        pid_t pid = fork();
        if (pid < 0) _exit(EXIT_FAILURE);
        if (pid > 0) _exit(EXIT_SUCCESS);

        if (setsid() < 0)
                _exit(EXIT_FAILURE);

        /* we fork twice so that SID != PID */
        pid = fork();
        if (pid < 0) _exit(EXIT_FAILURE);
        if (pid > 0) _exit(EXIT_SUCCESS);

        umask(S_IWGRP | S_IWOTH);

	FILE *dmz_proc = popen(cmd, "r");

	pid = getpid();
	pid_t pgid = getpgid(pid);

	dmz_log(dmz_proc, pgid, cmd);
}


void
usage(char *prog_name)
{
	printf("Usage: %s [option(s)] [argument] ...\n"
	"-e [cmd]     \t- Daemonizes a shell command and redirects its output to a file.\n"
	"             \t\te.g. %s -e \"/usr/bin/dmesg --follow\"\n"
	"-t           \t- Optional which goes along with -e. Appends a timestamp on each line on daemon's output.\n"
	"-l           \t- Lists all running daemons initiated by dmz.\n"
	"-d [dmz_id]  \t- Destroys a daemon by sending a SIGKILL signal to its process and deleting its log file.\n"
        "             \t  Use the -l option to check dmz ids and -s to save its log file.\n"
        "-s [dir_path]\t- Optional. Used along with -d to keep a log file, moves the file (with filename its dmz id)"
        " to that directory. Currently doesn't support moving between different filesystems.\n"
        "-h           \t- Displays this usage message.\n",
	prog_name, prog_name);

	exit(EX_USAGE);
}


int
main(int argc, char **argv)
{
	if (argc == 1)
		usage(argv[0]);

	int flag;
	unsigned int i;
	char *cmd = NULL;
        char *save_lf = NULL;
	long int kill_id = -1;
	unsigned int size = 0;
	dmz_info *dmz_spawns = malloc(sizeof(dmz_info));

	while ((flag = getopt(argc, argv, "lhe:d:ts:")) != -1) {
		switch (flag) {
		case 'l':
			get_all_dmz_info(dmz_spawns, &size);

			if (dmz_spawns[0].cmd == NULL) {
				printf("No daemons are running.\n");
				exit(EXIT_SUCCESS);
			}

			for (i = 0; i < size; i++)
				printf("---------\n"
				       "ID: %ld\n"
				       "PGID: %d\n"
				       "%s\n"
				       "Log path: %s\n"
				       "---------\n",
					dmz_spawns[i].id,
					dmz_spawns[i].pgid,
					dmz_spawns[i].cmd,
					dmz_spawns[i].log_path);
			
			printf("Total daemons running: %d\n", size);
			exit(EXIT_SUCCESS);
		case 'd':
			kill_id = strtol(optarg, NULL, 10);
                        break;
                case 'e':
                        cmd = optarg;
                        break;
                case 's':
                        save_lf = optarg;
                        break;
		case 't':
			note_time = 1;
			break;
                case 'h':
                        usage(argv[0]);
		default:
			usage(argv[0]);
		}
	}


        if (kill_id != -1) {
                get_all_dmz_info(dmz_spawns, &size);
                for (i = 0; i < size; i++)
                        if (dmz_spawns[i].id == kill_id)
                                break;

                if (i == size) {
                        printf("Requested ID was not found to kill.\n");
                        exit(EXIT_FAILURE);
                }

                if (kill(-dmz_spawns[i].pgid, SIGKILL) == -1) {
                        perror("kill");
                        exit(EXIT_FAILURE);
                }

                if (save_lf) {
                        move(dmz_spawns[i].id, dmz_spawns[i].log_path, save_lf);
                        printf("The log has been moved to [%s].\n", save_lf);
                } else {
                        if (remove(dmz_spawns[i].log_path) == -1) {
                                perror("remove()");
                                exit(EXIT_FAILURE);
                        }

                        printf("Log deleted.\n");
                }

                printf("Daemon session has been detroyed successfully.\n");
                exit(EXIT_SUCCESS);
        }

	if (cmd) {
		init_dmz(cmd);
		exit(EXIT_SUCCESS);
	}
}