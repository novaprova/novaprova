#!/bin/bash

TEST="$1"
xmllint --schema JUnit.xsd -noout reports/TEST-$TEST.xml || \
    echo 'FAIL xml schema validation failed'
