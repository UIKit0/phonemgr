// GSMlib stuff

#include "gsm.h"

#include <string>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <signal.h>
#include <fstream>
#include <iostream>

#include <gsmlib/gsm_me_ta.h>
#include <gsmlib/gsm_event.h>
#include <gsmlib/gsm_unix_serial.h>

#include <time.h>
#include <glib.h>


//	add_port() (port, baud, initstring)
//	remove_port()

// PhoneListener class

PhoneListener::PhoneListener()
{
	m_terminateSent=false;
	m_erase_mode=false;
}

void
PhoneListener::stop()
{
	m_terminateSent=true;

}

bool
PhoneListener::terminateSent()
{
	return m_terminateSent;
}

void
PhoneListener::queue_outgoing_message(std::string num, std::string msg)
{
	OutgoingMessage m;
	m.number=num;
	m.message=msg;
	sendQueue.push_back(m);
	g_message ("Queued message to %s", num.c_str());
}

void
PhoneListener::send_message (OutgoingMessage *m)
{
	gsmlib::Ref<gsmlib::SMSSubmitMessage> submitSMS =
	new gsmlib::SMSSubmitMessage();
	gsmlib::Address destAddr(m->number);
	submitSMS->setDestinationAddress(destAddr);
	g_message("Sending message to %s", m->number.c_str());
	mt->sendSMSs(submitSMS, m->message, true);
	g_message("Message sent");
}

PhoneListener::notify_sig_t
PhoneListener::signal_notify()
{
	return m_signal_notify;
}

PhoneListener::status_sig_t
PhoneListener::signal_status()
{
	return m_signal_status;
}

void
PhoneListener::send_message_notification(gsmlib::SMSMessageRef msg)
{
	struct tm stamp;
	time_t timestamp;
	int y;
	gsmlib::Address addr=msg->address();

	stamp.tm_sec=msg->serviceCentreTimestamp()._seconds;
	stamp.tm_min=msg->serviceCentreTimestamp()._minute;
	stamp.tm_hour=msg->serviceCentreTimestamp()._hour;
	stamp.tm_mday=msg->serviceCentreTimestamp()._day;
	stamp.tm_mon=msg->serviceCentreTimestamp()._month -1;
	y=msg->serviceCentreTimestamp()._year;
	stamp.tm_year=y < 80 ? y+100 : y;
	stamp.tm_isdst = -1;
	stamp.tm_yday=0;
	stamp.tm_wday=0;

	timestamp=mktime(&stamp);

	m_signal_notify.emit(addr.toString(), timestamp, msg->userData());
}

bool
PhoneListener::connect(std::string device)
{
    m_signal_status.emit(PHONELISTENER_CONNECTING);

	try {
	// open GSM device
    port = new gsmlib::UnixSerialPort(device,
									gsmlib::DEFAULT_BAUD_RATE,
									gsmlib::DEFAULT_INIT_STRING,
									false);

	// note the MeTa deletes the port in its deconstructor.

	// set 5 second timeout on serial port
	port->setTimeOut(5);
	mt = new gsmlib::MeTa(port);

	std::string dummy1, dummy2, receiveStoreName;
	mt->getSMSStore(dummy1, dummy2, receiveStoreName);
	mt->setMessageService(1);

	// switch on SMS routing
	mt->setSMSRoutingToTA(true, false, false,
						true); // only Receiption indication

	m_signal_status.emit(PHONELISTENER_CONNECTED);

	// register event handler
	EventHandler *e=new EventHandler();
	e->setStoreName(receiveStoreName);
	e->setMessageQueue(&newMessages);
	mt->setEventHandler(e);
	} catch (gsmlib::GsmException &ge) {
        cerr << ("[ERROR]: ") << ge.what() << endl;
        m_signal_status.emit(PHONELISTENER_ERROR);
        // signify exit by error
        return false;
	}
    return true;
}

void
PhoneListener::disconnect ()
{
		try {
			mt->setSMSRoutingToTA(false, false, false);
		} catch (gsmlib::GsmException &ge) {
			// some phones (e.g. Motorola Timeport 260) don't allow to switch
			// off SMS routing which results in an error. Just ignore this.
		}
}

void
PhoneListener::request_disconnect ()
{
    m_signal_status.emit(PHONELISTENER_DISCONNECTING);
    disconnect ();
    // the AT sequences involved in switching of SMS routing
    // may yield more SMS events, so go round the loop one more time
    sms_loop_once ();
    // disconnect properly
    delete mt;
    // delete port;
    // now say we're done -- got to be last
    m_signal_status.emit(PHONELISTENER_IDLE);
}

