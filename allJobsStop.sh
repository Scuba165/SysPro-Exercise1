#!/bin/bash
server="jobExecutorServer.txt"
if [ $# -ne 0 ]; then
    echo "Wrong input! Use no arguments."
    exit 1
fi

if [ ! -f "$server" ]; then 
    echo "Server isn't active. Script terminating."
    exit 1
fi

queuedJobs=$(./jobCommander poll queued)
runningJobs=$(./jobCommander poll running)


while IFS= read -r line; do
    jobID=$(echo "$line" | grep -oP 'job_[0-9]*')
    if [ -n "$jobID" ]; then
        ./jobCommander stop "$jobID"
    fi
done <<< "$queuedJobs"

while IFS= read -r line; do
    jobID=$(echo "$line" | grep -oP 'job_[0-9]*')
    if [ -n "$jobID" ]; then
        ./jobCommander stop "$jobID"
    fi
done <<< "$runningJobs"

