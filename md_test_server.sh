#!/bin/bash
# $1 is port
# $2 is md file

while true; do
    while true; do
        cat "$2"
        echo "waiting..." >&2
        sleep 1
    done | nc -l -v -p "$1"
done
