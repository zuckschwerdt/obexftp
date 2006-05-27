EXTRA_DIST = client.i charmap.i

if ENABLE_PERL
  MAYBE_PERL = perl
endif

if ENABLE_PYTHON
  # automake chokes on "MAYBE_PYTHON"?
  MAYBE_PYTHON_ = python
endif

if ENABLE_RUBY
  MAYBE_RUBY = ruby
endif

if ENABLE_TCL
  MAYBE_TCL = tcl
endif

SUBDIRS = $(MAYBE_PERL) $(MAYBE_PYTHON_) $(MAYBE_RUBY) $(MAYBE_TCL)

