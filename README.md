## TinyPipes git repo
===

Welcome to the tinyPipes git repo!  This is the home of all the hardware and firmware for tinyPipes electronics.  This should give you a basic idea of where everything lives, and how to use the repo.  Ready?  Onwards!

The repo has a pretty simple structure:

	/
	|
	+Firmware/
	|	|
	|	+bootloader/
	|	|
	|	+libraries/
	|	|
	|	+utilities/
	|	|
	|	+wetware/
	|	|
	|	+Shipped Firmware/
	|
	+Hardware/
		|
		+gerbers/
		|
		+demo/
		|
		+proto/

Firmware/

This is where all firmware lives.  The tinyPipes firmware is called wetware.  It's all based around Arduino C and compilable in arduino 1.0 and later.  

Install everything in libraries/ into the libraries folder in your Arduino sketch folder.  