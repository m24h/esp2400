# -*- coding: utf-8 -*-

# Know nothing about copy from left to right,
# if someone wanna use it, just copy it from left to right
#                by author m24h
 
import numpy
import sys
import os
import struct

if len(sys.argv)<3 :
	print('Convert font idx file to 32bit coded compatible with font.c')
	print('!!!Support utf-8 idx file only!!!  Please transfer the idx file to UTF-8 through other tools first')
	print('Usage: python', sys.argv[0], ' <idx file name> <output file name>')
	print('Example: python' ,sys.argv[0], 'hz.txt hz2.idx')
	exit(1)
	
if not os.path.isfile(sys.argv[1]):
	print('Idx file "%s" not exists' % sys.argv[1])
	exit(1)
	
if os.path.exists(sys.argv[2]):
	print('Output file name "%s" already exists, not overwrited' % sys.argv[2])
	exit(1)

fin=open(sys.argv[1], 'r', encoding='utf-8')
idx=fin.read()
fin.close()

fout= open(sys.argv[2], 'wb')
n=len(idx)
i=0
while (i<n):
	if (idx[i]=='\r' or idx[i]=='\n' or idx[i]=='\0'):
		break;
	fout.write(struct.pack('<I',ord(idx[i])))
	i+=1
	
fout.write(struct.pack('<I', 0))
fout.close()

