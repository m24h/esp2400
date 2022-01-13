from PIL import Image
import numpy
import sys
import os
import struct

if len(sys.argv)<3 :
	print('Make ST7789 style RGB565 image')
	print('Usage: python',sys.argv[0],'<image file name> <output file name>')
	print('Example: python',sys.argv[0],'a.jpg b.bin')
	exit(1)

if os.path.exists(sys.argv[2]):
	print('Output file name "%s" already exists, not overwrited' % sys.argv[2])
	exit(1)
	
fout=open(sys.argv[2], 'wb')
	
img = numpy.array(Image.open(sys.argv[1]).convert('RGB'))
(h,w,p)=img.shape
for y in range(h):
	for x in range(w):
		t=((img[y,x,0] & 0x00f8)<<8)|((img[y,x,1] & 0x00fc)<<3)|((img[y,x,2] & 0x00f8)>>3)
		fout.write(struct.pack('>H', t))

print("Total %d bytes writen" % (h*w*2))
fout.close()
exit(0)