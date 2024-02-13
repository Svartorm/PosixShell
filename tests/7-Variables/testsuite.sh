echo "$YELLOW==== Variables ====$WHITE"

run_test basic_var_assign
run_test basic_var_print

# Add your new tests here
run_test bad_variable1
run_test bad_variable2
run_test brandone
run_test command_var_1
run_test command_var_2
run_test dollar
run_test empty
run_test multiple_defs_on_line
run_test null_var_assign
run_test pwd_test
run_test quote_var_1
run_test random
run_test reset_var
run_test sharp
run_test simple_var_bracket
run_test simple_var_concat
run_test uid

run_test assign_print_same_line

echo "$YELLOW==============$WHITE"
