#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string>
#include <list>
#include "datagram.h"
#include <vector>
#include <sys/time.h>
#include "server.h"
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>

#define MYPORT "4950"


using namespace std;


bool compare_nocase (const CONNECTED_CLIENT& first, const CONNECTED_CLIENT& second)
{
    unsigned int i=0;
    while ( (i<first.name.length()) && (i<second.name.length()) )
    {
        if (tolower(first.name[i])<tolower(second.name[i])) return true;
        else if (tolower(first.name[i])>tolower(second.name[i])) return false;
        ++i;
    }
    return ( first.name.length() < second.name.length() );
}





//majac struct sockaddr their_addr;
//przekaz (struct sockaddr *)&their_addr



CONNECTED_CLIENT::CONNECTED_CLIENT(string _name, struct sockaddr _adr, uint64_t _session_id, int8_t _turn_direction) {
    name = _name;
    adr = _adr;
    next_expected_event_no = 0;
    session_id = _session_id;
    connected = true;
    set_turn_direction(_turn_direction);
    if(gettimeofday(&tval, NULL) == 0) {
        last_notification = (uint64_t)tval.tv_usec + (uint64_t)tval.tv_sec*usecond;
    }
}


SERVER::SERVER(uint32_t _maxx, uint32_t _maxy, uint32_t _port_number,
    uint32_t _ROUNDS_PER_SEC, uint32_t _TURNING_SPEED, uint32_t _set_seed) {

    game_maxx = _maxx;
    game_maxy = _maxy;
    server_port = _port_number;
    FPS = _ROUNDS_PER_SEC;
    random_numbers.set_seed(_set_seed);
    TURNING_SPEED = _TURNING_SPEED;
    last_save_event_no = 0;
    game_ongoing = false; //normalnie false
    next_event_number = 0;
    events_initilised = false;

    pixel_event_vector.resize(22);//22  SIZE_BYTE_OF_PIXEL_EVENT
    player_eliminated_event_vector.resize(14); //SIZE_BYTE_OF_PLAYER_ELIMINATE_EVENT
    game_over_event_vector.resize(13); //SIZE_BYTE_OF_GAME_OVER_EVENT
}



bool SERVER::field_is_occupied(const double& x, const double& y) {
    if(x >= game_maxx || y >= game_maxy || x < 0 || y < 0){
        return false;
    }
    return GAME_BOARD[(uint32_t)x][(uint32_t)y];
}

void SERVER::take_board_field(const uint32_t& x, const uint32_t& y) {
    if(x >= game_maxx || y >= game_maxy){
        return;
    }
    GAME_BOARD[x][y] = true;
}


void SERVER::copy_to_vector(const char* buffer, vector<char>& v, int len) {
    for(int i = 0; i < len; i++) {
        v[i] = buffer[i];
    }
}


void SERVER::end_game() {
    game_ongoing = false;

    for(CONNECTED_CLIENT p: playing_clients) {
        if(p.is_connected()){
            waiting_players_clients.push_back(p);
        }
    }
    playing_clients.clear();
    for(CONNECTED_CLIENT& p: waiting_players_clients) {
        p.set_turn_direction(0);
        p.set_ready_for_new_game(false);
    }
}

void SERVER::initialize_new_game_board() {
    GAME_BOARD.clear();
    vector<bool> y_column;
    y_column.resize(game_maxy, 0);
    for(unsigned int i = 0; i < game_maxx; i++) {
        GAME_BOARD.push_back(y_column);
    }
}


void SERVER::disconnect_not_responding_clients() {
    //disconnect playing clients
    for (list<CONNECTED_CLIENT>::iterator it=playing_clients.begin(); it!=playing_clients.end(); ++it) {
        if((*it).is_connected() && (*it).should_disconnect()) {
            (*it).disconnect();
        }
    }
    //disconnect observers clients
    for (list<CONNECTED_CLIENT>::iterator it=observers_clients.begin(); it!=observers_clients.end(); ++it) {
        if((*it).should_disconnect()) {
            it = observers_clients.erase(it);
        }
    }
    //disconnect waiting players
    for (list<CONNECTED_CLIENT>::iterator it=waiting_players_clients.begin(); it!=waiting_players_clients.end(); ++it) {
        if((*it).should_disconnect()) {
            it = waiting_players_clients.erase(it);
        }
    }
}


bool SERVER::should_start_new_game() {
    if(waiting_players_clients.size() < number_of_player_when_game_over+1) //TO DO 2
        return false;
    for (CONNECTED_CLIENT t: waiting_players_clients) {
        if(!t.get_ready_for_new_game()) {
            return false;
        }
    }
    return true;
}

