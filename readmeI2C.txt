
Fazit: es gibt immer nur stress, da das Board agressiv i2c blockiert, der
bme hat nur quatsch geliefert nachdem er erkannt wurde, daher eigenes
projekt sensor_esp mit eigenem esp32

Wire ist die Arduino-Abstraktion für I²C. Der ESP32-S3 hat zwei Hardware-I²C-Controller:

    Wire = I2C_NUM_0 – der erste Controller
    Wire1 = I2C_NUM_1 – der zweite Controller

Beide können auf beliebigen GPIO-Pins laufen – du sagst ihnen beim begin() welche Pins sie nutzen 
sollen. Das ist beim ESP32 flexibler als bei anderen Mikrocontrollern wo I²C-Pins fest verdrahtet 
sind.- das ist nicht sicher, besser beides auf Wire und bme nach dem display
initialisieren - das geht leider nicht, Arduinos Wire will neu
initialisieren, fehlschlag. 

Nächste Aussage der KI: 
Also doch Wire1 – und ja, es funktioniert auf denselben Pins. Der ESP32-S3 erlaubt das weil 
Wire1 I2C_NUM_1 nutzt, einen komplett anderen Hardware-Controller. 
Die Pins 19/20 können von beiden Controllern genutzt werden. - na ob das mal
kein Chaos gibt ...

Das Problem hier ist dass der GT911-Treiber in der smartdisplay-Bibliothek direkt über ESP-IDF 
auf I2C_NUM_0 zugreift – ohne Arduino's Wire zu nutzen. Wenn Wire.begin() vorher aufgerufen wird, 
hat Arduino bereits I2C_NUM_0 belegt und der ESP-IDF-Treiber kann ihn nicht nochmal installieren 
→ Crash.

Nach smartdisplay_init() hat der GT911 I2C_NUM_0 bereits über ESP-IDF initialisiert. 
Wenn dann Wire.begin(19, 20) kommt, erkennt Arduino dass der Bus schon läuft → deshalb die Warnung 
Bus already started in Master Mode – aber kein Crash, weil Arduino nur registriert statt neu 
installiert.

Mehrere Geräte auf demselben I²C-Bus ist völlig normal – GT911 (0x5D) und BME280 (0x76) teilen 
sich einfach die Leitung, jeder antwortet nur auf seine eigene Adresse. Das funktioniert mit Wire 
solange smartdisplay_init() zuerst läuft.

und nach einigem nachhaken noch die Erklärung
Jetzt habe ich eine gute Erklärung. Es gibt ein bekanntes Problem im ESP32-Arduino-Framework: 
Die Initialisierungsreihenfolge von Bus-Treibern spielt eine Rolle. Wenn I²C vor einem 
anderen Bus-Treiber (I²S, DMA, etc.) initialisiert wird, kann der später initialisierte 
Treiber die I²C-Bus-Kommunikation stören. Die Lösung ist, den anderen Treiber zuerst zu 
initialisieren. GitHub
In deinem Fall ist es analog: Der DMA-Treiber des Displays wird in smartdisplay_init() 
initialisiert. Wenn I²C (Wire1.begin()) vor dem DMA-Treiber initialisiert wird, 
funktioniert es – weil der I²C-Treiber zuerst seine Ressourcen reserviert und der DMA-Treiber 
danach seine eigenen Ressourcen unabhängig davon bekommt. Andersherum würde der DMA-Treiber 
Ressourcen belegen die I²C danach nicht mehr bekommt. GitHub
Das erklärt warum Wire1.begin() vor smartdisplay_init() funktioniert, aber nicht danach – es 
ist eine Ressourcen-Reihenfolge im ESP-IDF, nicht eine logische Abhängigkeit.



