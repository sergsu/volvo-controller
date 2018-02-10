# WebastoHeaterWBusArduinoInterface
Arduino software for controlling Webasto ThermoTop heaters over its custom WBUS protocol


This is a from Stuart Pittaway's great work.
I just hacked it quickly to do the following thing:

* Use an Arduino Mega as a translator between a VW Remote key and an VW T5 supplemental heater (Webasto Thermotop C)
* The receiver box of the remote key sends the "supplemental heater on" cmd. Funnily, my supplemental heater starts only if it gets the "parking heater on" cmd. So I translate it.
* Also, I have to do a refresh every some seconds to keep it burning. The receiver box doesn't do it by itself.
* Also the "off" cmd would be translated, if sent by the remote key.

For now, I installed everything in the car and it's working nicely.