void SERVER::create_NEW_GAME() {

    playing_clients.clear();
    waiting_players_clients.sort(compare_nocase); //sort alfabetically
    int player_number = 0;
    int left_space_in_datagram = DATAGRAM_SIZE - SIZE_BYTE_OF_NEW_GAME_to_maxy - uint32_len; //space for names and crc32
    list<CONNECTED_CLIENT>::iterator it;

    for (it=waiting_players_clients.begin(); it!=waiting_players_clients.end(); ++it) {
        if(left_space_in_datagram >= (int)(*it).name.length() + 1) {// +1 for '\0'
            left_space_in_datagram -= (*it).name.length() + 1;
            (*it).set_player_number(player_number);
            (*it).eliminated = false;
            player_number++;
            playing_clients.push_back((*it));
        }
        else
            break;
    }
    not_eliminated_players_number = playing_clients.size();
    waiting_players_clients.erase(waiting_players_clients.begin(), it);

    memset(new_game_event_buffer, 0, DATAGRAM_SIZE);
    list<string> names;
    for(CONNECTED_CLIENT p: playing_clients)//playing_clients
        names.push_back(p.name);

    server_datagram.pack_new_game_to_datagram(new_game_event_buffer, EVENT_NUMBER_NEW_GAME, game_maxx, game_maxy, names);
    new_game_event_vector.resize(DATAGRAM_SIZE - left_space_in_datagram);

    last_save_event_no = 0;
    next_event_number = 1;
    copy_to_vector(new_game_event_buffer, new_game_event_vector, new_game_event_vector.size());

    events_history.push_back(EVENT(new_game_event_vector.size(), new_game_event_vector));
    events_initilised = true;
    send_datagrams_to_all_players();
}


void SERVER::create_PIXEL(const int8_t& player_number, const uint32_t& x, const uint32_t& y) {

    memset(pixel_event_buffer, 0, SIZE_BYTE_OF_PIXEL_EVENT);
    server_datagram.pack_pixel_to_datagram(pixel_event_buffer, next_event_number, player_number, x, y);

    next_event_number++;
    last_save_event_no++;
    pixel_event_vector.resize(22);

    copy_to_vector(pixel_event_buffer, pixel_event_vector, SIZE_BYTE_OF_PIXEL_EVENT);


    events_history.push_back(EVENT(SIZE_BYTE_OF_PIXEL_EVENT, pixel_event_vector));

    send_datagrams_to_all_players();
}


void SERVER::create_PLAYER_ELIMINATED(const int8_t& player_number) {
    memset(player_eliminated_event_buffer, 0, SIZE_BYTE_OF_PLAYER_ELIMINATE_EVENT);
    server_datagram.pack_player_eliminate(player_eliminated_event_buffer, next_event_number, player_number);
    not_eliminated_players_number--;
    next_event_number++;
    last_save_event_no++;
    player_eliminated_event_vector.resize(14);

    copy_to_vector(player_eliminated_event_buffer, player_eliminated_event_vector, SIZE_BYTE_OF_PLAYER_ELIMINATE_EVENT);

    events_history.push_back(EVENT(SIZE_BYTE_OF_PLAYER_ELIMINATE_EVENT, player_eliminated_event_vector));

    send_datagrams_to_all_players();
}


void SERVER::create_GAME_OVER() {

    memset(game_over_event_buffer, 0, SIZE_BYTE_OF_GAME_OVER_EVENT);

    server_datagram.pack_game_over_to_datagram(game_over_event_buffer, next_event_number);

    next_event_number++;
    last_save_event_no++;
    game_over_event_vector.resize(13);

    copy_to_vector(game_over_event_buffer, game_over_event_vector, SIZE_BYTE_OF_GAME_OVER_EVENT);
    events_history.push_back(EVENT(SIZE_BYTE_OF_GAME_OVER_EVENT, game_over_event_vector));
    end_game();
    send_datagrams_to_all_players();
}


void SERVER::start_new_game() {
    events_history.clear();//new game -> clear events history

    initialize_new_game_board();
    game_ongoing = true;
    game_id = (uint32_t)random_numbers.next_rand();
    create_NEW_GAME();

    double tmp_x, tmp_y;
    uint32_t tmp_direction;
    for(CONNECTED_CLIENT& p: playing_clients) {
        tmp_x = (random_numbers.next_rand() % game_maxx) + 0.5;
        tmp_y = (random_numbers.next_rand() % game_maxy) + 0.5;
        tmp_direction = random_numbers.next_rand() % 360;
        p.initialize_new_game(tmp_x, tmp_y, tmp_direction);

        if(!field_is_occupied(tmp_x, tmp_y)){
            take_board_field(tmp_x, tmp_y);
            create_PIXEL(p.get_player_number(), tmp_x, tmp_y);
        }
        else {
            create_PLAYER_ELIMINATED(p.get_player_number());
            p.eliminated = true;
        }
    }
}



