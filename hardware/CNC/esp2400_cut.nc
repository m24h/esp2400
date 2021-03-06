%
/ use D=1mm tool

G90
G21
G17

F1000
G00 Z20
G00 X0 Y0

M0
M3

/ cut usb 3.16*9mm, equal to 2.16*8mm when using D=1mm tool
F1000 G00 Z20
F1000 G00 X62.3 Y24.2
F1000 G00 Z5
F200 G01 Z0

G91

F50 G01 Z-0.5
F200 G01 X5.84
F200 G03 X0 Y2.16 R1.08
F200 G01 X-5.84
F200 G03 X0 Y-2.16 R1.08

F50 G01 Z-0.5
F200 G01 X5.84
F200 G03 X0 Y2.16 R1.08
F200 G01 X-5.84
F200 G03 X0 Y-2.16 R1.08

F50 G01 Z-0.5
F200 G01 X5.84
F200 G03 X0 Y2.16 R1.08
F200 G01 X-5.84
F200 G03 X0 Y-2.16 R1.08

F50 G01 Z-0.5
F200 G01 X5.84
F200 G03 X0 Y2.16 R1.08
F200 G01 X-5.84
F200 G03 X0 Y-2.16 R1.08

G90

F200 G01 Z0
F1000 G00 Z20
F1000 G00 X0 Y0

/cut edge
/left additional 0.1mm for top and bottom edge
M0
G90
F1000 G00 Z20
F1000 G00 X-0.5 Y-0.6
F1000 G00 Z5
F200 G01 Z0

G91
F50 G01 Z-0.5
F200 G01 X82.0
F200 G01 Y20.4
F200 G01 X-9.0
F200 G01 Y8.4
F200 G01 X-64.0
F200 G01 Y-8.4
F200 G01 X-9.0
F200 G01 Y-20.4

F50 G01 Z-0.5
F200 G01 X82.0
F200 G01 Y20.4
F200 G01 X-9.0
F200 G01 Y8.4
F200 G01 X-64.0
F200 G01 Y-8.4
F200 G01 X-9.0
F200 G01 Y-20.4

F50 G01 Z-0.5
F200 G01 X82.0
F200 G01 Y20.4
F200 G01 X-9.0
F200 G01 Y8.4
F200 G01 X-64.0
F200 G01 Y-8.4
F200 G01 X-9.0
F200 G01 Y-20.4

F50 G01 Z-0.5
F200 G01 X82.0
F200 G01 Y20.4
F200 G01 X-9.0
F200 G01 Y8.4
F200 G01 X-64.0
F200 G01 Y-8.4
F200 G01 X-9.0
F200 G01 Y-20.4

G90

F200 G01 Z0
F1000 G00 Z20
F1000 G00 X0 Y0

M5
%
