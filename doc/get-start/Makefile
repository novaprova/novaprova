#
#  Copyright 2011-2012 Gregory Banks
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

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

docs: $(DELIVERABLES)

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

