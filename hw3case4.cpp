#include <iostream>
#include "UdpSocket.h"
#include "Timer.h"

using namespace std;

#define PORT 23460       // my UDP port
#define MAX 20000        // times of message transfer
#define MAXWIN 30        // the maximum window size
#define LOOP 10          // loop in test 4 and 5
const int SIZE_ONE = 1;
const int SIZE_THIRTY = 30;

// client packet sending functions
void clientUnreliable( UdpSocket &sock, const int max, int message[] );
int clientStopWait( UdpSocket &sock, const int max, int message[] );
int clientSlidingWindow( UdpSocket &sock, const int max, int message[], 
        int windowSize );
//int clientSlowAIMD( UdpSocket &sock, const int max, int message[],
//         int windowSize, bool rttOn );

// server packet receiving fucntions
void serverUnreliable( UdpSocket &sock, const int max, int message[] );
void serverReliable( UdpSocket &sock, const int max, int message[] );
void serverEarlyRetrans( UdpSocket &sock, const int max, int message[], 
       int dropRate);
//void serverEarlyRetrans( UdpSocket &sock, const int max, int message[], 
//       int windowSize, bool congestion );

enum myPartType { CLIENT, SERVER, ERROR } myPart;

int main( int argc, char *argv[] ) {

  int message[MSGSIZE/4]; // prepare a 1460-byte message: 1460/4 = 365 ints;
  UdpSocket sock( PORT );  // define a UDP socket

  myPart = ( argc == 1 ) ? SERVER : CLIENT;

  if ( argc != 1 && argc != 2 ) {
    cerr << "usage: " << argv[0] << " [serverIpName]" << endl;
    return -1;
  }

  if ( myPart == CLIENT ) // I am a client and thus set my server address
    if ( sock.setDestAddress( argv[1] ) == false ) {
      cerr << "cannot find the destination IP name: " << argv[1] << endl;
      return -1;
    }

	
	//-------------------------------------------------------------------------
	// Test case 4 runs similar to test case 3, but the "sliding window" 
	// algorithm/implementation only runs for a sliding window of size 1 and 
	// size 30. Add code to the serverEarlyRetrans function in udphw3case4.cpp
	// so that packets are randomly dropped packets N% of the time, where N is
	// every integer from 0 to 10. You will also need to modify the output of
	// hw3case4.cpp so that it outputs the drop percentage (instead of the 
	// window size) into the file. You can simulate a drop by just not 
	// returning an ACK when you receive a packet.
	//-------------------------------------------------------------------------
  int testNumber;
  cerr << "Please pick a sliding window size" << endl;
  cerr << "   1: Packet window size = 1" << endl;
  cerr << "   2: Packet window size = 30" << endl;
  cerr << "--> ";
  cin >> testNumber;

  if ( myPart == CLIENT ) 
  {

    Timer timer;           // define a timer
    int retransmits = 0;   // # retransmissions

    switch(testNumber) 
	{
		case 1:
		for (int n = 0; n <= LOOP; n++) 
		{
			timer.start( );                                         // start timer
			retransmits = clientSlidingWindow(sock, MAX, message, SIZE_ONE);           // actual test
			cerr << "Drop rate: " << n << "%" << endl;
			cerr << "Elasped time = " << timer.lap() << endl;
			//cout << timer.lap( ) << endl;                           // lap timer
			cerr << "retransmits = " << retransmits << endl;
		}
		break;
		case 2:
		for (int n = 0; n <= LOOP; n++) 
		{
			timer.start( );                                         // start timer
			retransmits = clientSlidingWindow(sock, MAX, message, SIZE_THIRTY);          // actual test
			cerr << "Drop rate: " << n << "%" << endl;
			cerr << "Elasped time = " << timer.lap() << endl;
			//cout << timer.lap( ) << endl;                           // lap timer
			cerr << "retransmits = " << retransmits << endl;
		}
		break;
		default:
			cerr << "no such test case" << endl;
		break;
    }
  }
  
 
   /* if ( myPart == SERVER ) {
    switch( testNumber ) {
    case 1:
      serverUnreliable( sock, MAX, message );
      break;
    case 2:
      serverReliable( sock, MAX, message );
      break;
    case 3:
      for ( int windowSize = 1; windowSize <= MAXWIN; windowSize++ )
	serverEarlyRetrans( sock, MAX, message, windowSize );
      break;
    default:
      cerr << "no such test case" << endl;
      break;
    }*/
  
  
  if ( myPart == SERVER ) 
  {
    switch(testNumber) 
	{
		case 1:
			for ( int n = 0; n <= LOOP; n++ )
			{
				serverEarlyRetrans( sock, MAX, message, n );
			}
			break;
		case 2:
			for ( int n = 0; n <= LOOP; n++ )
			{
				serverEarlyRetrans( sock, MAX, message, n );
			}
			break;
		default:
			cerr << "no such test case" << endl;
		break;
    }

    // The server should make sure that the last ack has been delivered to
    // the client. Send it three time in three seconds
    cerr << "server ending..." << endl;
    for ( int i = 0; i < 10; i++ ) {
      sleep( 1 );
      int ack = MAX; 
      sock.ackTo( (char *)&ack, sizeof( ack ) );
    }
  }

  cerr << "finished" << endl;

  return 0;
}

