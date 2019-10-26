#include "client.h"
// Main loop to take in input and display output result from server
void Loop(int socket) {
  fd_set clientFds;
  char choice[MAX_BUFFER];
  int show_menu = 1;
  int waiting_for_choice = 1;
  int waiting_for_reply = 0;
  while (1) {
    if (show_menu) {
      printf("\n--------------------------------------------------\n");
      puts("Please select your prefer service:\n  1. Echo\n  2. "
           "Broadcast\n  3. "
           "Quit\nEnter your choice: ");
      show_menu = 0;
    }

    // Reset the connection_file_descriptor_socket set each time since select()
    // modifies it
    FD_ZERO(&clientFds);
    FD_SET(socket, &clientFds);
    FD_SET(0, &clientFds);
    // wait for an available connection_file_descriptor_socket
    if (select(FD_SETSIZE, &clientFds, NULL, NULL, NULL) != -1) {
      for (int connection_file_descriptor_socket = 0;
           connection_file_descriptor_socket < FD_SETSIZE;
           connection_file_descriptor_socket++) {
        if (FD_ISSET(connection_file_descriptor_socket, &clientFds)) {
          if (connection_file_descriptor_socket == socket) {
            if (waiting_for_reply) {
              if (!bcmp(choice, "1", 1)) {
                EchoProtocolHandleServerReply(socket);
                waiting_for_reply = 0;
                waiting_for_choice = 1;
                show_menu = 1;
              } else if (!bcmp(choice, "2", 1)) {
                BroadcastProtocolHandleServerReply(socket);
                waiting_for_reply = 0;
                waiting_for_choice = 1;
                show_menu = 1;
              }
            }
          } else if (connection_file_descriptor_socket == 0) {
            if (waiting_for_choice) {

              if (fgets(choice, 1024, stdin) == NULL) {
                if (errno == EINTR) {
                  perror("fgets error");
                  printf("restart...");
                  continue;
                } else {
                  perror("fgets else error");
                  break;
                }
              }
              if (bcmp(choice, "1", 1) && bcmp(choice, "2", 1) &&
                  bcmp(choice, "3", 1)) {
                printf("Please enter a valid number from 1 to 3\n");
                continue;
              }
              waiting_for_choice = 0;

              // Quit-----------------------------------------------------------------------------------------
              if (!bcmp(choice, "3", 1)) {
                printf("Your choice is to Quit the program\n");
                exit(0);
              }
              // Echo-----------------------------------------------------------------------------------------
              if (!bcmp(choice, "1", 1)) {
                printf("Your choice is Echo Protocol\n");
                EchoProtocolSendRequestToServer(socket);
                waiting_for_reply = 1;
              }
              // Broadcast-----------------------------------------------------------------------------------------
              else if (!bcmp(choice, "2", 1)) {
                printf("Your choice is Broadcast Protocol\n");
                BroadcastProtocolSendRequestToServer(socket);
                waiting_for_reply = 1;
              }
            }
          }
        }
        continue;
      }
    }
  }
}

void establish_connection_with_server(struct sockaddr_in *serverAddr,
                                      struct hostent *host,
                                      int connection_socket, long port) {
  memset(serverAddr, 0, sizeof(serverAddr));
  serverAddr->sin_family = AF_INET;
  serverAddr->sin_addr = *((struct in_addr *)host->h_addr_list[0]);
  serverAddr->sin_port = htons(port);
  if (connect(connection_socket, (struct sockaddr *)serverAddr,
              sizeof(struct sockaddr)) < 0) {
    perror("Couldn't connect to server");
    exit(1);
  }
}

void set_non_blocking(int file_descriptor) {
  int flags = fcntl(file_descriptor, F_GETFL);
  if (flags < 0)
    perror("fcntl failed");

  fcntl(file_descriptor, F_SETFL, flags);
}

void leave_request(int socket) {
  if (write(socket, "/exit\n", MAX_BUFFER - 1) == -1)
    perror("write failed: ");

  close(socket);
  exit(1);
}
