
#define NOMBRE_MODULE 4


class sequenceur 
{
public:
/* struct order {
              unsigned char instruction ;
              unsigned char data ;
	      }; */
#define SIZE_INSTRUCTION 2
struct mod { 
unsigned short pas ;	// Index programme
unsigned char pas_courant ;
         char aad;	// Arithmetiqye Accu data Cha for this moment
unsigned char lcr;	// Logic
unsigned char acr;
unsigned short time ;
} module[NOMBRE_MODULE];

  void setup();
  void live( );
 
  	
};
