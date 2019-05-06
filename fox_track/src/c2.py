#!/usr/bin/python
import serial, subprocess, sys, argparse, re, datetime


def run_command(command):
    p = subprocess.Popen(command,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT)
    return iter(p.stdout.readline, b'')

def run_signal_comm(signal_user_id, signal_group_id, found_message):
    print('SIGNAL COMMAND:', 'signal-cli -u %s send -m \"%s\" -g %s &' % (signal_user_id, found_message,signal_group_id))
    command = 'signal-cli -u %s send -m \"%s\" -g %s &' % (signal_user_id, found_message,signal_group_id)
    p = subprocess.Popen(command,
                          shell=True,
                          stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT)


def handle_c2_line(line, signalUserId, signalGroupId):
    # line_pieces = re.split(r'\s+',line)
    line_pieces = line.split('|')
    if line_pieces and line_pieces[0].startswith("[FOUND]"):
        if signalMode:
            print("Begin signal comm")
            run_signal_comm(signalUserId,signalGroupId,line)
            print("End signal comm")
        mac = line_pieces[1].strip()
        channel = line_pieces[2].strip()

        with open("Mac","w") as text_file:
            text_file.write(mac)
        with open("Channel","w") as text_file:
            text_file.write(channel)

        print('\x1b[6;30;42m' + str(datetime.datetime.time(datetime.datetime.now())) + '\x1b[0m')
        print('\x1b[6;30;42m' + line + '\x1b[0m')
        sys.exit()

    elif line:
        print (line)

signalMode=False
def main(argv):
    parser = argparse.ArgumentParser(description='Keep track of a sniffly mesh network and issue signal messages accordingly')
    parser.add_argument('-s', "--serialPort",
                        help='The serial port to monitor',
                        required=True)
    parser.add_argument('-u', '--signalUserId',
                        help='The signal user id to send messages with',
                        required=False)
    parser.add_argument('-g','--signalGroupId',
                        help='The signal group id to send messages to',
                        required=False)
    parser.add_argument('-m','--mode',
                        help='The mode you would like to run in.\n mode 1 is via pyserial and mode 2 is via pio monitoring',
                        required=True)

    args = parser.parse_args()
    serialPort = args.serialPort
    if (args.signalGroupId and not args.signalUserId) or (args.signalUserId and not args.signalGroupId):
        print "Signal group ID and signal user ID are mututally dependent arguments and must be supplied together."
        parser.print_help()
        sys.exit()
    signalUserId = args.signalUserId
    signalGroupId = args.signalGroupId
    signalMode = args.signalUserId is not None
    mode = args.mode

    # print ("TESTING SIGNAL OUT: " + 'signal-cli -u %s -m %s -g %s' % (signalUserId, signalGroupId, 'gi'))
    print 'Serial: ', serialPort
    print 'signalUserId: ', signalUserId
    print 'signalGroupId: ', signalGroupId
    print 'signalMode: ', signalMode
    print 'mode: ', mode

    try:
        if mode == '1':
            ser = serial.Serial(serialPort, 115200, timeout=.25)
            if ser.isOpen():
                ser.close()
            ser.open()

            while 1:
                bytesToRead = ser.inWaiting()
                if bytesToRead:
                    handle_c2_line(ser.readline().rstrip(), signalUserId, signalGroupId)

        elif mode == '2':
            command = ("pio device monitor --port %s --baud 115200" % serialPort).split()
            for line in run_command(command):
                handle_c2_line(line, signalUserId, signalGroupId)

    except KeyboardInterrupt:
        print "\nKilled.\n"
        ser.close()


if __name__ == "__main__":
    main(sys.argv[1:])
