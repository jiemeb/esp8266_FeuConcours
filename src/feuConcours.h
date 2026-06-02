#include <list>
#include <iostream>
using namespace std;

#define port_multicast 5687

#define bitStart 4
#define bitStop 5

class feuConcours
{
private:
	/* data */
//IPAddress broadcast_ip_addr = IPAddress(255, 255, 255, 255); // Broadcast address
IPAddress broadcast_ip_addr=IPAddress(192,168,0,255);



long last_milli;
unsigned int indexPas ;
long Klaxon,tempsSequence,minute ;
boolean start ;
boolean etatKlaxon;
boolean etatStart ;
boolean etatStop ;



int sendToPort =80 ;



struct  sequence { unsigned int klaxon; unsigned int temps;unsigned int feu ; boolean ab;int pausePas;  } ;

vector <sequence> pas ;

void setFeux(int feu) ;
void setKlaxon (boolean stat) ;
void setAB(boolean ab);


public:
	boolean run ;
	String  AdressOfFriend = "";
	String  AdressOfFeux = "";

	feuConcours(/* args */);
	~feuConcours();
	void setup();
    void live();
	void initFile() ;
	void nextSequence() ;
	void sendToFriend(String address,String url);
	String sParam ();
	void udpHandle();
	void udpSendToAll(String);
};