void SERVER::execute_next_round() {
    list<CONNECTED_CLIENT>::iterator player;
    for (player=playing_clients.begin(); player!=playing_clients.end(); ++player) {
        if((*player).eliminated)
            continue;

        (*player).turn(TURNING_SPEED);
        (*player).move_one_unit();
        if (!(*player).position_changed()) {
            continue;
        }
        if (!(*player).fits_on_board(game_maxx, game_maxy)) {
            create_PLAYER_ELIMINATED((*player).get_player_number());
            waiting_players_clients.push_back((*player));
            //player = playing_clients.erase(player);
            (*player).eliminated = true;
            if(not_eliminated_players_number <= 1){
                create_GAME_OVER();
                return;
            }
            continue;
        }
        else if (field_is_occupied((*player).last_int_x, (*player).last_int_y)) {
            create_PLAYER_ELIMINATED((*player).get_player_number());
            waiting_players_clients.push_back((*player));
            //player = playing_clients.erase(player);
            (*player).eliminated = true;
            if(not_eliminated_players_number <= 1){
                create_GAME_OVER();
                return;
            }
            continue;
        }
        take_board_field((*player).x_integer(), (*player).y_integer());
        create_PIXEL((*player).get_player_number(), (*player).last_int_x, (*player).last_int_y);
    }
}

void SERVER::pack_to_datagram_from_event(const uint32_t& start_event_no, int& size_send, uint32_t& next_event_number) {
    memset(datagram_buffer, 0, DATAGRAM_SIZE);
    int len = 0 + uint32_len;//always start with game_id 4 bytes
    server_datagram.pack_game_id(datagram_buffer, game_id);
    for(unsigned int i = start_event_no; i <= last_save_event_no; i++) {
        if(!pack_event_to_datagram(events_history[i], len)) {
            next_event_number = i;
            size_send = len;
            return;
        }
        len += events_history[i].len;
        next_event_number = last_save_event_no+1;
    }
    size_send = len;
};

bool SERVER::pack_event_to_datagram(EVENT event, int start) {
    if(start + event.len -1 > DATAGRAM_SIZE-1)
        return false;
    for(int i = 0; i < event.len; i++) {
        datagram_buffer[i+start] = event.datagram_particle[i];
    }
    return true;
}




void SERVER::create_socket_and_bin() {


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, to_string(server_port).data(), &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd_server = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd_server, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd_server);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        exit(1);
    }

    freeaddrinfo(servinfo);
    fcntl(sockfd_server, F_SETFL, O_NONBLOCK);
}



void SERVER::disconnect_client(struct sockaddr adrr) {
        //disconnect playing clients
        for (list<CONNECTED_CLIENT>::iterator it=playing_clients.begin(); it!=playing_clients.end(); ++it) {

            if((*it).same_socket(adrr) && (*it).is_connected()) {
                (*it).disconnect();
                //not_eliminated_players_number--;  NO, because snake still should go forward
                //if not_eliminated_players_number <= 1 game over TO DO
                return;
            }
        }
        //disconnect observers clients
        for (list<CONNECTED_CLIENT>::iterator it=observers_clients.begin(); it!=observers_clients.end(); ++it) {
            if((*it).same_socket(adrr)) {
                it = observers_clients.erase(it);
                return;
            }
        }
        //disconnect waiting players
        for (list<CONNECTED_CLIENT>::iterator it=waiting_players_clients.begin(); it!=waiting_players_clients.end(); ++it) {
            if((*it).same_socket(adrr)) {
                it = waiting_players_clients.erase(it);
                return;
            }
        }
}


bool SERVER::exist_client_with_sockaddresinf(struct sockaddr adrr) {
//disconnect playing clients
    for (list<CONNECTED_CLIENT>::iterator it=playing_clients.begin(); it!=playing_clients.end(); ++it) {
        if((*it).same_socket(adrr) && (*it).is_connected()) {
            return true;
        }
    }
    //disconnect observers clients
    for (list<CONNECTED_CLIENT>::iterator it=observers_clients.begin(); it!=observers_clients.end(); ++it) {
        if((*it).same_socket(adrr)) {
            return true;
        }
    }
    //disconnect waiting players
    for (list<CONNECTED_CLIENT>::iterator it=waiting_players_clients.begin(); it!=waiting_players_clients.end(); ++it) {
        if((*it).same_socket(adrr)) {
            return true;
        }
    }
    return false;
}


void SERVER::update_or_substitute_client(struct sockaddr adrr) {
    for (list<CONNECTED_CLIENT>::iterator it=playing_clients.begin(); it!=playing_clients.end(); ++it) {
        if((*it).same_socket(adrr) && (*it).is_connected()) {
            updating(it);
            return;
        }
    }
    for (list<CONNECTED_CLIENT>::iterator it=observers_clients.begin(); it!=observers_clients.end(); ++it) {
        if((*it).same_socket(adrr)) {
            updating(it);
            return;
        }
    }
    for (list<CONNECTED_CLIENT>::iterator it=waiting_players_clients.begin(); it!=waiting_players_clients.end(); ++it) {
        if((*it).same_socket(adrr)) {
            updating(it);
            return;
        }
    }
}




