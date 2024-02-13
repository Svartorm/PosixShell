echo "$YELLOW==== Main ====$WHITE"

# removed: we are trying to give '-c "echo Input as string"' in stdin
# run_test string
run_test file

run_test_with_stdin stdin_from_file2
run_test_with_stdin stdin_from_string2

echo "$YELLOW==============$WHITE\n"
