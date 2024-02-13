# To be implemented
# Description: Testsuite for the errors

echo "$YELLOW==== Errors ====$WHITE"

run_test syntax_test
run_test unset_test
run_test unset_fun
run_test cd_test
run_test do_not_exist
run_test for_err

echo "$YELLOW==============$WHITE\n"
