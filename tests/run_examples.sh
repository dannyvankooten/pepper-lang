#!/bin/bash

for FILE in examples/*.monkey; do
    echo -e -n "$FILE: "
    bin/monkey $FILE > /dev/null
    case "$?" in
    0) echo -e "\x1b[32mok\033[0m" ;;
    1) echo -e "\033[91mfailed\033[0m" ;;
    esac

        
done
