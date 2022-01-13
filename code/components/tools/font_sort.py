
# Know nothing about copy from left to right,
# if someone wanna use it, just copy it from left to right
#                by author m24h


import numpy
import sys
import os
import struct

if len(sys.argv)<3 :
	print('Sort font and its coded idx file incremently according to font\'code ')
	print('!!! IMPORTANT: This will overwrite original file !!!')
	print('')
	print('Usage: python', sys.argv[0], '<bytes of every font in font file> <font file name> <idx file name>')
	print('Example: python' ,sys.argv[0], '24 hz.fon hz.idx')
	print('')
	print('If <bytes> is 0, I will try to guess using the font file size and the number of codes in idx file')
	exit(1)
	
bytes=int(sys.argv[1])
	
if not os.path.isfile(sys.argv[2]) :
	print('Font file "%s" not exists' % sys.argv[2])
	exit(1)

if not os.path.isfile(sys.argv[3]) :
	print('Idx file "%s" not exists' % sys.argv[3])
	exit(1)

fin=open(sys.argv[2], 'rb')
font=fin.read()
fin.close()

fin=open(sys.argv[3], 'rb')
idx=fin.read()
fin.close()

n=0
lenIdx=len(idx)
lenFont=len(font)
codes=[]
while n*4+4<=lenIdx:
	c=struct.unpack('<I',idx[n*4 : n*4+4])[0]
	if c==0:
		break
	codes.append((c, n))
	n+=1
print('%s codes read from idx file' % n)

if bytes==0:
	bytes=int(lenFont/n)
	print('I guess every font is %d bytes long' % bytes)
	if bytes<1:
		print('Wrong!')
		exit(1)
		
codes.sort(key=lambda elem : elem[0])

ffon= open(sys.argv[2], 'wb')
fidx= open(sys.argv[3], 'wb')
i=0
t=0
total=0
while i<n:
	t=codes[i][1]
	if t*bytes+bytes<=lenFont:
		fidx.write(struct.pack('<I', codes[i][0]))
		ffon.write(font[t*bytes : t*bytes+bytes])
		total+=1
	i+=1
	
fidx.write(struct.pack('<I', 0))
fidx.close()
ffon.close()

print('Totally %d font sorted and saved back' % total)
