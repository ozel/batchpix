#!/bin/bash
while true; do
        nc -u -q 0  ozelipad 8123 < frame_
        echo "new frame:" | tee /dev/stderr | nc -u -q 0 ozelipad 8123
done
