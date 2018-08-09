#!/usr/bin/python

import serial, subprocess, sys, argparse, re, aenum
from aenum import Enum

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

class Alias(Enum):
    Donald_Duck = 3943002768
    Mickey = 3942906659
    @classmethod
    def sub(cls, line, escape):
        # if escape:
        aliasedLine = re.sub('\d{10}', lambda v: "\033[11;97m" + cls(int(v.group())).name + "\033[m", line)
        # else:
            # aliasedLine = re.sub('\d{10}', lambda v: "\033[11;97m" + cls(int(v.group())).name + "\033", line)
        return aliasedLine

def run_command(command):
    p = subprocess.Popen(command,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT)
    return iter(p.stdout.readline, b'')

def run_signal_comm(signal_user_id, signal_group_id, found_message):
    p = subprocess.Popen('signal-cli -u %s -m %s -g %s' % (signal_user_id, signal_group_id, found_message),
                         # shell=True
                         stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT)


def handle_c2_line(line, signalUserId, signalGroupId):
    line_pieces = re.split(r'\s+',line)
    if line_pieces and line_pieces[0].startswith("[FOUND]"):
        run_signal_comm(signalUserId,signalGroupId,line)
        print('\x1b[6;30;42m' + line + '\x1b[0m')
        print('\x1b[6;30;42m' + Alias.sub(line,False) + '\x1b[0m')
    elif line:
        print(Alias.sub(line,True))

#
#
def main(argv):
    # mode = 1
    # usage = 'c2.py -s <serialPort> -u <signalUserId> -g <signalGroupId>'

    parser = argparse.ArgumentParser(description='Keep track of a sniffly mesh network and issue signal messages accordingly')
    parser.add_argument('-s', "--serialPort",
                        help='The serial port to monitor',
                        required=True)
    parser.add_argument('-u', '--signalUserId',
                        help='The signal user id to send messages with',
                        required=True)
    parser.add_argument('-g','--signalGroupId',
                        help='The signal group id to send messages to',
                        required=True)
    parser.add_argument('-m','--mode',
                        help='The mode you would like to run in.\n mode 1 is via pyserial and mode 2 is via pio monitoring',
                        required=True)

    args = parser.parse_args()
    # if '-s' in args || '--serialPort'
    serialPort = args.serialPort
    signalUserId = args.signalUserId
    signalGroupId = args.signalGroupId
    mode = args.mode
    print ("TESTING SIGNAL OUT: " + 'signal-cli -u %s -m %s -g %s' % (signalUserId, signalGroupId, 'gi'))

    print 'Serial: ', serialPort
    print 'signalUserId: ', signalUserId
    print 'signalGroupId: ', signalGroupId

    try:
        if mode == 1:
            ser = serial.Serial(serialPort, 115200, timeout=.2)
            if ser.isOpen():
                ser.close()
            ser.open()
            while 1:
                bytesToRead = ser.inWaiting()
                if bytesToRead:
                    handle_c2_line(ser.readline().rstrip())

        elif mode == 2:
            command = ("pio device monitor --port %s --baud 115200" % serialPort).split()
            for line in run_command(command):
                handle_c2_line(line,signalUserId,signalGroupId)

    except KeyboardInterrupt:
        print "\nDied with honor."
        ser.close()

# class Alias(Enum):
#     _init_ = 'value text'
#     bob = 12345, 'Bob'
#     jon = 23456, 'Jon'
#     jack = 34567, 'Jack'
#     jill = 45678, 'Jill'
#     steph = 89012, 'Steph'
#     @classmethod
#     def sub(cls, line):
#         return re.sub('\d{5}', lambda v: cls(int(v.group())).text, line)

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
