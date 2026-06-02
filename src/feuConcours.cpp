#include <Arduino.h>
#include <string.h>
#include <stdio.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

#include <LittleFS.h>
#include <RemoteDebug.h>
#include <WiFiUdp.h>


#include "getFileLine.h"
#include "feuConcours.h"
#include "automate.h"
#include "Esp8266_FeuConcours.h"
#define DEBUG

extern RemoteDebug Debug;
extern WiFiClient client;
extern HTTPClient http;



WiFiUDP  udpReceive;
WiFiUDP  udpSend;


char incomingPacket[256];

enum FEUX
{
	ROUGE = 1,
	JAUNE,
	VERT
};

feuConcours::feuConcours(/* args */)
{
}

feuConcours::~feuConcours(){};
void feuConcours::setup()
{
	Klaxon = 0;
	tempsSequence = 0;
	setKlaxon(false);
	etatKlaxon = false;
	etatStart=false;
	etatStop=false;	

	minute = 1000;
	start = false ;
	initFile();
	// Start listen UDP client broadcast
	int status = udpReceive.begin(port_multicast);
	Debug.printf("retour BeginBroadcast %d \r\n", status);
	udpSendToAll("whoIsHere?");
}

void feuConcours::live()
{
	udpHandle();

	if (digitalRead(t_in[bitStart]) == LOW) // Button is pressed
	{
		if (etatStart)
		{	
			udpSendToAll("start");
			run = true; // Start sequene telecommande etat ru pour web
			etatStart = false;
			Debug.println("Start button pressed");
		}
	}else
	{
			if (!etatStart)
		{
			etatStart = true;
			Debug.println("Start button released");
		}
	}
	if (digitalRead(t_in[bitStop]) == LOW) // Button stop is pressed
	{
		if (etatStop)
		{
			udpSendToAll("stop");
			etatStop = false;
			Debug.println("Stop button pressed");
		}
	}else
	{
			if (!etatStop)
		{
			etatStop = true;
			Debug.println("Stop button released");
		}
	}
	if (run)
	{ // Run concours
		long delta = millis() - last_milli;
	
		if (delta >= 1000) // Every second when live on
		{
			if(start)
			{
			last_milli = millis() - 1000;
			start = false ;
			}
			else
			{	
				last_milli = millis() - (delta - 1000);
			}

		if (tempsSequence > 0) // We are on step
			{
				tempsSequence--;
				if (Klaxon) // Klaxon each second
				{
					if (etatKlaxon)
					{
						Klaxon--;
						etatKlaxon = false;
						setKlaxon(false);
					}
					else
					{
						setKlaxon(true);
						etatKlaxon = true;
					}
				}
			}
			else
			{
				if (indexPas < pas.size())
				{
					Klaxon = pas[indexPas].klaxon;
					setFeux(pas[indexPas].feu);
					setAB(pas[indexPas].ab);
					tempsSequence = pas[indexPas].temps;
					etatKlaxon = false;
					if (pas[indexPas].pausePas == 2)
						run = false;
					indexPas++;
				}
				else
				{ // We are in the end
					run = false;
					indexPas = 0;
					setKlaxon(false);
				}
			}
		}
	}
	else
	{
//
		start = true ;
		last_milli = millis(); // could be crazy or not
	}
}
void feuConcours::setFeux(int feu)
{
	switch (feu)
	{
	case ROUGE:
		digitalWrite(t_out[0], false);
		digitalWrite(t_out[1], false);
#ifdef DEBUG
		DEBUG_PRINTLN("rouge");
#endif

		break;
	case JAUNE:
		digitalWrite(t_out[0], true);
		digitalWrite(t_out[1], false);
#ifdef DEBUG
		DEBUG_PRINTLN("jaune");
#endif
		break;
	case VERT:
		digitalWrite(t_out[0], false);
		digitalWrite(t_out[1], true);
#ifdef DEBUG
		DEBUG_PRINTLN("vert");
#endif
		break;
	}
}
void feuConcours::setKlaxon(boolean stat)
{
#ifdef FRIEND
	if (run)
		digitalWrite(t_out[3], true);			
	else
		digitalWrite(t_out[3], false);		

#else
	digitalWrite(t_out[3], stat);
#endif
}
void feuConcours::setAB(boolean ab)
{
	digitalWrite(t_out[2], ab);
}

