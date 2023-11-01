//
// Created by silver on 03/10/23.
//

#include "server/server.h"
#include "client/client.h"

int main(int argc, char** argv) {

    /**
     * Starting the server. Not blocking.
     */
    server_start();

    /**
     * Start the client. Blocking, since the client need the main thread for OpenGL.
     */
    client_start();

    /**
     * Stopping the server after the client closing
     */
    server_stop();
    server_join();

    return 0;
}