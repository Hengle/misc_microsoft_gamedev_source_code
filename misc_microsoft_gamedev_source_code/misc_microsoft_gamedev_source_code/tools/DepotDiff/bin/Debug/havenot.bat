@echo off
dir /s/b/a-d %1 | p4 -x- have >c:\\have.txt 2>c:\\havenot.txt