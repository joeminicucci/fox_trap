#!/usr/bin/python
import sys
import argparse
import serial
import time

def main(argv):
    # mode = 1
    parser = argparse.ArgumentParser(description='Update the C2. Commands (rem, tar) are meant to contain a single MAC address in lowercase format, e.g. 1337af666666.')
    parser.add_argument('-s', "--serialPort",
    help='The serial port to monitor',
    required=True)

    parser.add_argument('-c', "--command",
    help='The command to issue',
    required=True)

    parser.add_argument('-i', "--interactive",
    help='stdin interactive mode',
    required=False,
    action='store_true')

    args = parser.parse_args()
    # if '-s' in args || '--serialPort'
    serialPort = args.serialPort
    command = args.command
    interactive = args.interactive

    writeCommand(command,serialPort,interactive)


def writeCommand(command, serialPort, interactive):
    try:
        serialCom = serial.Serial(serialPort, 115200)
        if serialCom.isOpen():
            serialCom.close()

        serialCom.open()
        commToSerial(command, serialCom);
        print 'wrote ' + command + ' to serial port ' + serialPort

        while (interactive):
            time.sleep(0.1)
            command = sys.stdin.readline()
            print 'command is ' + command
            if len(command) > 16:
                print 'command is too long, try again'
                continue
            else:
                commToSerial(command, serialCom)
    except KeyboardInterrupt:
        print '\n KILLED\n'
        serialCom.close()

def commToSerial(command, serialCom):
    # if serialCom.isOpen():
    #     serialCom.close()
    #
    # serialCom.open()
    serialCom.flushInput()
    serialCom.write(command+'\n')

if __name__ == "__main__":
        main(sys.argv)

        # def send_command(serialPort, command):
        # 	comm = serial.Serial(serialPort, 115200)
        # 	if comm.isOpen():
        # 		comm.close()
        # 	comm.open()
        # 	comm.flushInput()
        # 	comm.write(command+'\n')
        # 	time.sleep(0.1)
        #     return comm.readline()
