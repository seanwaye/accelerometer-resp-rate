import serial
import matplotlib.pyplot as plt
import numpy as np
import win32com.client
import csv

connected = False

#finds COM port that the Arduino is on (assumes only one Arduino is connected)
wmi = win32com.client.GetObject("winmgmts:")
#comPort = "/dev/serial/by-id/usb-FTDIBUS/VID_0403+PID_6001+A403A4KVA/0000"
print wmi.InstancesOf("Win32_SerialPort")
for port in wmi.InstancesOf("Win32_SerialPort"):
    print "in loop" # port.Name #port.DeviceID, port.Name
    if "Arduino" in port.Name:
        comPort = port.DeviceID
        print comPort, "is Arduino"

ser = serial.Serial('COM3', 9600) #sets up serial connection (make sure baud rate is correct - matches Arduino)

while not connected:
    serin = ser.read()
    connected = True


plt.ion()                    #sets plot to animation mode

length = 5                 #determines length of data taking session (in data points)
x = [0]*length               #create empty variable of length of test
y = [0]*length
z = [0]*length

xline, = plt.plot(x)         #sets up future lines to be modified
yline, = plt.plot(y)
zline, = plt.plot(z)
plt.ylim(450,550)        #sets the y axis limits
breathCounter = 0
counter = 0
for i in range(10*length):     #while you are taking data
    counter +=counter
    data = ser.readline()    #reads until it gets a carriage return. MAKE SURE THERE IS A CARRIAGE RETURN OR IT READS FOREVER
    sep = data.split()      #splits string into a list at the tabs
    #print sep
   
    x.append(int(sep[0]))   #add new value as int to current list
    y.append(int(sep[1]))
    z.append(int(sep[2]))
    if sep[1] > 500 and sep[2] > 500:     #500 arbitrarily set as threshold value
        print "Breath"     #prints breath when accelerometer reading is above a predetermined threshold value
        breathCounter += breathCounter
  
    del x[0]
    del y[0]
    del z[0]
   
    xline.set_xdata(np.arange(len(x))) #sets xdata to new list length
    yline.set_xdata(np.arange(len(y)))
    zline.set_xdata(np.arange(len(z)))
   
    xline.set_ydata(x)                 #sets ydata to new list
    yline.set_ydata(y)
    zline.set_ydata(z)

    #with open('data.csv', 'w') as csvfile:     #Writes to csv file
    #fieldnames = ['X-AXIS', 'Y-AXIS', 'Z-AXIS']
    #writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

    #writer.writeheader()
    #writer.writerow({'X-AXIS': x, 'Y-AXIS': y, 'Z-AXIS': z})
 
    plt.pause(0.001)                   #in seconds
    if (counter%length) == 0:
        plt.savefig('breathingplot.pdf')
    else:
        plt.draw()                         #draws new plot


rows = zip(x, y, z)                  #combines lists together
plt.savefig('lastplot.pdf')
print 'saved'
row_arr = np.asarray(rows)               #creates array from list
print row_arr
np.savetxt("test_radio2.csv", row_arr, delimiter=",") #save data in file (load w/np.loadtxt())

ser.close() #closes serial connection (very important to do this! if you have an error partway through the code, type this into the cmd line to close the connection)
