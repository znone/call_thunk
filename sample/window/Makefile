
all : Window

winexam.res : winexam.rc
	rc $**

Window : Window.cpp ..\..\call_thunk.cpp winexam.res
	cl /MD /O2 $** user32.lib gdi32.lib user32.lib