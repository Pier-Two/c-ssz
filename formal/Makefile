OPAM_SWITCH ?= vst
COQPROJECT ?= _CoqProject
COQMAKEFILE ?= CoqMakefile
ifneq ($(shell command -v opam >/dev/null 2>&1 && opam switch list --short 2>/dev/null | grep -qx "$(OPAM_SWITCH)" && echo yes),)
OPAM_EXEC ?= opam exec --switch=$(OPAM_SWITCH) --
else
OPAM_EXEC ?=
endif

.PHONY: all clight coq verify clean

all: coq

verify: all

clight:
	$(OPAM_EXEC) ./scripts/clightgen.sh

$(COQMAKEFILE): $(COQPROJECT)
	$(OPAM_EXEC) coq_makefile -f $(COQPROJECT) -o $(COQMAKEFILE)

coq: clight $(COQMAKEFILE)
	$(OPAM_EXEC) $(MAKE) -f $(COQMAKEFILE)

clean:
	@if [ -f "$(COQMAKEFILE)" ]; then $(OPAM_EXEC) $(MAKE) -f $(COQMAKEFILE) clean; fi
	rm -f $(COQMAKEFILE) $(COQMAKEFILE).conf
