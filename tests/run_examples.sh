#!/bin/bash

for FILE in examples/*.monkey; do
    printf "%-36s" "$FILE"

    # Extract expected output from first line: 
    # Output: <expected output here>
    EXPECTED_OUTPUT=$(grep -m1 -oP "(?<=\/\/ Output: ).*" $FILE)
    # Hack to get newlines to work
    EXPECTED_OUTPUT=$(echo -e "$EXPECTED_OUTPUT")

    # Run command and keep track of elapsed time
    START_TIME="$(date -u +%s.%N)"
    ACTUAL_OUTPUT=$(bin/monkey $FILE 2>&1)
    END_TIME="$(date -u +%s.%N)"
    ELAPSED="$(bc <<<"scale=2; $END_TIME-$START_TIME")"

    # Check exit status and expected output (if set)
    if [[ $? && ( $EXPECTED_OUTPUT == "" || "$EXPECTED_OUTPUT" == "$ACTUAL_OUTPUT" ) ]]; then
        printf "\x1b[32m%-12s\033[0m (%.4f s)\n" "ok" "$ELAPSED";
    else
        echo -e -n "\033[91mfailed: \033[0m" ;
        echo "expected \"$EXPECTED_OUTPUT\", got \"$ACTUAL_OUTPUT\""
    fi  

        
done
