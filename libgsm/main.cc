#include <stdlib.h>
#include <sigc++/sigc++.h>
#include "gsm.h"
#include <glib.h>
#include <time.h>

static void
on_phone_message (std::string sender, time_t sent,
		std::string msg)
{
	g_message ("Message: %s | %ld | %s", sender.c_str(),
			sent, msg.c_str());
}

static void
on_status_change (int status)
{
	g_message ("New status is %d", status);
}

int
main (int argc, char **argv)
{
	SigC::Connection notifycon, statuscon;

	PhoneListener * listener =  new PhoneListener ();
	notifycon = listener->signal_notify().connect (
		SigC::slot (&on_phone_message));

	statuscon = listener->signal_status().connect (
		SigC::slot (&on_status_change));

	listener->connect ("/dev/rfcomm7");

	//listener->queue_outgoing_message ("1234567",
	//		"Test from libgsm");

	listener->sms_loop ();

	// the loop function automatically disconnects
	
	notifycon.disconnect ();
	statuscon.disconnect ();

	delete listener;

	return 0;
}