bool SERVER::client_with_name_exist(const string& name) {
    if(name == "")
        return false;

    for (list<CONNECTED_CLIENT>::iterator it=playing_clients.begin(); it!=playing_clients.end(); ++it) {
        if((*it).is_connected() && (*it).name == name) {
            return true;
        }
    }

    for (list<CONNECTED_CLIENT>::iterator it=waiting_players_clients.begin(); it!=waiting_players_clients.end(); ++it) {
        if((*it).is_connected() && (*it).name == name) {
            return true;
        }
    }
    return false;
}





void SERVER::connect_new_client() {
    //podlacz nowego klienta
    CONNECTED_CLIENT cl = CONNECTED_CLIENT(recv_player_name, *(struct sockaddr *)&client_addr, recv_session_id, recv_turn_direction);
    if(recv_player_name == "") {//observer only
        observers_clients.push_back(cl);
    }
    else {//new player
        waiting_players_clients.push_back(cl);
    }
    send_datagrams_to_client(cl);
}






void SERVER::receive_from_clients() {
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    //fd_set write_fds;
    int fdmax;        // maximum file descriptor number
    int rcv_len;

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    // add the listener to the master set
    FD_SET(sockfd_server, &master);

    // keep track of the biggest file descriptor
    fdmax = sockfd_server; // so far, it's this one
    //while(true) {
        read_fds = master; // copy to read file descriptor set
        if (select(fdmax+1, &read_fds, NULL, NULL, &tv) == -1) {
            perror("select");
            exit(1);
        }
        addr_len = sizeof client_addr;

        if (FD_ISSET(sockfd_server, &read_fds)) {
            memset(recv_client_buffer, 0, RECV_CLIENT_DATAGRAM_SIZE);

            if ((rcv_len = recvfrom(sockfd_server, recv_client_buffer, RECV_CLIENT_DATAGRAM_SIZE,
                                    0, (struct sockaddr *)&client_addr, &addr_len)) == -1) {
                //perror("recvfrom");
            }

            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                //perror("socket now is empty");
                return;
            }
            if (rcv_len < 0) {
                perror("read tcp");
                return;
            }
            if (rcv_len == 0) {
                perror("gui disconnected"); //should not occur since using UDP not TCP
                return;
            }
            //odebralismy datagram
            if(!server_datagram.read_datagram_from_client(recv_client_buffer, rcv_len,
                                                      recv_session_id, recv_turn_direction,
                                                      recv_next_expected_event_no, recv_player_name)) {

                return; //invalid arguments
            }



            if(!exist_client_with_sockaddresinf(*(struct sockaddr *)&client_addr)) {
                if(client_with_name_exist(recv_player_name))
                    return; //skip datagram
                connect_new_client();
            }
            else {
                update_or_substitute_client(*(struct sockaddr *)&client_addr);
            }
        }
    //}

}




void SERVER::send_datagrams_to_client(CONNECTED_CLIENT cl) {
    fd_set master;    // master file descriptor list
    fd_set write_fds;
    int fdmax;        // maximum file descriptor number
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    // add the listener to the master set
    FD_SET(sockfd_server, &master);
    // keep track of the biggest file descriptor
    fdmax = sockfd_server; // so far, it's this one

    write_fds = master; // copy to read file descriptor set
    if (select(fdmax+1, NULL, &write_fds, NULL, &tv) == -1) {
        perror("select");
        exit(1);
    }
    addr_len = sizeof client_addr;
    uint32_t next_event_to_start_load = cl.next_expected_event_no;
    int send_size;
    //uint32_t
    if (FD_ISSET(sockfd_server, &write_fds)) {
        while(next_event_to_start_load <= last_save_event_no && events_initilised) {

            memset(datagram_buffer, 0, DATAGRAM_SIZE);
            pack_to_datagram_from_event(next_event_to_start_load, send_size, next_event_to_start_load);
            if ((numbytes=sendto(sockfd_server, datagram_buffer, send_size, 0,
                                 (struct sockaddr *)&cl.adr, sizeof cl.adr)) == -1) {
                //perror("sendto");
                //exit(1);
            }
            if(numbytes != send_size)
                return;
        }
    }
}



void SERVER::send_datagrams_to_all_players() {
    for (CONNECTED_CLIENT& c: playing_clients)
        send_datagrams_to_client(c);

    for (CONNECTED_CLIENT& c: waiting_players_clients)
        send_datagrams_to_client(c);

    for (CONNECTED_CLIENT& c: observers_clients)
        send_datagrams_to_client(c);
}