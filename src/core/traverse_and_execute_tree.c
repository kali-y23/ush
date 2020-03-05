#include "ush.h"

static void shell_and_operator(t_tree *tree, t_ush *ush, int *status) {
    if (*status > 0 || *status < 0)
        return;
    mx_traverse_and_execute_tree(tree->right, ush, status);
}

static void shell_or_operator(t_tree *tree, t_ush *ush, int *status) {
    if (!*status)
        return;
    mx_traverse_and_execute_tree(tree->right, ush, status);
}

void mx_traverse_and_execute_tree(t_tree *tree, t_ush *ush, int *status) {
    char **args = NULL;
    t_job *job = NULL;

    if (!tree)
        return;
    mx_traverse_and_execute_tree(tree->left, ush, status);
    args = mx_split_cmd(tree->data);
    if (args && *args) {
        if (!mx_strcmp(args[0], "&&"))
            shell_and_operator(tree, ush, status);
        else if (!mx_strcmp(args[0], "||"))
            shell_or_operator(tree, ush, status);
        else {
            job = mx_create_job(tree->data);
            *status = mx_launch_job(job, ush, ush->env);
            mx_delete_job(&job);
            mx_traverse_and_execute_tree(tree->right, ush, status);
        }
        mx_del_strarr(&args);
    }
}
