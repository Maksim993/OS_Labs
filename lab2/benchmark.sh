#!/bin/bash

echo "Performance benchmark for multithreaded string search"
echo "===================================================="

gcc -o lab2_naive_search lab2_naive_search.c -lpthread

TEXT="abacabaabacababacabaabacababacabaabacababacabaabacaba"
PATTERN="aba"

echo "Text: $TEXT"
echo "Pattern: $PATTERN"
echo ""

for threads in 1 2 4 8
do
    echo "Threads: $threads"
    ./lab2_naive_search $threads "$TEXT" "$PATTERN" | grep -E "(Time:|Threads used:)"
    echo "----------------------------------------"
done