echo "$YELLOW==== Loops ====$WHITE"

run_test while_false
run_test until_true
run_test for_no_var_use

run_test while_break
run_test until_break
run_test for_break
run_test for_continue

run_test miscellaneous

echo "$YELLOW==============$WHITE"
