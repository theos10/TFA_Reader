/*
Ce programme récupere les informations du signal radio d'une sonde TFA 30.3120.90 recu par le raspberry PI
Vous pouvez compiler cette source via la commande :
        g++ TFA_Reader.cpp -o TFA_Reader -lwiringPi
        N'oubliez pas d'installer auparavant la librairie wiring pi ainsi que l'essentiel des paquets pour compiler
Vous pouvez lancer le programme via la commande :
        sudo chmod 777 TFA_Reader
        ./TFA_Reader p 2
        Le parametres de fin étant le numéro wiringPi du PIN relié au récepteur RF 433 mhz
@author : Valentin CARRUESCO (idleman@idleman.fr)
@contributors : Yann PONSARD, Jimmy LALANDE
@Adaptation : Ghislain FOURNIER, pour rasperryPI.
@webPage : http://blog.idleman.fr
@references & Libraries: https://projects.drogon.net/raspberry-pi/wiringpi/, http://playground.arduino.cc/Code/HomeEasy
@Aide C++ : http://carl.seleborg.free.fr/cpp/cours/annexes/operateurs.html
*/

#include <wiringPi.h>
#include <iostream>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <sched.h>
#include <sstream>
#include <fstream>
#include <string>

using namespace std;
using std::string ;

//initialisation du pin de reception
int pin;
int	verbose = 0 ;
int ar[44]; // Declaration tableau de données de 44bits et variable de position // int ar[44];
int pos=0;
int b, d, td, td1, td2;
int id=0;
int ida=0;
int idb=0;
int idx=0;
int donne=0;
int nbit=0;
int nposi=0;
int i=0;
int crc=0;
char data1;
char data2;
char data3;
char data4;
char data5;
int st1=0;
int st2=0;
int typ=0;
int result=0;
unsigned long t = 0;
char donhex;
string nomFichier("fic");
string chemin("");

// Fonction de log
void log(string a){
        cout << a << endl;
}

// Fonction de passage du programme en temps réel (car la reception se joue a la micro seconde près)
void scheduler_realtime() {
        struct sched_param p;
        p.__sched_priority = sched_get_priority_max(SCHED_RR);
        if( sched_setscheduler( 0, SCHED_RR, &p ) == -1 ) {
        perror("Failed to switch to realtime scheduler.");
        }
}

//Fonction de remise du programme en temps standard
void scheduler_standard() {
        struct sched_param p;
        p.__sched_priority = 0;
        if( sched_setscheduler( 0, SCHED_OTHER, &p ) == -1 ) {
        perror("Failed to switch to normal scheduler.");
        }
}

//Recuperation du temp (en micro secondes) d'une pulsation
int pulseIn(int pin, int level, int timeout)
{
   struct timeval tn, t0, t1;
   long micros;
   gettimeofday(&t0, NULL);
   micros = 0;
   while (digitalRead(pin) != level)
   {
      gettimeofday(&tn, NULL);
      if (tn.tv_sec > t0.tv_sec) micros = 1000000L; else micros = 0;
      micros += (tn.tv_usec - t0.tv_usec);
      if (micros > timeout) return 0;
   }
   gettimeofday(&t1, NULL);
   while (digitalRead(pin) == level)
   {
      gettimeofday(&tn, NULL);
      if (tn.tv_sec > t0.tv_sec) micros = 1000000L; else micros = 0;
      micros = micros + (tn.tv_usec - t0.tv_usec);
      if (micros > timeout) return 0;
   }
   if (tn.tv_sec > t1.tv_sec) micros = 1000000L; else micros = 0;
   micros = micros + (tn.tv_usec - t1.tv_usec);
   return micros;
}

// Fonction decodage des blocs binaire
void decode (int ar[], int nposi, int nbit)
{
	nbit--;
	d=0;
	for (int i=0; i <= nbit; i++)
    {
        d=d << 1;
		if (ar[nposi+i]==1) b=0;
		else {
			b=1;
			d=d+b;
			}
    }
    donne=d; // data

	if (verbose) {
            std::stringstream ss;
            ss<< std::hex << d; // int decimal_value
            std::string donhex ( ss.str() );
            std::cout<<" la valeur en decimale :" << d << " et en hexadecimal :" << donhex << endl;
    }
}

// Fonction affichage de l'usage
void usage (void) {
	printf("\nTFA_Reader   Usage:    TFA_Reader [-h] [-p PIN] [-c chemin] [-v]\n");
	printf("	Pour plus de details utiliser l'aide: TFA_Reader -h\n\n");
	printf("	Version 1.2\n\n");
}

