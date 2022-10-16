# Wiki build instructions

TODO: write this page nicely

- you need mkdocs (pip3 install mkdocs)
- also you need mkdocs-material from pip3
- to work on the documentation:
	- execute ./run.sh to host a local version on localhost:8088
	- when a file is changed, the website should refresh automatically
- to deploy the website on the server (notes for self):
	- execute ./deploy.sh
	- this will:
		- build the site to the ./site directory
		- remove the old fpgc folder on the server
		- copy the site directory to the server, with fpgc as name
		- recursively set group to www-data
	- just make sure you use public key authentication to the server