void feuConcours::initFile()
{
	String nameInitFile ;
	short numberRead ;
	char tempo[80];
	getFileLine gline; // must read 4 argument
	gline.setFile("/feuxConcours.ini", 4);
	numberRead = gline.getLine(tempo);	
	nameInitFile = tempo ; 
	Debug.printf("-%s-\n\r",nameInitFile.c_str());
	gline.setFile(nameInitFile, 3);
	
	// int klaxon; int temps;int feux ; boolean ab; pause PAS;
	int a, b, c, d, e;
	pas.clear();
	if (!((gline.getPos()) < 0))
	{
		while ((gline.getLine(tempo)))
		{
			if (5 == sscanf(tempo, "%d %d %d %d %d", &a, &b, &c, &d, &e))
			{
				pas.push_back({(unsigned int)a, (unsigned int)b, (unsigned int)c, (boolean)d, (int)e});
	//			Debug.printf(" conv %s %d %d %d %d %d \n\r",tempo,a,b,c,d,e);
			}
		}
	}

}

String feuConcours::sParam()
{

	String param = "Klaxon ";
	int index = indexPas;
	if (run)
		index--;
	param += Klaxon;
	param += " Temps  ";
	param += tempsSequence;
	param += " Index  ";
	param += indexPas;

	if ((indexPas < pas.size()))
	{
		if (!pas[index].ab)
			param += " CD ";
		else
			param += " AB ";

		param += " feux  ";

		switch (pas[index].feu)
		{
		case ROUGE:
			param += " rouge ";
			break;
		case JAUNE:
			param += " jaune ";
			break;
		case VERT:
			param += " vert ";
			break;
		}
	}
	return param;
}

void feuConcours::nextSequence()
{
	run = false;
	#ifdef DEBUG
	Debug.printf("now %d size pas %d\n\r", indexPas, pas.size());
	#endif
	while (indexPas < pas.size())
	{
#ifdef DEBUG
		Debug.println(pas[indexPas].pausePas);
#endif

		if (pas[indexPas].pausePas > 0)
		{
			break;
		}
		indexPas++;
	}
	tempsSequence = 1;
#ifdef DEBUG
	Debug.printf(" find at %d size pas %d\n\r", indexPas, pas.size());
#endif

	if (indexPas >= pas.size())
		indexPas = 0;
	else if (pas[indexPas].pausePas != 2)
		run = true;
}

void feuConcours::sendToFriend(String address,String url)

{
#ifdef DEBUG
	Debug.print("connecting to ");
	Debug.println(address);
	Debug.print("Requesting URL: ");
	Debug.println(url);
#endif
if(!address.isEmpty())
{

	http.begin(client, address, sendToPort, url);
	int httpCode = http.GET();
	if (httpCode)
	{
		if (httpCode == 200)
		{
			String payload = http.getString();
#ifdef DEBUG
			DEBUG_PRINTLN(" response ");
			DEBUG_PRINTLN(payload);
#endif
		}
		else
		{
#ifdef DEBUG
			DEBUG_PRINT("response ");
			DEBUG_PRINTLN(httpCode);
#endif
		}
	}
#ifdef DEBUG
	Debug.println("closing connection");
#endif
	http.end();
}


}
void feuConcours::udpHandle()
{
int packetSize = udpReceive.parsePacket();
if (packetSize)
{
  Debug.printf("Received %d bytes from %s, port %d\n", packetSize, udpReceive.remoteIP().toString().c_str(), udpReceive.remotePort());
  int len = udpReceive.read(incomingPacket, 255);
  if (len > 0)
  {
    incomingPacket[len] = '\0';
  }
char tempo [16] ;
char  value [40];
Debug.printf("Reception %s \n\r",incomingPacket);
 sscanf(incomingPacket,"%s %s",tempo,value) ;
  String orderR =tempo ;
 // Get Order from receive message
  if(!orderR.compareTo("start") )
  {
	run = true;
	udpSendToAll("getOrder") ;
  }
   if(!orderR.compareTo("stop") )
  {
		nextSequence();
	udpSendToAll("getOrder") ;
  }
  
  if(!orderR.compareTo("whoIsHere?"))
	udpSendToAll(DEVICE_NAME) ;

 if(orderR.compareTo("feuxFriend"))
	{
			AdressOfFriend = udpReceive.remoteIP().toString();
	}
	if(orderR.compareTo("feux"))
	{
			AdressOfFeux = udpReceive.remoteIP().toString();
	}

}
}
void feuConcours::udpSendToAll(String Order){
	// Send broadcast packet
	int status = udpSend.beginPacket(broadcast_ip_addr, port_multicast);

	delay(100);
	if(status == 0)
		Debug.println("Begin erreur");
	if (udpSend.write(Order.c_str(), Order.length()) <= 0)
		Debug.println("erreur send");
	status = udpSend.endPacket();
	if(status == 0)
		Debug.println("EndPacket erreur");

	Debug.println("SendBroadcast");
}