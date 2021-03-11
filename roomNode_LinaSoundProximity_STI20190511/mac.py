import os
import random
import time
a=True
while (a==True):
    asd=random.randint(1,100)
    if (asd>=1 and asd<=10):
        os.system("afplay TJA.wav", winsound.SND_ASYNC)
        time.sleep(1)
    if (asd>=11 and asd<=20):
        os.system("afplay IN.wav", winsound.SND_ASYNC)
        time.sleep(1)
    if (asd>=21 and asd<=30):
        os.system("afplay SPET.wav", winsound.SND_ASYNC)
        time.sleep(1)
    if (asd>=31 and asd<=40):
        os.system("afplay NAZAJ.wav", winsound.SND_ASYNC)
        time.sleep(1)
    if (asd>=41 and asd<=100):
        time.sleep(0.2)
