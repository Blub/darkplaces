#! /bin/sh
grep -oPr '(?<=case )OP_\w+(?=:)' prvm_edict.c > edictoplist
grep -oPr '(?<=case )OP_\w+(?=:)' prvm_execprogram.h > opcodelist
for i in `cat opcodelist`; do
  grep -qP "^$i\$" edictoplist || echo $i missing
done
rm -f edictoplist
rm -f opcodelist
