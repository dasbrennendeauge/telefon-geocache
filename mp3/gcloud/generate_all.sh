#!/bin/bash
while IFS=';' read a b; do \
    sh generate.sh "$b" ${a}; \
done < ../texte