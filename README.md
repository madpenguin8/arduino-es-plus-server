arduino-es-plus-server
======================

Arduino based server that gives Gardner Denver® air compressors 
equipped with the ES+® controller ethernet connectivity.

Data from the controller is returned as a JSON object containing
all three requests to the controller, they are:

 operating data <STX>oo<ETX>
 operating mode <STX>mm<ETX>
 service data <STX>BB<ETX>
 
 This is an example of the JSON data returned:
 
{
  "opdata": "059806270C6A0B9F065", 
  "opmode": "110D", 
  "servicedata": "00C3$27400753#20400752#28400752#28400752#28400752#27400744#28400744#28400744#06400744#08400742$400795#309892$000009#000030#000000#000000#000000#000043#346025#025206#016686#007899#004695#000202$400713#400731$0000736"
}

Currently only 9600 baud is supported but support is planned for 1200 as well.

This software in theory should work on RS2000® controllers as well since the
requests and responses to and from the controllers are identical to ES+®. 
However, in practice it seems to fail. I have not had time to track down the
problem but I think it is in the delay() in the Serial.available() loop.

Hardware used in this configuration is an Arduino UNO, Arduino Ethernet shield
( Wiznet 5100 ) and a typical MAX232 TTL to RS232 level shifter circuit.


