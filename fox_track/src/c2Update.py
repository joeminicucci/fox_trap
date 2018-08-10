#!/usr/bin/python

def main(argv):
    # mode = 1
    # usage = 'c2.py -s <serialPort> -u <signalUserId> -g <signalGroupId>'

    parser = argparse.ArgumentParser(description='Update the C2')
    parser.add_argument('-s', "--serialPort",
    help='The serial port to monitor',
    required=True)
    parser.add_argument('-c', "--command",
    help='The command to issue',
    required=True)

    args = parser.parse_args()
    # if '-s' in args || '--serialPort'
    serialPort = args.serialPort
    command = args.command
    comm = serial.Serial(serialPort, 115200)
    if comm.isOpen():
        comm.close()
    comm.open()
    comm.flushInput()
    comm.write(command+'\n')
    time.sleep(0.1)
    comm.readline()


        # def send_command(serialPort, command):
        # 	comm = serial.Serial(serialPort, 115200)
        # 	if comm.isOpen():
        # 		comm.close()
        # 	comm.open()
        # 	comm.flushInput()
        # 	comm.write(command+'\n')
        # 	time.sleep(0.1)
        #     return comm.readline()
