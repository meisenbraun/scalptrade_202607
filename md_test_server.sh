#!/bin/bash
# $1 is port
# $2 is md file

while true; do
    while true; do
        cat $2
        echo "waiting..."
        sleep 1
    done | nc -l -v -p "$1"
done
