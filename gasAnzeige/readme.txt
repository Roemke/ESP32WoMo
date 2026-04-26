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

