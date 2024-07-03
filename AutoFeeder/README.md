The Cat Smart Auto Feeder with remote control, scheduling, and feeding history is built upon an Arduino Uno R3 base, incorporating an LCD display (16x2), an RTC DS3231 for real-time clock functionalities, and a Servo MG996R motor. This system allows users to remotely control feeding times, set schedules for automatic feeding, and keeps a log of feeding history for monitoring purposes.

Watch on youtube: https://youtu.be/s_e-0ddQXQ8

1.	Some technical stuff:
	1.	Based on Arduino Uno R3 															(300UAH - 7.5$)
	2.	Real Time Clock Module DS3231 														(120UAH – 3$)
	3.	LCD display 16x2 																	(160UAH – 4$)
	4.	Servo motor: MG996R 																(190UAH – 4.75$)
	5.	Kitchen grain dispenser 															(780UAH – 19.5$)
	6.	Power Supplier based on Step-down DC/DC converter only for Servo motors (max 3A) 	(40UAH – 1$)
	7.	Mini DC/DC linear converter for Arduino LCD and RTC module							(12UAH – 0.3$)
	8.	DHT Module for Temperature & Humidity (will be stored at Feed History each time run)(55uah – 1.3$)	
	9.	Relay module with Dry contact from Sonoff eWelink ecosystem (you can use any of Smart House ecosystem; the only requirement is that it should be Dry Contact Relay module) (500UAH – 12.5$)
	10.	Total price: 53.85$ excludes my professional C++ programming skills and 3 weeks for dev & testing.
	
2.	Buttons, Menu & Settings
	1.	4 buttons: Ok, Up, Down and return back, one big button for manual feeding.
	2.	Long press on the Ok button: will show you the current temperature and humidity.
	3.	Second long press: History menu, use up and down to scroll 25 last fed.
	4.	3rd long press: Scheduler presets: 2Day means 2 times in between 7AM and 11PM. 3Day – means 3 times feeding within the same time range. DayNight means from 12AM to 23PM time range. And so on (scrolling other presets)
	5.	4th long press will bring you to Start Angle menu set. Value ranges from 0 to 90 degrees means Start Angle for servo motors to set portion size.
	6.	And the last one is rotate count, which means how many times servos will rotate from StartAngle (from previous menu) to 180 degrees maximum.
