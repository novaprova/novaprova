
MARKDOWN= \
    PYTHONPATH=$(shell cd ../../.. ; pwd)/markdown-2.1.1/Markdown-2.1.1 \
    python -m markdown.__main__
# MARKDOWN=   python -m markdown
MDFLAGS=    -x codehilite -x headerid -x fenced_code
PYGMENTIZE= pygmentize

GEN_DELIVERABLES= \
	pygmentize.css \
	$(patsubst %.md,%.html,$(wildcard *.md))

STATIC_DELIVERABLES= \

DELIVERABLES= $(GEN_DELIVERABLES) $(STATIC_DELIVERABLES)

all install:

documentation: $(DELIVERABLES)

pygmentize.css:
	$(PYGMENTIZE) -S default -f html -a .codehilite > $@

%.html: %.md prologue.html epilogue.html
	( \
	    cat prologue.html ;\
	    $(MARKDOWN) $(MDFLAGS) $< || exit 1;\
	    cat epilogue.html \
	) > .$@.tmp && /bin/mv -f .$@.tmp $@

clean:

distclean:
	$(RM) $(GEN_DELIVERABLES)

