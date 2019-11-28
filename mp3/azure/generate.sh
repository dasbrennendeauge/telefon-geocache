#!/bin/bash

cat <<EOF > data.xml
<speak version="1.0" xml:lang="de-DE">
  <voice xml:lang="de-DE" xml:gender="Female" name="de-DE-Hedda">
    ${1}
  </voice>
</speak>
EOF

curl -H "Authorization: Bearer $(cat bearer)" \
  -H 'X-Microsoft-OutputFormat: audio-16khz-32kbitrate-mono-mp3' \
  -H 'Content-Type: application/ssml+xml' \
  -d @data.xml \
  -XPOST https://francecentral.tts.speech.microsoft.com/cognitiveservices/v1 \
   > $2.mp3

rm data.xml
