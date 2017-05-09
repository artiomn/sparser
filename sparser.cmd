@echo off
sparser.exe %*|iconv\iconv.exe -c -s -f UTF-8 -t CP866

