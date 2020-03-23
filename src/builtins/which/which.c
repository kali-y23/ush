#include "ush.h"

static char *get_full_filename(char *dirpath, char *filename) {
    char *tmp = NULL;
    char *full_filename = NULL;

    if (dirpath[mx_strlen(dirpath) - 1] == '/')
        full_filename =  mx_strjoin(dirpath, filename);
    else {
        tmp = mx_strjoin(dirpath, "/");
        full_filename = mx_strjoin(tmp, filename);
        mx_strdel(&tmp);
    }
    return full_filename;
}

static char **check_match(DIR *dir, char **flags, char *path, char *command) {
    char *full_filename = NULL;
    char **res = NULL;
    struct dirent *dirnt;
    struct stat st;

    while ((dirnt = readdir(dir)) != NULL) {
        if (!mx_strcmp(dirnt->d_name, command)) {
            full_filename = get_full_filename(path, command);
            lstat(full_filename, &st);

            if (MX_IS_EXEC(st.st_mode)) {
                res = mx_push_to_strarr(res, full_filename);
                if (!mx_check_flag(flags, 'a')) {
                    mx_strdel(&full_filename);
                    break;
                }
            }
            mx_strdel(&full_filename);
        }
    }
    return res;
}

static int scan_dir(char ***output, char **flags, char **path, char *arg) {
    int flag = 0;
    char **tmp = NULL;
    char **res = NULL;

    for (int i = 0; path[i]; ++i) {
        DIR *dir = opendir(path[i]);

        if (!dir)
            continue;

        if ((tmp = check_match(dir, flags, path[i], arg)) && flag == 0) {
            res = mx_strarr_join(*output, tmp);
            mx_del_strarr(&tmp);
            *output = res;
            output = &res;
            flag = 1;
        }
        closedir(dir);
    }

    return flag;
}

static char **check_builtins(t_ush *ush, char *arg) {
    char **output = NULL;
    int len = mx_strlen(arg) + mx_strlen(": shell built-in command") + 1;
    char *str = NULL;

    for (int i = 0; ush->builtins[i]; ++i) {
        if (!mx_strcmp(arg, ush->builtins[i])) {
            str = malloc(sizeof(char) * len + 1);
            mx_strcpy(str, arg);
            mx_strcpy(str + mx_strlen(arg), ": shell built-in command");
            output = mx_push_to_strarr(output, str);
            mx_strdel(&str);
            return output;
        }
    }
    return NULL;
}

char **mx_which(t_ush *ush, char **flags, char **args, int *status) {
    char *env_path = NULL;
    char **path = NULL;
    char **res = NULL;

    env_path = getenv("PATH");
    if (!env_path)
        env_path = mx_getenv(ush->local_variables, "PATH");
    else
        env_path = mx_strdup(env_path);
    path = mx_strsplit(env_path, ':');

    for (int j = 0; args[j]; ++j) {
        if ((res = check_builtins(ush, args[j])) && !mx_check_flag(flags, 'a'))
            continue;
        if (scan_dir(&res, flags, path, args[j]) == 0)
            *status = 1;
    }

    mx_del_strarr(&path);
    mx_strdel(&env_path);

    return res;
}
