#ifndef _PHONEMGR_GSM_H
#define _PHONEMGR_GSM_H

#include <gsmlib/gsm_me_ta.h>
#include <gsmlib/gsm_event.h>

#include <sigc++/sigc++.h>

#include <time.h>

// local class to handle SMS events

struct IncomingMessage
{
  // used if new message is put into store
  int _index;                   // -1 means message want send directly
  std::string _storeName;
  // used if SMS message was sent directly to TA
  gsmlib::SMSMessageRef _newSMSMessage;
  // used if CB message was sent directly to TA
  gsmlib::CBMessageRef _newCBMessage;
  // used in both cases
  gsmlib::GsmEvent::SMSMessageType _messageType;

  IncomingMessage() : _index(-1) {}
};

struct OutgoingMessage
{
  std::string number;
  std::string message;
};


enum {
  PHONELISTENER_IDLE,
  PHONELISTENER_CONNECTING,
  PHONELISTENER_CONNECTED,
  PHONELISTENER_DISCONNECTING,
  PHONELISTENER_ERROR
};

class EventHandler : public gsmlib::GsmEvent
{
public:
  // inherited from GsmEvent
  void SMSReception(gsmlib::SMSMessageRef newMessage,
                    gsmlib::GsmEvent::SMSMessageType messageType);
  void CBReception(gsmlib::CBMessageRef newMessage);
  void SMSReceptionIndication(std::string storeName, unsigned int index,
                              gsmlib::GsmEvent::SMSMessageType messageType);
  
  virtual ~EventHandler() {}

  // extended
  void setStoreName(std::string rs);
  void setMessageQueue(vector<IncomingMessage> *nm);
protected:
  string receiveStoreName;
  vector<IncomingMessage> *newMessages;
};

class PhoneListener : public SigC::Object
{
 public:
  PhoneListener();

  void sms_loop ();
  void sms_loop_once ();
  void disconnect ();
  bool connect (std::string device);
  void queue_outgoing_message (std::string num, std::string msg);
  void stop ();

  // thread creator
  // Glib::Thread *sms_loop_thread(std::string device, Glib::Dispatcher *disp);

  // signals
  typedef SigC::Signal3<void, std::string, time_t, std::string> notify_sig_t;
  typedef SigC::Signal1<void, int> status_sig_t;
  notify_sig_t signal_notify();
  status_sig_t signal_status();

 protected:
  // methods
  void PhoneListener::send_message_notification(gsmlib::SMSMessageRef msg);
  // member variables
  notify_sig_t m_signal_notify;
  status_sig_t m_signal_status;
  bool terminateSent();
 
 private:
  bool m_terminateSent;
  bool m_erase_mode;
  vector<IncomingMessage> newMessages;
  vector<OutgoingMessage> sendQueue;
  gsmlib::MeTa *mt;

  // methods
  void init();
  void send_message(OutgoingMessage *om);
};
#endif
