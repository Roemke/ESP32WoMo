musste den Gasfernschalter abtrennen. Ist ein Truma gas fernschalter. daran
hängt ein livello AGC anzeige gerät. Verbunden über schwar rot (nimmt 12V)
und einen Ausgang von 5 V auf Grün. Intern muss ein ordentlicher Widerstand
sitzen. Vermutung claude.ai: Der Füllstandsensor stellt einen Widerstand
dar, ist er auf 90 Ohm, ist der Tank voll. bei 0 Ohm leer. Leer habe ich im
Womo nie gesehen, er zeigt noch eine grüne LED bei leer an(?)

Idee anzeigen auch im display
Kalibrierung in schritten noch nötig, nehme poti von 0-100 Ohm

Schaltung für ESP grünes Kabel - 9.83 kOhm --- 17.84 kOhm -- GND
                                            |--- ADC Esp (blau)
(9.83 sollte 10 sein :-), 17.84 durch 22 und 100 parallel)

nehme das in einen Schrumpfschlauch

Auflösung ADC reicht um mit dem geringen Spannungsunterschied zwischen 
Grün und GND zu messen, das sind nur bis ca 170 mV wenn ganz voll

Kabelfarben damit blau für eingang adc esp, grün kommt vom display, schwarz
kurz für esp, lang für gnd der Anzeige.


mal Messung der raw werte, 2 x 100 Ohm poti
raw    led
0      rot E
490    8/8
480    8/8 
480    7/8 
470    7/8
450    7/8
440    7/8
420    7/8
414    6/8
380    6/8
360    5/8
322    5/8
290    5/8
270    4/8
250    4/8
250    3/8
220    3/8
180    3/8 
150    2/8 
125    2/8
120    2/8 springt auch auf 1/8
100    1/8 
70     1/8
70     0/8
60     0/8
0      0/8

bei fester widerstandseinstellung sinkt raw, bis sich die led bewegt, dann
geht raw wieder hoch und sinkt wieder 

auch bei 200 Ohm habe ich werte die massiv hochgehen raw 600, danach pendelt es sich
auf niedrigen werten zwischen 400 und 480 ein manchmal auch bei 550, dann
geht led auf 8/8 klar, insgesamt seltsames verhalten, hängt auch am drehen 
der potis sehr seltsam 