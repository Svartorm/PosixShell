echo "$YELLOW==== Operators ====$WHITE"

run_test negation_simple
run_test negation_if
run_test negation_if_fork
run_test negation_double
run_test negation_pipe
run_test pipe

run_test and_basic
run_test and_chain
run_test or_basic
run_test or_chain
run_test and_or

echo "$YELLOW==============$WHITE"
