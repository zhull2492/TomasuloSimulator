EXE_A = Makefile2
EXE_B = Makefile3

MAKEOBJS = $(shell find . -type f -name 'Makefile_*')

BUILDDIR = build
BINDIR = bin
COMDIR = common

#MY_TARGETS = $(EXE_A) $(EXE_B)
MY_TARGETS = $(MAKEOBJS)

.PHONY: all $(MY_TARGETS)

all: $(MY_TARGETS)

$(MY_TARGETS):
	@$(MAKE) -f $@

.PHONY: clean

clean:
	@echo " Cleaning...";
	@echo " $(RM) -r $(BUILDDIR) $(BINDIR)/* $(COMDIR)/*.o"; $(RM) -r $(BUILDDIR) $(BINDIR)/* $(COMDIR)/*.o
