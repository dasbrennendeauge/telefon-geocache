#!/bin/bash

curl -H "Content-type: application/x-www-form-urlencoded" \
    -H "Content-Length: 0" \
    -H "Ocp-Apim-Subscription-Key: $1" \
    -XPOST https://francecentral.api.cognitive.microsoft.com/sts/v1.0/issuetoken > bearer

while IFS=';' read a b; do \
    sh generate.sh "$b" ${a}; \
done < ../texte

rm bearer