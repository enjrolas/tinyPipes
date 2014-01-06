#!/usr/bin/python
import math
import serial
def sign(plainText, privateKey):
    cipherText = ""
    n = privateKey[0]
    e = privateKey[1]
    j = 0

    for i in plainText:
        temp = pow(ord(i), e, n)
        M = divmod(temp, 0x100)
        cipherText += chr(M[1])
        cipherText += chr(M[0])

    print "++++++++++BEGIN CIPHERTEXT++++++++++++"
    print cipherText
    print "+++++++++++END CIPHERTEXT+++++++++++++"
    cipherString="{ "
    for i in cipherText:
        cipherString+="%d, "% ord(i)
    cipherString+="}"
    print cipherString
    
    return cipherText

def pad(string, targetLength):
    for i in range(0, targetLength-len(string)):
        string+=" "
    return string
    
privateKey= (14351, 11);
publicKey = (14351, 1283);
message="isn't it a little too late for this?"
message=pad(message, 80)
signed=sign(message, privateKey)
# configure the serial connections (the parameters differs on the device you are connecting to)
ser = serial.Serial('/dev/tty.usbserial-A901N8FN', 115200)
if ser.isOpen():
    ser.close()
ser.open()
print len(signed)
for i in signed:   
    ser.write(i)

while True:
    print ser.inWaiting()