// Fonction affichage de l'aide
void help (void) {
	printf("\nTFA_Reader lit les sondes de température Lacrosse TFA 433mhz (FOURNIER Ghislain - 2015 \n\n");
	printf("Usage:   TFA_Reader [-h] [-p PIN] [-v]\n\n");
	printf("        -h      : Aide\n");
	printf("        -p PIN  : Pin du raspberry PI q'il faut lire\n");
	printf("        -c chemin  : Chemin d'enregistrement du fichier\n");
	printf("        -v      : Mode Verbeux\n");
	printf("  ** Commande examples **\n");
	printf("     TFA_Reader -p 2  \n\n");
	printf("     Version 1.2\n\n");
}

// Fonction analyse de la ligne de commande
char parse_cmdline(int argc,char *argv[]) {
   int helpflg=0;
   int errflg=0;
   int c;
while ((c = getopt(argc, argv, "p:c:vh")) != EOF)
   {
   switch (c)
      {
 		 case 'p':
            	pin = atoi(optarg);
				            	break;
		 case 'c':
				chemin = optarg;
				break;
		 case 'v':
            	 verbose = 1;
            	 break;
        case 'h':
            	helpflg++;
         	break;
      }
        if (helpflg)	/* Aide */
	{
           help();
           exit (1);
        }
   }
   }

// ***  Programme principal  ***
int main (int argc, char** argv)
{
//on récupere l'argument 1, qui est le numéro de Pin GPIO auquel est connecté le recepteur radio
// on traite les parametres ( arg = nombre entre 1 et 13, ou --help, et -c chemin des fichier et -v verbeux )
parse_cmdline(argc,argv) ;

	if (verbose) { log(" Start..."); }
    if (verbose) { log(" **** M o d e   V e r b e u x **** "); }

//On passe en temps réel
scheduler_realtime();

//Si on ne trouve pas la librairie wiringPI, on arrête l'execution

    if(wiringPiSetup() == -1)
    {
		if (verbose) { log("Librairie Wiring PI introuvable, veuillez lier cette librairie..."); }
        return -1;
    }else{
		if (verbose) { log("Librairie WiringPI detectée");}
    }
    pinMode(pin, INPUT);

	if (verbose) { printf ("Pin GPIO %d configuré en entrée\n" , pin ); }

	if (verbose) { log("Attente d'un signal du transmetteur ...\n"); }

//On boucle pour ecouter les signaux
	b: pos=0;

// Ecoute de la sequence d'initialisation + le type de mesure.
// Soit 8bit 0 A ( 1111 0101 ) + 1111  ( 0=Thermo - E=Hydro )
t = pulseIn(pin, HIGH, 1000000);
t = pulseIn(pin, HIGH, 1000000); if ((t>1100)&&(t<1500)) {ar[pos] = 1; pos++;
t = pulseIn(pin, HIGH, 1000000); if ((t>1100)&&(t<1500)) {ar[pos] = 1; pos++;
t = pulseIn(pin, HIGH, 1000000); if ((t>1100)&&(t<1500)) {ar[pos] = 1; pos++;
t = pulseIn(pin, HIGH, 1000000); if ((t>1100)&&(t<1500)) {ar[pos] = 1; pos++;

t = pulseIn(pin, HIGH, 1000000); if ((t>400)&&(t<700)) {ar[pos] = 0; pos++;
t = pulseIn(pin, HIGH, 1000000); if ((t>1100)&&(t<1500)) {ar[pos] = 1; pos++;
t = pulseIn(pin, HIGH, 1000000); if ((t>400)&&(t<700)) {ar[pos] = 0; pos++;
t = pulseIn(pin, HIGH, 1000000); if ((t>1100)&&(t<1500)) {ar[pos] = 1; pos++;

t = pulseIn(pin, HIGH, 1000000); if ((t>1100)&&(t<1500)) {ar[pos] = 1; pos++;
t = pulseIn(pin, HIGH, 1000000); if ((t>1100)&&(t<1500)) {ar[pos] = 1; pos++;
t = pulseIn(pin, HIGH, 1000000); if ((t>1100)&&(t<1500)) {ar[pos] = 1; pos++;
t = pulseIn(pin, HIGH, 1000000); if ((t>1100)&&(t<1500)) {ar[pos] = 1; pos++;

}else goto b;
}else goto b;
}else goto b;
}else goto b;
}else goto b;
}else goto b;
}else goto b;
}else goto b;
}else goto b;
}else goto b;
}else goto b;
}else goto b;

// Ecoute des 32 bits restant
	if (verbose) { printf("Récéption Data : \n"); }
	for (int i=1; i <= 32; i++){
		t = pulseIn(pin, HIGH, 1000000);
		if ((t>400)&&(t<700)){ar[pos] = 0; pos++;}
		if ((t>1100)&&(t<1500)){ar[pos] = 1; pos++;}
	}

// START
    decode (ar, 0, 4);
    st1=donne;
    decode (ar, 4, 4);
    st2=donne;

// TYPE
    decode (ar, 8, 4);
    typ=donne;

// ID 1 : traitement du premier quartet, IDa npos=12 nbit=4
	decode (ar, 12, 4);
	ida=donne; // IDa

// ID 2 : traitement du deuxieme quartet, idb npos=16 nbit=3
	decode (ar, 16, 3);
	idx=donne; // ones of degrees
	id=(ida*10)+idx;
	decode (ar, 16, 4);
	idb=donne; // IDb
	if (verbose) { printf("ID Capteur : %d \n",id) ; }

// DATA 1 : traitement du premier quartet, les dizaines ex: 0111 -> 1 npos=20 nbit=4
	decode (ar, 20, 4);
	d=donne-5; // on sustrait 50 a la valeur. donc pourquoi 5 ??
	data1=donne;
	td1=d*100; // tens of degrees.

// DATA 2 : traitement du deuxieme quartet, les unités npos=24 nbit=4
	decode (ar, 24, 4);
	data2=donne;
	td2=td1+10*donne; // ones of degrees

// DATA 3 : traitement du troisiemes quartet, les decimales npos=28 nbit=4
	decode (ar, 28, 4);
	td2=td2+donne;  // decimal parts of degrees
	data3=donne;
	td=td2/10;

// DATA 4 : traitement du troisiemes quartet, les decimales npos=28 nbit=4
	decode (ar, 32, 4);
	data4=donne;

// DATA 5 : traitement du troisiemes quartet, les decimales npos=28 nbit=4
	decode (ar, 36, 4);
    data5=donne;

// CRC : traitement du dernier quartet, les decimales npos=40 nbit=4
	decode (ar, 40, 4);
	crc=donne;

// CALCUL CRC
// Checksum: (0 + A + 0 + 0 + E + 7 + 3 + 1 + 7 + 3) and F = D
	result=(st1+st2+typ+ida+idb+data1+data2+data3+data4+data5);
// and 0xFF
	result = (char) (result&0xF);

    if (crc==result){  // revoir le controle de CRC
        if (verbose) {cout << " CRC OK = " << result << endl;}
        cout << td << "." << d << endl;

// Ecriture dans le fichier
        std::stringstream sstm;
        sstm << chemin << "sonde." << id;
        nomFichier = sstm.str();

        ofstream IDFlux(nomFichier.c_str());
        if (verbose) {cout << " ouverture du fichier : " << nomFichier << endl;}
            if(IDFlux)
            {
                if (verbose) {cout << " Ecrit : " << td << "." << d << " dans "<< nomFichier << endl;}
                IDFlux << td << "." << d << endl;
            }
        else
            {
                cout << "ERREUR: Sur le fichier." << endl;
            }
    }
    else {
        if (verbose) {cout << " BAD CRC " << endl;}
    }

	if (verbose) {
		for (int i=0; i <= 43; i++)
		{
			cout << ar[i];
		}
		cout << endl;
	}
scheduler_standard();

}

