#!/bin/bash
# To watch log output, use: tail -f /tmp/debug_camera_demo3.log
stdbuf -oL ./camera_demo3 >> /tmp/debug_camera_demo3.log 2>&1
