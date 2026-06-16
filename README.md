# ABOUT WAVESYNCH



WaveSynch is a gesture-control system that lets you control your computer's media and volume using hand gestures. It uses an Arduino UNO with two ultrasonic sensors,HC-SR04,to detect how far away your hands are, and a Python script to trigger actual keyboard shortcuts on your PC.





##### HOW DOES IT WORK?

The system divides the distance in front of the sensors into two active zones:

A) CLOSE ZONE

B) FAR ZONE



The LEFT Sensor controls the PC Volume

The RIGHT Sensor controls Fast Forward and Rewind 

Both sensors simultaneously control Play or Pause 



**1)Initiating the Pulse**

The entire measurement cycle is kicked off by the Arduino firmware rather than the sensor itself. To get a clean starting point, the Arduino sets the sensor's Trigger pin to a low voltage state for just a couple of microseconds. Once stable, the code pulls the Trigger pin high for exactly 10 microseconds before dropping it back to low. This precise, ultra-short electrical pulse acts as a start signal that wakes up the internal circuitry of the HC-SR04 sensor.



**2)Emitting the Ultrasonic Wave**

The instant the sensor detects that 10-microsecond trigger pulse, its transmitter transducer fires an ultrasonic sound burst into the air. This burst consists of exactly eight cycles of sound waves vibrating at 40 kHz, which is a frequency far too high for human ears to hear. Sending exactly eight cycles gives the acoustic wave enough energy to travel forward through the room while remaining distinct enough for the sensor to recognize when it bounces back.



**3)The Stopwatch**

At the exact microsecond the sound waves leave the transmitter, the sensor’s internal logic pulls its Echo pin up to a high state of 5V. It is important to note that the Echo pin doesn't wait for the sound to return to go high; it acts like a stopwatch that starts running the moment the sound wave is born. The Echo pin stays locked in this high voltage state while the sound wave travels through the air, waiting for something to happen.



**4)Echo Reflection and Calculations**

The sound wave travels forward at roughly 343 meters per second until it hits an obstacle, like your hand. When it strikes your skin, the sound energy bounces backward toward the sensor. The moment the sensor's receiver transducer detects this returning 40 kHz echo, it instantly drops the Echo pin back down to a low state of 0V. The Arduino, which has been monitoring this pin using its internal timers, counts how many microseconds the pin stayed high. Because the sound had to travel to your hand and back, the code takes that total round-trip time, multiplies it by the speed of sound, and divides it by two to calculate the final, accurate distance in centimeters.



##### Hardware Needed

1 x Arduino (Uno, Nano, or Mega)

2 x HC-SR04 Ultrasonic Sensors (Left and Right)

A breadboard and jumper wires



##### Setup Instructions



1\. Hardware Setup

Before flashing the code, wire up your two ultrasonic sensors to the Arduino. Both sensors can share the Arduino’s 5V and GND pins using the power rails on a breadboard. The data pins must be connected exactly as follows to keep the left and right channels separate:



Left Sensor: Connect the Trig pin to Digital Pin 2, and the Echo pin to Digital Pin 3.

Right Sensor: Connect the Trig pin to Digital Pin 4, and the Echo pin to Digital Pin 5.



2\. Arduino Setup

Open gesture\_controller in Arduino IDE.

Select your board type and COM port, then click Upload.



3\. Python Setup

Open your terminal inside the project folder and install the two required libraries - pyautogui \& pyserial



4\. Running the Project

Make sure your Arduino is plugged in via USB, check what COM port it is using, and run the Python script.



##### Code Logic \& Features

To stop the sensors from acting glitchy or triggering random keys, the Arduino code handles a few important things:



**Median Filtering**: The code takes 5 quick readings in a row and sorts them to find the middle value. This removes any random spikes.

**Debouncing**: Your hand has to stay in a zone for a split second before the command registers. This stops the logic from considering random movements as defined gesture.

**Cooldowns**: After a Play/Pause is triggered with both hands, the single sensor controls are temporarily locked so that pulling your hands away doesn't accidentally trigger a volume or seek command.

