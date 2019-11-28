#!/bin/bash
while IFS=';' read a b; do \
    aws polly synthesize-speech --output-format mp3 --voice-id Marlene --text "$b" ${a}.mp3; \
done < ../texte