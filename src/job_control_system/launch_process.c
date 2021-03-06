#include "ush.h"

static int launch_process(t_process *process, t_ush *ush) {
    int (**builtin_func)(char **, t_ush *) = (
        int (**)(char **, t_ush *))mx_init_builtins();
    int status = MX_SUCCESS;

    for (int i = 0; i < MX_BUILTINS_COUNT; ++i)
        if (!mx_strcmp(process->argv[0], ush->builtins[i])) {
            status = builtin_func[i](process->argv, ush);
            mx_reset_env_and_clean_data(builtin_func);
            _exit(status);
        }
    mx_get_command_path(ush, process);
    if ((status = execve(process->argv[0], process->argv, ush->env)) < 0) {
        mx_command_not_found_error(process->argv[0]);
        _exit(status);
    }
    mx_reset_env_and_clean_data(builtin_func);
    _exit(status);
}

static void manage_fds(t_process *process, int *fd, t_ush *ush) {
    int out = -1;

    if (process->next) {
        if (fd[0] != STDIN_FILENO) {
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
        }
        if (fd[1] != STDOUT_FILENO) {
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
        }
    }
    else {
        if (ush->cmd_subst) {
            out = open(ush->cmd_substs_file, O_WRONLY|O_CREAT|O_APPEND, 0600);
            dup2(out, STDOUT_FILENO);
        }
        if (fd[0] != STDIN_FILENO)
            dup2(fd[0], STDIN_FILENO);
    }
}

static int redirect_and_launch(t_ush *ush, char **argv) {
    int (**builtin_func)(char **, t_ush *) = (
        int (**)(char **, t_ush *))mx_init_builtins();
    int status = MX_SUCCESS;
    int out = -1;

    if (ush->cmd_subst) {
        out = open(ush->cmd_substs_file, O_WRONLY|O_CREAT|O_APPEND, 0600);
        dup2(out, STDOUT_FILENO);
    }
    for (int i = 0; i < MX_BUILTINS_COUNT; ++i)
        if (!mx_strcmp(argv[0], ush->builtins[i])) {
            status = builtin_func[i](argv, ush);
            mx_reset_env_and_clean_data(builtin_func);
        }
    return status;
}

int mx_launch_simple_builtin(t_ush *ush, char **argv, int copy_stdout) {
    int status = redirect_and_launch(ush, argv);

    dup2(copy_stdout, STDOUT_FILENO);
    return status;
}

int mx_launch_proccess(pid_t pgid, t_process *process, int *fd, t_ush *ush) {
    pid_t pid = getpid();

    if (!pgid)
        pgid = pid;
    setpgid(pid, pgid);
    tcsetpgrp(STDIN_FILENO, pgid);
    if (ush->cmd_subst)
        mx_ignore_signals();
    else
        mx_default_signals();
    manage_fds(process, fd, ush);
    return launch_process(process, ush);
}
