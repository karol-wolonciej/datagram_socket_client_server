#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <regex>
#include <iostream>
#include <stdlib.h>
#include <utility>
#include <string>
#include <arpa/inet.h>
#include <sstream>
#include <list>
#include <fcntl.h>
#include "client.h"
#include "datagram.h"
#include<time.h>
#include "server.h"

using namespace std;

uint32_t parse_optarg_to_int(int option, char* optarg) {
    char * t;
    int64_t check;
    check = (int64_t)strtol(optarg, &t, 10);
    if ((*t) != '\0' || t == optarg) {
        cerr << "passed argument wrong " <<(char)(option) << " " << optarg << endl;
        exit(1);
    }
    if(check < 0) {
        cerr << "passed argument negative value  " <<(char)(option) << " " << optarg << endl;
        exit(1);
    }
    uint32_t v = (uint32_t)check;
    return v;
}



uint32_t parse_optarg_to_int_validation(int option, char* optarg) {
    char * cptr;
    uint32_t v = (uint32_t)strtol(optarg, &cptr, 10);
    if ((*cptr) != '\0' || cptr == optarg) {
        cerr << "passed argument wrong " <<(char)(option) << " " << optarg << endl;
        exit(1);
    }
    return v;
}




int main (int argc, char *argv[]) {
    int c;
    int index;

    uint32_t set_maxx = 800;
    uint32_t set_maxy = 600;
    uint32_t set_port_number = 12345;
    uint32_t set_ROUNDS_PER_SEC = 50;
    uint32_t set_TURNING_SPEED = 6;
    uint64_t set_seed = (uint64_t)time( NULL );

    opterr = 0;

    while ((c = getopt (argc, argv, "r:s:t:W:p:H:")) != -1)
        switch (c)
        {
            case 'W':
                set_maxx = parse_optarg_to_int('W', optarg);
                break;
            case 'H':
                set_maxy = parse_optarg_to_int('H', optarg);
                break;
            case 'p':
                set_port_number = parse_optarg_to_int('p', optarg);
                break;
            case 'r':
                set_seed = (uint64_t)parse_optarg_to_int('r', optarg);
                break;
            case 's':
                set_ROUNDS_PER_SEC = parse_optarg_to_int('s', optarg);
                break;
            case 't':
                set_TURNING_SPEED = parse_optarg_to_int('t', optarg);
                break;

            case '?':
                if (optopt == 'c')
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint (optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf (stderr,
                             "Unknown option character `\\x%x'.\n",
                             optopt);
                return 1;
            default:
                abort ();
        }



    for (index = optind; index < argc; index++) {
        cerr<<"Non-option argument\n";
        return 1;
    }

    SERVER game_server = SERVER(set_maxx, set_maxy, set_port_number,
                                set_ROUNDS_PER_SEC, set_TURNING_SPEED, (uint32_t)set_seed);

    ROUND_TIMER server_round_timer = ROUND_TIMER(set_ROUNDS_PER_SEC);


    game_server.create_socket_and_bin();
    bool runserver = true;


    while(runserver) {

        game_server.disconnect_not_responding_clients();
        if(game_server.currently_game_ongoing()) {
            if(server_round_timer.new_round_start()){
                game_server.execute_next_round();
            }
            game_server.check_if_game_over();
        }
        else{
            if(game_server.should_start_new_game()) {
                game_server.start_new_game();
            }
        }

        game_server.receive_from_clients();
    }


    return 0;
}

