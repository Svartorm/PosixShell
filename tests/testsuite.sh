#!/bin/sh

RED="\e[31m"
GREEN="\e[32m"
YELLOW="\e[33m"
BLUE="\e[34m"
TURQUOISE="\e[36m"
WHITE="\e[0m"

TOTAL_RUN=0
TOTAL_FAIL=0

ref_file_out=/tmp/42sh_testsuite_ref_out
ref_file_err=/tmp/42sh_testsuite_ref_err
my_file_out=/tmp/42sh_testsuite_my_out
my_file_err=/tmp/42sh_testsuite_my_err

run_test() {
    [ -e "$1" ] || echo "Missing test file $1" 1>&2
    sucess=true
    TOTAL_RUN=$((TOTAL_RUN+1))

    printf "|-->>%b %s %b-->> " "$BLUE" "$1" "$WHITE"
    bash --posix "$1" > $ref_file_out 2> $ref_file_err
    REF_CODE=$?

    ./../42sh "$1" > $my_file_out 2> $my_file_err
    MY_CODE=$?

    diff --color=always -u $ref_file_out $my_file_out > $1.diff
    DIFF_CODE=$?
    if [ $REF_CODE != $MY_CODE ]; then
        echo -n "[$RED RETURN - {$MY_CODE} $WHITE]"
        sucess=false
    fi
    if [ $DIFF_CODE != 0 ]; then
        echo -n "[$RED STDOUT $WHITE]"
        sucess=false
    fi
    if { [ -s $ref_file_err ] && [ ! -s $my_file_err ]; } ||
        { [ ! -s $ref_file_err ] && [ -s $my_file_err ]; }; then
            echo -n "[$RED STDERR $WHITE]"
            sucess=false
    fi

    if $sucess; then
        echo "[$GREEN OK $WHITE]"
        rm -f $1.diff
    else
        [ -s "$(realpath $1.diff)" ] && echo -n "$RED (cat $(realpath $1.diff))$WHITE"
        echo
        TOTAL_FAIL=$((TOTAL_FAIL + 1))
        echo "STDERR:"
        cat $my_file_err
    fi
}

run_test_with_stdin() {
    [ -e "$1" ] || echo "Missing test file $1" 1>&2
    sucess=true
    TOTAL_RUN=$((TOTAL_RUN+1))

    printf "|-->>%b %s %b-->> " "$BLUE" "$1" "$WHITE"
    ("$1" | bash --posix) > $ref_file_out 2> $ref_file_err
    REF_CODE=$?

    ("$1" | ./../42sh) > $my_file_out 2> $my_file_err
    MY_CODE=$?

    diff --color=always -u $ref_file_out $my_file_out > $1.diff
    DIFF_CODE=$?
    if [ $REF_CODE != $MY_CODE ]; then
        echo -n "[$RED RETURN - {$MY_CODE}$WHITE]"
        sucess=false
    fi
    if [ $DIFF_CODE != 0 ]; then
        echo -n "[$RED STDOUT $WHITE]"
        sucess=false
    fi
    if { [ -s $ref_file_err ] && [ ! -s $my_file_err ]; } ||
        { [ ! -s $ref_file_err ] && [ -s $my_file_err ]; }; then
            echo -n "[$RED STDERR $WHITE]"
            sucess=false
    fi

    if $sucess; then
        echo "[$GREEN OK $WHITE]"
        rm -f $1.diff
    else
        [ -s "$(realpath $1.diff)" ] && echo -n "$RED (cat $(realpath $1.diff))$WHITE"
        echo
        TOTAL_FAIL=$((TOTAL_FAIL + 1))
    fi
}

run_category() {
    cd $1
    source ./testsuite.sh
    cd - >/dev/null
}

find_directory() {
    if [ -d "$1" ]; then
        cd "$1" || exit
        if [ -f ./testsuite.sh ]; then
            . ./testsuite.sh
        else
            echo "testsuite.sh not found in $1"
        fi
        cd - > /dev/null || exit
    else
        echo "$1 is not a directory"
    fi
}

run_testsuite() {
    for directory in */ ; do
        if [ -d "$directory" ]; then
            find_directory "$directory"
        fi
    done
}

run_testsuite $(find . -type d)

if [ "$TOTAL_RUN" -eq 0 ]; then
    echo "No tests have been run."
else
    PERCENT_SUCESS=$(((TOTAL_RUN - TOTAL_FAIL) * 100 / TOTAL_RUN))

    echo  "$BLUE======================================"
    echo  "$WHITE RECAP: $([ $PERCENT_SUCESS = 100 ] && echo $GREEN || echo $RED) $PERCENT_SUCESS% - $((TOTAL_RUN - TOTAL_FAIL))/$TOTAL_RUN $WHITE"
    echo  "$BLUE=======================================$WHITE"
fi
