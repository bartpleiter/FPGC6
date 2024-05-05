#!/bin/sh

python3 -m mkdocs build --clean

# Obviously I use public key authentication to login.
# No secret server information is displayed here :P
ssh 192.168.0.222 'rm -rf /var/www/b4rt.nl/html/fpgc/'
rsync -r site/ 192.168.0.222:/var/www/b4rt.nl/html/fpgc/
ssh 192.168.0.222 'chown bart:www-data -R /var/www/b4rt.nl/html/fpgc/'
