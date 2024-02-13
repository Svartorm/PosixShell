echo "$YELLOW==== Redirections ====$WHITE"

run_test redir_out_echo
run_test redir_out_cat
run_test redir_out_ls

run_test redir_in_grep

run_test redir_app_out_echo
run_test redir_app_out_cat
run_test redir_app_out_ls

run_test redir_out_stderr_ls
run_test redir_out_fd45_cat

run_test redir_rw_man
run_test redir_rw_grep

run_test redir_multiple
run_test redir_function
run_test redir_if

echo "$YELLOW==============$WHITE"
