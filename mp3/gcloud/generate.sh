#!/bin/bash

cat <<EOF > data.json
{
  "audioConfig": {
    "audioEncoding": "MP3",
    "effectsProfileId": [
      "small-bluetooth-speaker-class-device"
    ],
    "pitch": 0,
    "speakingRate": 1
  },
  "input": {
    "text": "${1}"
  },
  "voice": {
    "languageCode": "de-DE",
    "name": "de-DE-Wavenet-B",
    "ssmlGender": "FEMALE"
  }
}
EOF

curl -H "Authorization: Bearer $(gcloud auth application-default print-access-token)" \
  -H "Content-Type: application/json; charset=utf-8" \
  --data @data.json "https://texttospeech.googleapis.com/v1/text:synthesize" > synthesize-text.txt

cat synthesize-text.txt | grep 'audioContent' | \
sed 's|audioContent| |' | tr -d '\n ":{},' > tmp.txt && \
base64 tmp.txt --decode > $2.mp3 && \
rm tmp.txt synthesize-text.txt data.json
