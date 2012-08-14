#!/bin/bash
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

exp_conc=1
if [ $# -gt 1 ] ; then
    log="$1.$2.log"
    case "$2" in
    -j*) exp_conc=${2#-j} ;;
    esac
else
    log="$1.log"
fi

awk -v exp_conc=$exp_conc '
BEGIN {
    maxn = 0;
    n = 0;
    startt = 0.0;
    lastt = 0.0;
    tott = 0.0;
    first = 1;
    epsilon = 0.1;
}
function delta(ts, d) {
    if (first) {
	lastt = startt = ts;
	first = 0;
    }
    tott += n * (ts - lastt);
    lastt = ts;
    n += d;
    if (n > maxn)
	maxn = n;
}
/ begins$/ {
    delta(0+$1, 1);
}
/ ends$/ {
    delta(0+$1, -1);
}
END {
    printf "Maximum concurrency: %d\n", maxn;

    if (maxn != exp_conc)
	printf "FAIL expected maximum concurrency %d got %d\n",
	    exp_conc, maxn;
    else
	printf "PASS good maximum concurrency\n";

    avg_conc = tott/(lastt - startt);
    printf "Average concurrency: %.2f\n", avg_conc;

    if (avg_conc < (1-epsilon)*exp_conc ||
	avg_conv > (1+epsilon)*exp_conc)
	printf "FAIL expected average concurrency between %.2f and %.2f got %.2f\n",
	    (1-epsilon)*exp_conc,
	    (1+epsilon)*exp_conc,
	    avg_conc;
    else
	printf "PASS good average concurrency\n";
}
' $log
