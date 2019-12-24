#!/bin/bash
ps -aux| grep SC2_x64 | grep -v grep |awk '{print $2}' | xargs kill