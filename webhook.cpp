#include <iostream>
using namespace std;

class BENotificationService 
{
 private:
    string notification;
    string sender;
    string receiver;

 public:
    BENotificationService(string notification, string sender, string receiver){
        this->notification = notification;
        this->sender = sender;
        this->receiver = receiver;
    }
    void sendAcknowledgement(){
        cout<<"Message delivered to : " << receiver << endl;
    }
    void displayNotification(){
        cout<<"Notification received: " << notification << endl;
    }

    ~BENotificationService();
};

class DisplayNotification
{
 public:
    DisplayNotification(BENotificationService *aNotifService){
        aNotifService->displayNotification();
    }
};

class SendAcknowledgement
{
 public:
    SendAcknowledgement(BENotificationService *aNotifService){
        aNotifService->sendAcknowledgement();
    }
};

class PushNotification
{
 public:
    PushNotification(string iPayloadData, string sender, string receiver) {
        BENotificationService *aNotificationService = new BENotificationService(iPayloadData, sender, receiver);
        // Use Notification service to send acknowledgement to the sender
        SendAcknowledgement *aAcknowledgement = new SendAcknowledgement(aNotificationService);
        // Use Notification service to display the notification received by the receiver
        DisplayNotification *aDisplay = new DisplayNotification(aNotificationService);
    }
};

class TextNotification{

};

class EmailNotification{

};

int main(){
    
    PushNotification *aPushNotifService = new PushNotification("Hello", "PSP", "Frontend");

    return 0;
}

