About
=====
The idevicelocation tool allows to set the location on iOS Devices.

Requirements
============

Development Packages of:

	libimobiledevice
	libusbmuxd
	libplist
	openssl

Software:

	usbmuxd
	make
	autoheader
	automake
	autoconf
	libtool
	pkg-config
	gcc

Installation
============

To compile run:

	./autogen.sh
	make
	sudo make install

Usage
=====

$idevicelocation [OPTIONS] LATITUDE LONGITUDE
 
Set the location passing two arguments, latitude and longitude.

	$idevicelocation 67.0877 -5.009 

Passing a negative value :

	$idevicelocation 77.432332 -- -7.008373

Options:

	-d enable connection debug messages.
	-u specify device UDID.
	-h help.

