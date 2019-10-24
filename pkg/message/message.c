#include "message.h"
Message *UnmarshallMessage(int message_sender, const char *marshalled_message) {
  const int message_size = ExtractMessageBodySize(marshalled_message);
  const uint16_t message_protocol = ExtractMessageProtocol(marshalled_message);
  const char *message_body = ExtractMessageBody(marshalled_message);
  fprintf(stderr,
          "[DEBUG] UNMARSHALLING MESSAGE - ORIGIN SOCKET [%d] | PROTOCOL "
          "[0x%04hX] = [%C] | BODY SIZE [%d] | BODY [%s] \n",
          message_sender, message_protocol, message_protocol, message_size,
          message_body);
  Message *p = (Message *)malloc(sizeof(Message));
  if (p == NULL) {
    perror("Couldn't allocate anymore memory!");
    exit(EXIT_FAILURE);
  }
  p->body = malloc(sizeof(message_body));
  if (p->body == NULL) {
    perror("Couldn't allocate anymore memory!");
    exit(EXIT_FAILURE);
  }
  p->message_sender = message_sender;
  p->protocol = message_protocol;
  p->size = message_size;
  strcpy(p->body, message_body);
  return p;
}

int SendMessageOverTheWire(int target_socket, const uint16_t protocol,
                           const char *src) {
  unsigned char buffer[MAX_BUFFER];
  int mesg_length = MarshallMessage(buffer, protocol, src);
  return send(target_socket, buffer, mesg_length, 0);
}
int MarshallMessage(unsigned char *dest, const uint16_t protocol,
                    const char *content) {

  int payload_length;
  char *encoded_body = content;
  payload_length = sizeof(encoded_body);
  // Write magic short 2 bytes
  *(uint16_t *)(dest) = htons(0xC0DE);
  fprintf(stderr, "[DEBUG] sizeof magic [%lu]\n", sizeof(htons(0xC0DE)));
  // Write protocol - short 2 bytes
  *(uint16_t *)(dest + 2) = htons(protocol);
  fprintf(stderr, "[DEBUG] sizeof protocol [%lu]\n",
          sizeof(htons(REQ_SEND_MSG)));

  // Write body length - long 4 bytes
  *(uint32_t *)(dest + 4) = htonl(payload_length);
  fprintf(stderr, "[DEBUG] sizeof body length [%lu]\n",
          sizeof(htonl(payload_length)));

  // Write message

  strncpy((char *)(dest + PROTOCOL_HEADER_LEN), encoded_body, payload_length);
  //   return PROTOCOL_HEADER_LEN + payload_length;
  return PROTOCOL_HEADER_LEN + payload_length;
}
// Message to real content
// The return value is the from/to descriptor
const char *ExtractMessageBody(const unsigned char *src) {
  // int ExtractMessageData(unsigned char *dest, const unsigned char *src) {
  char *dest;
  int decoded_body_length = ExtractMessageBodySize(src);
  fprintf(stderr, "[DEBUG] server body size [%d] \n", decoded_body_length);

  dest = malloc(decoded_body_length + 1);
  strncpy((char *)dest, (char *)src + PROTOCOL_HEADER_LEN, decoded_body_length);
  dest[decoded_body_length] = 0;
  return dest;
}
int CalculatePayloadSize(const unsigned char *src) {
  return ntohl(*(uint32_t *)(src + PROTOCOL_HEADER_LEN));
}

int IsValidProtocol(const unsigned char *buf) {
  if (*(uint16_t *)(buf) == htons(0xC0DE))
    return 1;
  return 0;
}
// Get length of the body
int ExtractMessageBodySize(const unsigned char *buf) {
  return ntohl(*(uint32_t *)(buf + 4));
}

// Get message type
uint16_t ExtractMessageProtocol(const unsigned char *buf) {
  return ntohs(*(uint16_t *)(buf + 2));
}