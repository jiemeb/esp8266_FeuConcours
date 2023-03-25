#include <list>
#include <iostream>
using namespace std;

#define port_multicast 9999


class feuConcours
{
private:
	/* data */
IPAddress multicast_ip_addr=IPAddress(224, 0, 1, 3);
String  AdressOfFriend = "";
long last_milli;
unsigned int indexPas ;
long Klaxon,tempsSequence,minute ;
boolean start ;
boolean etatKlaxon;


int sendToPort =80 ;



struct  sequence { unsigned int klaxon; unsigned int temps;unsigned int feu ; boolean ab;int pausePas;  } ;

vector <sequence> pas ;

void setFeux(int feu) ;
void setKlaxon (boolean stat) ;
void setAB(boolean ab);

public:
	boolean run ;

	feuConcours(/* args */);
	~feuConcours();
	void setup();
    void live();
	void initFile() ;
	void nextSequence() ;
	void sendToFriend(String url);
	String sParam ();
	void udpHandle();
	void udpSendToAll(String);
};