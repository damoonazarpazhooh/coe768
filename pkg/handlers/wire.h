#ifndef WIRE
#define WIRE
#include "../shared/consts.h"
#include <stdio.h>

// for uint type
#include <stdint.h>
// for htons and other methods of the same family
#include <arpa/inet.h>
// For string helper methods
#include "../message/message.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// checks to see if the message is based on a valid protocol
// in which we have defined handlers
int IsValidProtocol(const unsigned char *buf);
// Encode
// SendMessageOverTheWire - sends the message to a target after marshalling
int SendMessageOverTheWire(int target_socket, const uint16_t protocol,
                           const char *src);

#endif