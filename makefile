#
#
#
ROOT_DIR=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
DIR_NAME=$(notdir $(CURDIR))

# SSL URL for remote git server. override with ROOTURL=<URL>		
ROOTURL?=git@github.com/g40
# i.e. root repo name
ROOTNAME?=$$(basename $(CURDIR))
#
SUBS=git submodule foreach git 
#
all:
	-@echo "DIR=$(CURDIR)"
	-@echo "DIR_NAME=$(DIR_NAME)"
	-@echo "ROOT_DIR=$(ROOT_DIR)"
	-@echo "ROOT_DIR=$(lastword $(ROOT_DIR))"
	-@echo "MAKEFILE_LIST=$(MAKEFILE_LIST)"

# list all available targets
list:
	@LC_ALL=C $(MAKE) -pRrq -f $(firstword $(MAKEFILE_LIST)) : 2>/dev/null | awk -v RS= -F: '/(^|\n)# Files(\n|$$)/,/(^|\n)# Finished Make data base/ {if ($$1 !~ "^[#.]") {print $$1}}' | sort | grep -E -v -e '^[^[:alnum:]]' -e '^$@$$'

# check
check:
	@git status
	@git submodule foreach git status
	@git submodule foreach git remote -v
	
# move all repos to new remote
move:
	-@echo "Moving ..."
#	git submodule foreach git 
	git remote set-url git@github.com/g40/wde2
	git remote set-url $(ROOTURL)
# do the repo setup
setup:
	git init
	git status
	git add .
	git commit -m "Initial from squash"
	git remote add origin git@git.novadsp.com:g40/$(DIR_NAME).git
	git push -u origin master
	git submodule add git@git.novadsp.com:g40/g40.git modules\include\g40

# reduce to rubble
teardown:
	git status
	rm -rf submodules
	rm .gitmodules
	rm -rf .git

#
# move2:
#	git remote add origin git@github.com:g40/g40.git
#	git branch -M main
#	git push -u origin main
#
#	git remote add origin git@github.com:g40/wde2.git
#	git branch -M main
#	git push -u origin main
#

#------------------------------------------------
# Have multiple remotes?
#
# git remote set-url origin git@git.github.com:g40/wde2.git

#
# >git remote add gh git@github.com:g40/wde2.git
# >git push --repo=gh
# >git push gh
#
