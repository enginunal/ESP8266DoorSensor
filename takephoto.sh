!/bin/bash

DATE=$(date +"%Y-%m-%d_%H%M%S")

sudo fswebcam --delay 2  --input 0 --device v4l2:/dev/video0 -r 800x600  --jpeg 95  --save /home/pi/$

sudo curl -X POST https://content.dropboxapi.com/2/files/upload \
    --header "Authorization: Bearer ********" \
    --header "Dropbox-API-Arg: {\"path\": \"/door/$DATE.jpg\",\"mode\": \"add\",\"autorename\": true,\"mute\": false,\"strict_conflict\": false}" \
    --header "Content-Type: application/octet-stream" \
    --data-binary @/home/pi/$DATE.jpg

sudo rm  /home/pi/$DATE.jpg
