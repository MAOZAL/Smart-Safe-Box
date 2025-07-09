Smart Safe System (ESP32 & Telegram Bot Integration)
This project aims to create a secure smart safe system that can be remotely controlled using an ESP32 microcontroller,
a servo motor, a buzzer, and the Telegram Bot API. Unauthorized access attempts are instantly reported via Telegram,
while password entry and safe status control can also be managed through the RemoteXY mobile application.
________________________________________
Features

•	Telegram Bot Control:

  o	/start: Introduction to the bot and command list.

  o	/status: Query the current status of the safe (open/closed, alarm status).

  o	/alarm_off: Remotely deactivate the active alarm.

  o	/close: Lock the safe via Telegram.

  o	/change_password <new_password>: Change the safe password via Telegram (authorized users only).

  o	Instant notifications for unauthorized access attempts and system status changes.


•	RemoteXY Integration:

  o	Password input through the mobile app.

  o	Manual locking of the safe (via the # command).

  o	Alerts on incorrect password attempts, with alarm activation after a certain number of failures.


•	Auto-Locking:

The safe automatically locks itself after being open for a certain period and sends a notification to users.


•	Persistent Password Storage:

Configured safe passwords are stored in the ESP32’s non-volatile memory (NVS), so they are retained even after a device reboot.


•	Multi-User Support:

Owner and second user roles enable different levels of authorization (such as changing passwords).


•	Alarm System:

An audible alarm (buzzer) is triggered upon incorrect password attempts or unauthorized access.

________________________________________

Hardware Requirements

  •	ESP32 Development Board (e.g., ESP32 DevKitC)

  •	Servo Motor(SG-90) – to control the safe door lock

  •	Buzzer – for alarm sounds

  •	Power Supply – suitable 5V power source for ESP32 and servo

  •	Cables and Breadboard/PCB – for circuit connections

  •	Optional: A physical safe structure (made from cardboard, wood, metal, or 3D printed)

________________________________________

Software Requirements

•	Arduino IDE or PlatformIO

•	ESP32 board definitions must be added to the Arduino IDE

•	Libraries:

  o	UniversalTelegramBot – for interacting with the Telegram API

  o	WiFiClientSecure – for secure Wi-Fi connections

  o	ESP32Servo – to control the servo motor

  o	Preferences – for storing data in ESP32's NVS memory

  o	RemoteXY – for mobile UI integration

________________________________________

Setup and Usage
1. Create a Telegram Bot

    1.Talk to BotFather on Telegram to create a new bot.
	
    2.Save the HTTP API Token provided to you (e.g., 123456:ABC-DEF1234ghIkl-zyx57W2v1u1234).
	
    3.Retrieve the Telegram Chat IDs of the owner and second user:
   
  o	Send any message to your bot

  o	Open the following link in your browser to view chat_id values:

  https://api.telegram.org/bot<YOUR_BOT_TOKEN>/getUpdates

2. Set Up Arduino IDE (or PlatformIO)
   
   1.Install the necessary libraries in the Arduino IDE
(Sketch > Include Library > Manage Libraries):
  o	UniversalTelegramBot.h — by Brian Lough

  o	RemoteXY.h — by Evgeny Shemanuev

  o	ESP32Servo.h — by Kevin Harrington

  o	Preferences.h — this library is included by default in the ESP32 core package; you just need to #include it in your code

  o	WiFi.h — for connecting your ESP board to Wi-Fi networks

  o	WiFiClientSecure.h — built on top of WiFi.h, used for secure (SSL/TLS) connections such as HTTPS

  2.	Open your project file in the IDE.
     
3.Configure the Code

Update the following variables with your own values:

#define REMOTEXY_WIFI_SSID "WIFI_SSID"            // The name of the WiFi network to connect to

#define REMOTEXY_WIFI_PASSWORD "WIFI_PASSWORD"    // The password of the WiFi network to connect to  

#define REMOTEXY_CLOUD_SERVER "CLOUD_SERVER"      // We get the server part from the RemoteXY My Cloud tokens section

#define REMOTEXY_CLOUD_PORT 1234                  // We get the device port part from the RemoteXY My Cloud tokens section

#define REMOTEXY_CLOUD_TOKEN "CLOUD_TOKEN"        //  We get the token part from the RemoteXY My Cloud tokens section 

#define BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"       // Token received from BotFather  



4. Hardware Connections

Make the following pin connections to your ESP32 board:

  •	Buzzer → BUZZER_PIN (e.g., GPIO 2)

  •	Servo signal pin → SERVO_PIN (e.g., GPIO 5)

  •	Proper 5V and GND connections for both ESP32 and the servo motor

________________________________________
5.Uploading and Running the Code

  1.In Arduino IDE, select the correct ESP32 board and COM port

  2.Compile and upload the code to your ESP32
	
  3.Once it starts, open the Serial Monitor to observe startup messages and debug info
   
  4.Install the RemoteXY mobile app and configure the UI to match your project
	
________________________________________
Default Passwords

When the project is first uploaded or if no passwords are stored, the following default passwords are automatically set:

  •	Owner Password: OwnerPass8

  •	Second User Password: UserPass8

You can change these passwords using the /change_password command via Telegram.
________________________________________
Contributing
If you’d like to contribute to the development of this project, pull requests and feedback are welcome!
________________________________________
License
This project is open source and released under the MIT License.

