#!/usr/bin/python

import serial, subprocess, sys, getopt, re, pydbus
from pydbus import SystemBus

import sys

# ser = serial.Serial()
# ser.baudrate = 115200
# ser.port = '/dev/ttyUSB0'
# ser.timeout = 1.5  # IMPORTANT, can be lower or higher
# #ser.open()
# if ser.isOpen():
#     ser.close()
# ser.open()
# ser = serial.Serial('/dev/tty0', 115200, timeout=2.5)

# #attempt 1
# while 1:
#     ser.flushInput()
#     ser.write("SHITTY MCSHIT SHIT"'\n')
#     print ser.readline()

# # attempt 2
# while True:
#     cmd = raw_input("> ");
#     ser.write(cmd + "\r\n")
#     ret = ser.read(len(cmd)) # eat echo
#     time.sleep( 0.2 )
#     while ( ser.inWaiting() ):
#         ret = ser.readline().rstrip()
#         print ret

# attempt 3
# while 1:
#     #print("attempting serial read")
#     #serial_line = ser.readline()
#
#     bytesToRead = ser.inWaiting()
#     #print bytesToRead
#     while bytesToRead:
#         serial_line = ser.readline(bytesToRead)
#         print serial_line
#
#         if not serial_line.strip():
#             continue
# print(serial_line) # If using Python 2.x use: print serial_line
# Do some other work on the data

# time.sleep(300) # sleep 5 minutes

#     # Loop restarts once the sleep is finished

# ser.close() # Only executes once the loop exits

def run_command(command):
    p = subprocess.Popen(command,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT)
    return iter(p.stdout.readline, b'')

def handle_c2_line(line):
    line_pieces = re.split(r'\s+',line)
    if line_pieces and line_pieces[0].startswith("[FOUND]"):
        print('\x1b[6;30;42m' + 'SIGNAL COMM WILL GO HERE' + '\x1b[0m')
        print('\x1b[6;30;42m' + line + '\x1b[0m')
    elif line:
        print(line)

#
#
def main(argv):
    mode = 1
    serialPort = ''
    # outputfile = ''
    try:
        opts, args = getopt.getopt(argv, "hs:", ["serialPort="])
    except getopt.GetoptError:
        print 'test.py -s <serialPort>'
        sys.exit(2)
    if not argv:
        print 'test.py -s <serialPort>'
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print 'test.py -s <serialPort>'
            sys.exit()
        elif opt in ("-s", "--serialPort"):
            serialPort = arg
            # elif opt in ("-o", "--ofile"):
            #     outputfile = arg
    print 'Serial: "', serialPort


    try:
        if mode == 1:
            ser = serial.Serial(serialPort, 115200, timeout=.1)

            while 1:
                while ser.inWaiting():
                    handle_c2_line(ser.readline().rstrip())

        elif mode == 2:
            command = ("pio device monitor --port %s --baud 115200" % serialPort).split()
            for line in run_command(command):
                handle_c2_line(line)

    except KeyboardInterrupt:
        print "Died with honor."


if __name__ == "__main__":
    main(sys.argv[1:])

# def runProcess(exe):
#     p = subprocess.Popen(exe, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
#     while(True):
#       retcode = p.poll() #returns None while subprocess is running
#       line = p.stdout.readline()
#       yield line
#       if(retcode is not None):
#         break
#
# for line in runProcess('pio device monitor --port /dev/ttyUSB0 --baud 115200'.split()):
#     print line,
