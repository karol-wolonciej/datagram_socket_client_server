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
#include<time.h>

using namespace std;

string passed_name;

string game_server_host = "localhost", game_server_port = "12345";
string game_ui_host = "localhost", game_ui_port = "12346";



bool name_is_assiociate_with_port(string name)
{
    for(int i = (int)name.length()-1; i >= 0; i--){
        if('0' <= name[i] && name[i] <= '9'){
            continue;
        }
        if(name[i] == ':'){
            return true;
        }
        else {
            return false;
        }
    }
    return false;
}

pair <string,string> break_into_the_address_and_port(string name){
    auto ss = (int)name.length();
    for(int i = (int)name.length()-1; i >= 0; i--){
        if(name[i] == ':'){
            ss = i;
            break;
        }
    }
    pair <string,string> server_port = make_pair(name.substr(0,ss), name.substr(ss+1, name.length()-1));
    if(server_port.first.length() == 0) {
        fprintf(stderr, "server name empty");
    }
    return server_port;
}



void assign_addres_and_port(string& addres, string& port, const string &name_to_break){
    if(name_is_assiociate_with_port(name_to_break)) {
        pair <string,string> addres_port = break_into_the_address_and_port(name_to_break);
        addres = addres_port.first;
        port = addres_port.second;
    }
    else {
        addres = name_to_break;
    }
}


bool assign_program_parameters(int argc, char **argv,
                               string& game_ui_host, string& game_ui_port,
                               string& game_server_host, string& game_server_port){
    if(argc <= 2) {
        cerr<<"not specified server address\n";
        exit(1);
    }

    if(argc == 4) {
        assign_addres_and_port(game_ui_host, game_ui_port, string(argv[3]));
    }
    assign_addres_and_port(game_server_host, game_server_port, string(argv[2]));
    return true;
}


int main (int argc, char *argv[]) {
    assign_program_parameters(argc, argv ,
                              game_ui_host, game_ui_port,
                              game_server_host, game_server_port);



    passed_name = string(argv[1]);

    Client client = Client(passed_name,
                           game_server_host, game_server_port,
                           game_ui_host, game_ui_port); //TO DO popraw parsowanie, aby parsowal player_name



    if(!client.connect_to_gui()) {
        cerr<<"could not connect to gui \n";
        return 1;
    }

    if(!client.connect_to_server()) {
        cerr<<"could not connect to server \n";
        return 1;
    }

    bool t = true;


    while(t) {
        if(client.should_sent_next_datagram()) {
            if(!client.send_datagram_to_server()) return 1;
        }
        else {
            //receive tcp
            if(!client.update_direction()) {
                cerr<<"connection with gui error\n";
                return 1;
            }

            if(!client.receive_datagram_from_server()) {
                cerr<<"connection with server error\n";
                return 1;
            }


        }

    }

    return 0;
}