void
PhoneListener::sms_loop ()
{
	while (1) {
		struct timeval timeoutVal;
		timeoutVal.tv_sec = 1;
		timeoutVal.tv_usec = 0;
		mt->waitEvent(&timeoutVal);

        sms_loop_once ();

		// handle terminate signal
		if (terminateSent ()) {
            request_disconnect ();
            return;
		}
	}
}

void
PhoneListener::polled_loop ()
{
    struct timeval timeoutVal;
    timeoutVal.tv_sec = 0;
    timeoutVal.tv_usec = 0;

    try {
        mt->waitEvent(&timeoutVal);
        sms_loop_once ();
	} catch (gsmlib::GsmException &ge) {
        cerr << ("[ERROR]: ") << ge.what() << endl;
        m_signal_status.emit(PHONELISTENER_ERROR);
        // signify exit by error
	}
}

void
PhoneListener::sms_loop_once ()
{
    try {
		// if it returns, there was an event or a timeout
		while (newMessages.size() > 0) {
			// get first new message and remove it from the vector
			gsmlib::SMSMessageRef newSMSMessage = newMessages.begin()->_newSMSMessage;
			gsmlib::CBMessageRef newCBMessage = newMessages.begin()->_newCBMessage;
			gsmlib::GsmEvent::SMSMessageType messageType =
				newMessages.begin()->_messageType;
			int index = newMessages.begin()->_index;
			std::string storeName = newMessages.begin()->_storeName;
			newMessages.erase(newMessages.begin());

			// process the new message
			std::string result = "Type of message: ";
			switch (messageType)
			{
			case gsmlib::GsmEvent::NormalSMS:
				result += "SMS message\n";
				break;
			default:
				result += "unknown message\n";
				break;
			}

			if (!newSMSMessage.isnull())
			{
				result += newSMSMessage->toString();
				send_message_notification(newSMSMessage);
				g_message("SMS Message");
			}
			else if (! newCBMessage.isnull())
			{
				result += newCBMessage->toString();
				g_message("CBMessage");
			}
			else
			{
				gsmlib::SMSStoreRef store = mt->getSMSStore(storeName);

				if (messageType == gsmlib::GsmEvent::CellBroadcastSMS) {
				g_message("Broadcast message received");
		result += (*store.getptr())[index].cbMessage()->toString();
				}
				else
				{
				g_message("SMS message received");
		result += (*store.getptr())[index].message()->toString();
				send_message_notification((*store.getptr())[index].message());
				}

				if (m_erase_mode) {
				g_message("Erasing message from phone");
				store->erase(store->begin() + index);
				}
			}
			g_message(result.c_str());
		}

		// see if there's anything to send, if so send one

		if (send_waiting ()) {
			send_message (&(*sendQueue.begin()));
			sendQueue.erase (sendQueue.begin());
		}
	} catch (gsmlib::GsmException &ge) {
        cerr << ("[ERROR]: ") << ge.what() << endl;
        m_signal_status.emit(PHONELISTENER_ERROR);
        // signify exit by error
	}

    return;
}

bool
PhoneListener::send_waiting ()
{
    return (sendQueue.size() > 0);
}

void
PhoneListener::init()
{
	// do nothing for now
}
// EventHandler class

void
EventHandler::setStoreName(std::string rs)
{
	receiveStoreName=rs;
}

void
EventHandler::setMessageQueue(vector<IncomingMessage> *nm)
{
	newMessages=nm;
}

void EventHandler::SMSReception(gsmlib::SMSMessageRef newMessage,
				gsmlib::GsmEvent::SMSMessageType messageType)
{
	IncomingMessage m;
	m._messageType = messageType;
	m._newSMSMessage = newMessage;
	newMessages->push_back(m);
}

void EventHandler::CBReception(gsmlib::CBMessageRef newMessage)
{
	IncomingMessage m;
	m._messageType = gsmlib::GsmEvent::CellBroadcastSMS;
	m._newCBMessage = newMessage;
	newMessages->push_back(m);
}

void EventHandler::SMSReceptionIndication(std::string storeName, 
											unsigned int index,
								gsmlib::GsmEvent::SMSMessageType messageType)
{
	IncomingMessage m;
	m._index = index;

	if (receiveStoreName != "" && ( storeName == "MT" || storeName == "mt"))
		m._storeName = receiveStoreName;
	else
		m._storeName = storeName;

	m._messageType = messageType;
	newMessages->push_back(m);
}
