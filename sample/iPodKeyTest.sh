#!/usr/local/bin/newt

constant IPOD_BUTTON_ACTION              := $\r;
constant IPOD_BUTTON_MENU                := $m;
constant IPOD_BUTTON_REWIND              := $w;
constant IPOD_BUTTON_FORWARD             := $f;
constant IPOD_BUTTON_PLAY                := $d;
constant IPOD_SWITCH_HOLD                := $h;
constant IPOD_WHEEL_CLOCKWISE            := $r;
constant IPOD_WHEEL_ANTICLOCKWISE        := $l;
constant IPOD_WHEEL_COUNTERCLOCKWISE     := $l;
constant IPOD_REMOTE_PLAY                := $1;
constant IPOD_REMOTE_VOL_UP              := $2;
constant IPOD_REMOTE_VOL_DOWN            := $3;
constant IPOD_REMOTE_FORWARD             := $4;
constant IPOD_REMOTE_REWIND              := $5;

local	c;

while c := getch() do begin

	if c = $\n or c = IPOD_BUTTON_ACTION then begin
		print("Action\n");
		break;
	end;

	if c = IPOD_BUTTON_MENU then
		print("Menu\n")
	else if c = IPOD_BUTTON_REWIND then
		print("Rewind\n")
	else if c = IPOD_BUTTON_FORWARD then
		print("Forword\n")
	else if c = IPOD_BUTTON_PLAY then
		print("Play\n")
	else if c = IPOD_SWITCH_HOLD then
		print("Hold\n")
	else if c = IPOD_WHEEL_CLOCKWISE then
		print("Clock wise\n")
	else if c = IPOD_WHEEL_ANTICLOCKWISE then
		print("Anti clock wise\n")
	else if c = IPOD_WHEEL_COUNTERCLOCKWISE then
		print("Counter clock wise\n")
	else if c = IPOD_REMOTE_PLAY then
		print("Remote play\n")
	else if c = IPOD_REMOTE_VOL_UP then
		print("Remote vol up\n")
	else if c = IPOD_REMOTE_VOL_DOWN then
		print("Remote vol down\n")
	else if c = IPOD_REMOTE_FORWARD then
		print("Remote forward\n")
	else if c = IPOD_REMOTE_REWIND then
		print("Remote rewind\n")
	else
		print("### UNKNOWN ###\n");
end;
