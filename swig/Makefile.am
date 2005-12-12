EXTRA_DIST = client.i

if ENABLE_PERL
  MAYBE_PERL = perl
endif

if ENABLE_PYTHON
  MAYBE_PYTHON = python
endif

if ENABLE_TCL
  MAYBE_TCL = tcl
endif

if ENABLE_SWIG
  MAYBE_TEST = junk
endif

SUBDIRS = $(MAYBE_PERL) $(MAYBE_PYTHON) $(MAYBE_TCL)

