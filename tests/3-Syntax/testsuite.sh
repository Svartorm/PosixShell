echo "$YELLOW==== Syntax ====$WHITE"

run_test if_command
run_test if_command2

run_test compound_list
run_test compound_list2

run_test single_quote
run_test single_quote_mixed
run_test single_quote_tricky

run_test comment
run_test comment_EOL
run_test comment_tricky

echo "|----- double quote -----"

run_test dq/simple_dq
run_test dq/simple_dq_2
run_test dq/escaping_dq
run_test dq/normal_dq
run_test dq/quote_dq
run_test dq/dq_normal
run_test dq/dq_quote
run_test dq/dq_var
run_test dq/dq_var_bracket
run_test dq/var_dq
run_test dq/var_dq_normal
run_test dq/dq_norm_var

echo "$YELLOW================$WHITE\n"
