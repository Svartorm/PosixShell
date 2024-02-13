echo "$YELLOW==== Builtins ====$WHITE"

run_test ls
run_test ls_with_arg
run_test cat
run_test echo
run_test echo_string
run_test echo_mult_args
run_test echo_reserved_word
run_test echo_lf
run_test echo_e
run_test echo_E
run_test true
run_test false
run_test export_builtin
run_test export_builtin_2
run_test export_builtin_3
run_test dot_builtin
run_test dot_builtin_2
run_test dot_builtin_error

run_test exit_nocode
run_test exit_code0
run_test exit_code42
run_test exit_code255

echo "$YELLOW==============$WHITE"
