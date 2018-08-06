import serial, time

ser = serial.Serial()
ser.baudrate = 115200
ser.port = '/dev/tty0'
ser.timeout = .1  # IMPORTANT, can be lower or higher
#ser.open()
if ser.isOpen():
    ser.close()
ser.open()
# ser = serial.Serial('/dev/tty0', 115200, timeout=2.5)

#attempt 1
while 1:
    ser.flushInput()
    ser.write("SHITTY MCSHIT SHIT"'\n')
    print ser.readline()

#attempt 2
# while True:
#     #cmd = raw_input("> ");
#     #ser.write(cmd + "\r\n")
#     #ret = ser.read(len(cmd)) # eat echo
#     #time.sleep( 0.2 )
#     while ( ser.inWaiting() ):
#         ret = ser.readline().rstrip()
#         print ret

#attempt 3
# while 1:
#     print("attempting serial read")
#     #serial_line = ser.readline()
#
#     bytesToRead = ser.inWaiting()
#     print bytesToRead
#     while bytesToRead:
#         ret = ser.readline(200)
#         print ret

    # if not serial_line.strip():
    #     continue
    # print(serial_line) # If using Python 2.x use: print serial_line
    # Do some other work on the data

    #time.sleep(300) # sleep 5 minutes

#     # Loop restarts once the sleep is finished

ser.close() # Only executes once the loop exits
