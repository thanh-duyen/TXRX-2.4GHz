The reciver for testing is reused from here: https://www.youtube.com/watch?v=fhfw287tRes&t=444s
List of components for TX circuit:
1. Button 3x4mm 2 Pins x2
2. 18650 holder smd x1
3. Button FP-1825027-8-MFG x3
4. Capacitor 0.1uF 0603 x3
5. Capacitor 10uF 0603 x2
6. LED 0603 x1
7. ESP32_WROOM x1
8. ENCODER 24 PPR x1
9. USB Type-C 12 Pins x1
10. JoyStick PS5 x2
11. Switch 6 Pins SMD x3
12. Resistor 100K 0603 x10
13. Resistor 1K 0603 x1
14. Resistor 1K2 0603 x1
15. Resistor 47K 0603 x1
16. Resistor 10K 0603 x1
17. SPX3819M5-L-3-3 x2
18. Button 6x6x6mm 4 Pins x6
19. TFT SPI 13 Pins 80x160 x1
20. nRF24l01 + PA + LNA DIG x1
21. TP4056 x1

List of components for RX circuit version 1:
1. Capacitor 0603 0.1uF x6
2. FP-TAJC-MFG 100uF x1
3. FP-FKD8-MFG 330uF x1
4. LED 0603 x8
5. Diode 1N5819HW x1
6. BUTTON 3x4mm 2 Pin x1
7. USB Type-C 12 Pins x1
8. Inductor 100uH x1
9. Resistor 0603 2K2 x6
10. Resistor 0603 1K x2
11. Resistor 0603 1K2 x1
12. Resistor 0805 180 x1
13. Resistor 0805 0.33 x1
14. Resistor 0603 47K x1
15. Resistor 0603 15K x1
16. Resistor 0603 10k x1
17. SPX3819M5-L-3-3 x1
18. MSS-22D18G2 SWITCH x1
19. TQFP32 ATmega328-AU x1
20. MC34063ACD-TR x1
21. NRF24L01 + PA + LNA SMD x1
22. TP4056 x1
23. Crytal 16MHz 3 Pins x1
24. Led 3mm White x2, Red x2
List of accessories used for RX circuit:
1. Ball-bearings 11.5x10x4(thickness)x3(shaft)mm x2
2. Rubber wheels 18x8x2mm x4
3. Aluminum tube 3(Outer diameter)x2mm
4. Steel/zinc bar 1mm
5. Screw M2, M1.2
6. Servo SG90 360 degree x1
7. 1.5g Micro Linear Servo x1

Note for RX Version 1: The PCB that I designed for using boost to 5V, but the current was not enough to operate, so I removed the ic boost and jumped the direct battery to 5V signal.

For RX version 3, you can get all source here: https://www.pcbway.com/project/shareproject/2_4GHz_receiver_circuit_using_ESP32_nRF24L01_with_IC_motor_controller_L298N_dc1f96e2.html
