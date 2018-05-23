//-----------------------------------------------------------------------------
// AW Homework 3
//-----------------------------------------------------------------------------

#include "UdpSocket.h"
#include "Timer.h"
#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <sstream>
#include <vector>
#include <strings.h>

using namespace std;

const int BYTE_MESSAGE_SIZE = 1460; //allocates a 1460-byte message[]
const int TIMEOUT = 1500;


#define PORT 73519       // my UDP port
#define MAX 20000        // times of message transfer
#define MAXWIN 30        // the maximum window size
#define LOOP 10          // loop in test 4 and 5


//-----------------------------------------------------------------------------
// Case 2: int clientStopWait( UdpSocket &sock, const int max, int message[] ):
// sends message[] and receives an acknowledgment from the server max (=20,000)
// times using the sock object. If the client cannot receive an acknowledgment
// immediately, it should start a Timer. If a timeout occurs (i.e., no response
// after 1500 usec), the client must resend the same message. The function must
// count the number of messages retransmitted and return it to the main 
// function as its return value.
//-----------------------------------------------------------------------------
int clientStopWait(UdpSocket &sock, const int max, int message[])
{
	cerr << "Testing Client Stop Wait" << endl;
	int timeout = 1500;
	int retransmit_count = 0; // for return value
	Timer ack_timer;		  // to start acknowledgement timer
	
	// While there are frames to send
	for(int i = 0; i < max; i++)
	{
		// try to send the message and start it's timer. 
		message[0] = i;
		sock.sendTo((char*)message, BYTE_MESSAGE_SIZE);  
		cerr << "message " << message[0] << endl;
		ack_timer.start();
	
	
		// The method returns a positive number if there is data to read; 0 or negative otherwise
		while(sock.pollRecvFrom() == 0 || sock.pollRecvFrom() < 0)
		{
			// If a timeout occurs (i.e., no response after 1500 usec),
			if(ack_timer.lap() > TIMEOUT)
			{
				// the client must resend the same message
				sock.sendTo((char*)message, BYTE_MESSAGE_SIZE);
				retransmit_count += 1; // increment the retransmission count
				ack_timer.start(); // restart the timer
			}
		}
		sock.recvFrom((char *)message, BYTE_MESSAGE_SIZE);
	}
	// The function must count the number of messages retransmitted and return
	// it to the main function as its return value.
	return retransmit_count;
}

//-----------------------------------------------------------------------------
// Case 2: void serverReliable( UdpSocket &sock, const int max, int message[] ):
// repeats receiving message[] and sending an acknowledgment at a server side
// max (=20,000) times using the sock object.
//-----------------------------------------------------------------------------
void serverReliable(UdpSocket &sock, const int max, int message[])
{
	cerr << "Testing Server Reliable" << endl;
	bool in_order = true; // keep track of message order
	for(int i = 0; i < max; i++)
	{
		// While receiving messages in order
		do
		{
			// Receive data into msg[] of size length. This method can be used
			// both to receive a message from a client as well as an acknowledgment
			// from a server.
			sock.recvFrom((char*)message, BYTE_MESSAGE_SIZE);
			// If message received is in order
			if(message[0] == i)
			{
				// As an example, if you wish to receive a single char variable,
				// say c, your call should be recvFrom( &c, sizeof( c ) ).
				sock.ackTo((char*) &i, sizeof(int));
				cerr << message[0] << endl; 
			}
			// If message received is out of order
			else if(message[0] != i)
			{
				in_order = false;
			}
		} while(in_order);
	}
}

//-----------------------------------------------------------------------------
// Case 3: int clientSlidingWindow( UdpSocket &sock, const int max, 
// int message[], int windowSize ): sends message[] and receiving an
// acknowledgment from a server max (=20,000) times using the sock object. As 
// described above, the client can continuously send a new message[] and 
// increasing the sequence number as long as the number of in-transit messages
// (i.e., # of unacknowledged messages) is less than "windowSize." That number 
// should be decremented every time the client receives an acknowledgment. If 
// the number of unacknowledged messages reaches "windowSize," the client should
// start a Timer. If a timeout occurs (i.e., no response after 1500 usec), it 
// must resend the message with the minimum sequence number among those which 
// have not yet been acknowledged. The function must count the number of messages
// (not bytes) re-transmitted and return it to the main function as its return value.
//-----------------------------------------------------------------------------
int clientSlidingWindow( UdpSocket &sock, const int max, int message[], int windowSize )
{
	cerr << "Testing Client Sliding Window" << endl;
	int retransmit_count = 0;
	int next_frame = 0;
	int messages = 0;
	int ack_buf[1];
	int sequence = 0;
	Timer unack_thresh;

	// While there are frames to send
	while (sequence < max) 
	{
		if (sequence < next_frame)
		{
			sequence += 1;
			messages -= 1;
			continue;
		}
		// Loop until all frames are sent and acknowledgements received.
		while(messages < windowSize && (sequence + messages) < max)
		{
			message[0] = (sequence + messages);             
			sock.sendTo((char*)message, MSGSIZE);
			cerr << "message: " << message[0] << endl;
			messages += 1;
		}

		// Check to see if the socket has any data to read
		if(sock.pollRecvFrom() > 0)
		{
			sock.recvFrom((char*)ack_buf, sizeof(int));
			next_frame = ack_buf[0];
		}
		// If there is no data to read
		else
		{
			// Start timer 
			unack_thresh.start();
			while(sock.pollRecvFrom() == 0)
			{
				// If outside timeout window
				if(unack_thresh.lap() > TIMEOUT)
				{
					// Increment the retransmisison count and resend
					retransmit_count += 1;
					message[0] = sequence;
					sock.sendTo((char*)message, MSGSIZE);
					unack_thresh.start();
				}
			}
		}
	}
  return retransmit_count;
}

//-----------------------------------------------------------------------------
// void serverEarlyRetrans( UdpSocket &sock, const int max, int message[], 
// int windowSize ): receives message[] and sends an acknowledgment to the 
// client max (=20,000) times using the sock object. Every time the server 
// receives a new message[], it must save the message's sequence number in an 
// array and return a cumulative acknowledgment, i.e., the last received 
// message in order.
//-----------------------------------------------------------------------------
void serverEarlyRetrans(UdpSocket &sock, const int max, int message[], int windowSize)
{
	cerr << "Testing Server Early Retransmit" << endl;
	vector<bool> array(max, false);
	int ack_buf[1];
	int sequence = 0;

	// Receive from client
	while (sequence < max) 
	{
		// Recieve a message, advance to receive next message
		sock.recvFrom((char*)message, MSGSIZE);
		array[message[0]] = true;
		while(array[sequence] == true)
		{
			sequence += 1;
		}
		// Send acknowledgement
		ack_buf[0] = sequence;
		sock.ackTo((char*)ack_buf, sizeof(int));
		cerr << message[0] << endl;
	}
}