/* Informaton compementaire :
http://www.f6fbb.org/domo/sensors/tx3_th.php
http://www.jacquet80.eu/blog/post/2011/10/Decodage-capteur-thermo-hygro-TFA
http://fredboboss.free.fr/?p=752
http://www.instructables.com/files/orig/FAY/0LCL/H2WERYWD/FAY0LCLH2WERYWD.pdf
http://www.bitformation.com/art/weather_logger.html
http://lucsmall.com/2012/04/29/weather-station-hacking-part-2/
http://makin-things.com/articles/decoding-lacrosse-weather-sensor-rf-transmissions/
Funksensor TFA, 433 MHz Temperatur-Außensender TFA 30.3120.90 not compatible with receiver 30.3034.01
 11 Quartet
 Ex
1111 0101 | 1111 | 1100 110 0 | 1000 | 0111 | 0111 | 1000 | 0111 | 1001         -- > Données capturer
0000 1010 | 0000 | 1001 010 1 | 0111 | 1000 | 0100 | 0111 | 1000 | 1010			-- > Données utilisé
 0    A   |   0  |  9    2    |  7   |  8   |  4   |  7   |  8   |  A			-- > valeur en hex, utilisé en dec, soustraire 50
 0    10  |   0  |    149     |  7   |  8   |  4   |  7   |  8   |  10 			-- > end 28.8
  start   | type |  id        |  dix |  uni | dec  | dix  |  uni | crc
  Start   | Type |  ID     |  |              D A T A             | CrC
*/
// Fin
