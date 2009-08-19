#! /bin/sh
grep -oPr '(?<=case )OP_\w+(?=:)' prvm_edict.c > edictoplist && grep -oPr '(?<=case )OP_\w+(?=:)' prvm_execprogram.h > opcodelist && for i in `cat opcodelist`; do grep -q $i edictoplist || echo $i missing; done
rm -f edictoplist
rm -f opcodelist
