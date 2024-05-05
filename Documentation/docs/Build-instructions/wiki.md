# Wiki build instructions

This wiki is built using mkdocs, which runs on Python3

## Installation

```bash
pip3 install mkdocs mkdocs-material mkdocs-awesome-pages-plugin
```
These packages are also included in the `requirements.txt` of this repo.

## Run site (development)

Run `./run.sh` to host a local version on `localhost:8088`.

When a file is changed, the website should refresh automatically.


## Deploy site

To deploy the website on my server (only I can do this ofc), run `./deploy.sh`. Just make sure you use public key authentication to the server.

This will:

- build the site to the ./site directory
- remove the old fpgc folder on the server
- copy the site directory to the server, with fpgc as name
- recursively set group to www-data
