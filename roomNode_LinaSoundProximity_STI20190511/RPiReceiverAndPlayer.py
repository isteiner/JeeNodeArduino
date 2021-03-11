# Receiver of messages for JeeNodes sensors via USB and play sound random
# Programmed by Igor Steiner
# 2019-05-12	for Lina sound project

import serial, os
import sys
import random
import time

prevCount = 255
t0_hour = t0_day = t0_month = time.localtime()
print "time: ",time.asctime()

class PIRsensor:
	def __init__(self, input_string):
		self.get_string_split = input_string[:15].split() # split string but only first 15 characters
		self.count = int(self.get_string_split[3])
		global prevCount
		if self.count <> prevCount:
			prevCount = self.count
			print "Counter: ", self.count
			soundNr=random.randint(1,4)
			if (soundNr == 1):
				file = "IN_NAZAJ.wav"
			if (soundNr == 2):
				file = "IN_NAZAJ.wav"		
			if (soundNr == 3):
				file = "SPET_TJA.wav"
			if (soundNr == 4):
				file = "SPET_TJA.wav"				
			os.system("omxplayer " + "/home/pi/Music/" + file)
			print "soundNr: ", soundNr, " file: ", file
			time.sleep(1)
	def displayValues(self):
	   print "count: ", self.count
			
PORT = '/dev/ttyUSB0'	# set tty port
BAUD_RATE = 57600
serial_port = serial.Serial(PORT, BAUD_RATE)	#open serial port

# check if new message from sensor arrives
while True:
	serial_string = serial_port.readline()	#read line from tty

	#external sensor PIR roomNode id=33
	if serial_string[:5] == "OK 33":	
		ext_sens = PIRsensor(serial_string)
		#ext_sens.displayValues()
