export YKSM_TITLE		:= 	YKSM
export YKSM_DESCRIPTION	:=	Yo-kai Watch save manager
export YKSM_AUTHOR		:=	ArchieDxncan

export VERSION_MAJOR	:=	0
export VERSION_MINOR	:=	1
export VERSION_MICRO	:=	0
GIT_REV					:=	$(shell git rev-parse --short HEAD)
OLD_INFO				:=	$(shell if [ -e appinfo.hash ]; then cat appinfo.hash; fi)
NOW_INFO				:=	$(YKSM_TITLE) $(YKSM_DESCRIPTION) $(YKSM_AUTHOR) $(VERSION_MAJOR) $(VERSION_MINOR) $(VERSION_MICRO) $(GIT_REV)
REVISION_EXISTS			:=	$(shell if [ ! -e common/include/revision.h ]; then echo 1; fi)

OUTDIR			:= 	out
RELEASEDIR		:=	release
ICON			:=	assets/icon.png

ifeq ($(OS),Windows_NT)
 ifeq ($(shell where py),)
  export PYTHON := python
 else
  export PYTHON := py -3
 endif
else
 export PYTHON := python3
endif

debug: 3ds-debug

release: 3ds-release docs

compile-commands: 3ds-compile-commands

revision:
	@mkdir -p common/include
ifneq ($(NOW_INFO),$(OLD_INFO))
	@echo \#define GIT_REV \"$(GIT_REV)\" > common/include/revision.h
	@echo \#define VERSION_MAJOR $(VERSION_MAJOR) >> common/include/revision.h
	@echo \#define VERSION_MINOR $(VERSION_MINOR) >> common/include/revision.h
	@echo \#define VERSION_MICRO $(VERSION_MICRO) >> common/include/revision.h
	@echo "$(NOW_INFO)" > appinfo.hash
else
 ifneq ($(REVISION_EXISTS),)
	@echo \#define GIT_REV \"$(GIT_REV)\" > common/include/revision.h
	@echo \#define VERSION_MAJOR $(VERSION_MAJOR) >> common/include/revision.h
	@echo \#define VERSION_MINOR $(VERSION_MINOR) >> common/include/revision.h
	@echo \#define VERSION_MICRO $(VERSION_MICRO) >> common/include/revision.h
 endif
endif

3ds-debug: revision
	$(MAKE) -C 3ds

3ds-release: revision
	$(MAKE) -C 3ds RELEASE="1"

3ds-compile-commands: revision
	$(MAKE) -C 3ds clean
	$(MAKE) -C 3ds GENERATE_COMPILE_COMMANDS=1
	@mv 3ds/build/compile_commands.json 3ds_compile_commands.json
	@echo 3DS compile commands written to 3ds_compile_commands.json

tests:
	$(MAKE) -C tests

docs:
	@mkdir -p $(OUTDIR)
	@gwtc -o $(OUTDIR) -n "$(YKSM_TITLE) Manual - v$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_MICRO)" -t "$(YKSM_TITLE) v$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_MICRO) Documentation" --logo-img $(ICON) docs/wiki

clean:
	@rm -f appinfo.hash
	@rm -f common/include/revision.h
	$(MAKE) -C 3ds clean

spotless: clean
	$(MAKE) -C 3ds spotless

format:
	$(MAKE) -C 3ds format
	$(MAKE) -C core format

cppcheck:
	$(MAKE) -C 3ds cppcheck

cppclean:
	$(MAKE) -C 3ds cppclean

.PHONY: debug release 3ds-debug 3ds-release tests docs clean spotless format cppcheck cppclean
