

#ifndef UNTITLED_SERVER_H
#define UNTITLED_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string>
#include <list>
#include "datagram.h"
#include <vector>
#include <sys/time.h>
#include <arpa/inet.h>
#include <iostream>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

using namespace std;

//const int MESSAGE_FROM_GUI_LENGTH = 20;
//const size_t CLIENT_DATAGRAM_NUMERIC_FIELDS_LENGTH = 13; //64 + 32 + 8 bits
const uint64_t two_secunds = 2000000;
const uint64_t usecond = 1000000;
const uint32_t EVENT_NUMBER_NEW_GAME = 0;
const uint32_t RECV_CLIENT_DATAGRAM_SIZE = 100;

enum session_compare {current_session_lesser = 0, identical = 1, current_session_greater = 2};

const double PI = 3.14159265;

const int number_of_player_when_game_over = 1;

/*
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



bool same_socket_addres(struct sockaddr *lhs, struct sockaddr *rhs) {
    if(lhs->sa_family == rhs->sa_family) {

        if (lhs->sa_family == AF_INET) {//IPv4
            if(!((struct sockaddr_in*)lhs)->sin_port == (((struct sockaddr_in*)rhs)->sin_port)){//port comparison
                return false;
            }
            return ((struct sockaddr_in*)lhs)->sin_addr.s_addr == ((struct sockaddr_in*)rhs)->sin_addr.s_addr;//addres comparison
        }

        //IPv6
        if(!((struct sockaddr_in6*)lhs)->sin6_port == (((struct sockaddr_in6*)rhs)->sin6_port)){//port comparison
            return false;
        }
        char ip6_lhs[INET6_ADDRSTRLEN];
        char ip6_rhs[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, get_in_addr((struct sockaddr *)&lhs), ip6_lhs, sizeof ip6_lhs);
        inet_ntop(AF_INET6, get_in_addr((struct sockaddr *)&rhs), ip6_rhs, sizeof ip6_rhs);
        for(int i = 0; i < INET6_ADDRSTRLEN; i++) {
            if(ip6_lhs[i] != ip6_rhs[i])
                return false;
        }
        return true;
    }
    return false; //sa_family not match or nor IPv4 IPv6
}
*/



class EVENT {

public:
    vector<char> datagram_particle;
    int len;

    explicit EVENT(int _len, vector<char>& _datagram_particle) {
        len = _len;
        datagram_particle.clear();
        for(char ch: _datagram_particle){
            datagram_particle.push_back(ch);
        }
    }

};


class CONNECTED_CLIENT {

public:
    int next_expected_event_no;
    string name;
    bool eliminated;
    static double deg2rad(int deg) {return (double)deg * PI / 180.0;}
    uint32_t last_int_x, last_int_y;
    struct sockaddr adr;
    uint64_t session_id;


private:
    int port;
    bool connected;
    uint64_t last_notification;
    int8_t turn_direction;
    double x,y;
    int direction;
    struct timeval tval;
    int8_t player_number;
    bool ready_for_new_game;
    double radx;

    void *get_in_addr(struct sockaddr *sa)
    {
        if (sa->sa_family == AF_INET) {
            return &(((struct sockaddr_in*)sa)->sin_addr);
        }

        return &(((struct sockaddr_in6*)sa)->sin6_addr);
    }





    bool same_socket_addres(struct sockaddr *lhs, struct sockaddr *rhs) {

        if(lhs->sa_family == rhs->sa_family) {

            if (lhs->sa_family == AF_INET) {//IPv4

                if(((struct sockaddr_in*)lhs)->sin_port != (((struct sockaddr_in*)rhs)->sin_port)){//port comparison
                    return false;
                }
                return ((struct sockaddr_in*)lhs)->sin_addr.s_addr == ((struct sockaddr_in*)rhs)->sin_addr.s_addr;//addres comparison
            }

            //IPv6
            if(((struct sockaddr_in6*)lhs)->sin6_port != (((struct sockaddr_in6*)rhs)->sin6_port)){//port comparison
                return false;
            }
            char ip6_lhs[INET6_ADDRSTRLEN];
            char ip6_rhs[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, get_in_addr((struct sockaddr *)&lhs), ip6_lhs, sizeof ip6_lhs);
            inet_ntop(AF_INET6, get_in_addr((struct sockaddr *)&rhs), ip6_rhs, sizeof ip6_rhs);
            for(int i = 0; i < INET6_ADDRSTRLEN; i++) {
                if(ip6_lhs[i] != ip6_rhs[i])
                    return false;
            }
            return true;
        }
        return false; //sa_family not match or nor IPv4 IPv6
    }



public:
    explicit CONNECTED_CLIENT(string _name, struct sockaddr _adr, uint64_t _session_id, int8_t turn_direction);
    void initialize_new_game(double _x, double _y, int _direction)
        {x = _x; y = _y; direction = _direction; next_expected_event_no = 0; last_int_x = (uint32_t)x, last_int_y = (uint32_t)y; connected = true;}

    void change_direction(int new_direction) {direction = new_direction;}
    void change_position(double new_x, double new_y) {x = new_x; y = new_y;}
    void set_player_number(int8_t number) {player_number = number;}
    int8_t get_player_number() {return player_number;}
    bool is_connected() {return connected;}
    bool same_socket(struct sockaddr _adr) {return same_socket_addres((struct sockaddr *)&adr, (struct sockaddr *)&_adr);}

    bool get_ready_for_new_game() {return ready_for_new_game;}
    void set_ready_for_new_game(bool ready) {ready_for_new_game = ready;}
    void set_turn_direction(int8_t new_turn_direction) {
        turn_direction = new_turn_direction;
        if(turn_direction != 0){
            ready_for_new_game = true;
        }
    }
    session_compare same_session_id(uint64_t _session_id) {
        if(session_id < _session_id)
            return current_session_lesser;
        if(session_id > _session_id)
            return current_session_greater;
        return identical;
    }
    int get_direction() {return direction;}
    void move_one_unit() {
        radx = deg2rad(direction);
        x += cos(radx);
        y -= sin(radx);
    }
    bool should_disconnect() {
        if(gettimeofday(&tval, NULL) == 0) {
            return last_notification + two_secunds <= (uint64_t)tval.tv_usec + (uint64_t)tval.tv_sec*usecond;//TO DO
        }
        return false;
    }
    void disconnect() {connected = false;}
    void refresh_last_notification(uint32_t _next_expected_event_no) {
        next_expected_event_no = _next_expected_event_no;
        if(gettimeofday(&tval, NULL) == 0) {
            last_notification = (uint64_t)tval.tv_usec + (uint64_t)tval.tv_sec*usecond;
        }
    }
    void turn(const uint32_t TURNING_SPEED) {
        if(turn_direction == -1)
            direction = (direction + 360 - TURNING_SPEED) % 360;
        if(turn_direction == 1)
            direction = (direction + TURNING_SPEED) % 360;
    }
    bool fits_on_board(const int &maxx, const int &maxy) {
        return 0 <= x && x < maxx && 0 <= y && y < maxy;
    }
    uint32_t x_integer() {return (uint32_t) x;}
    uint32_t y_integer() {return (uint32_t) y;}
    bool position_changed() {
        bool changed;
        changed = (last_int_x != x_integer() || last_int_y != y_integer());
        last_int_x = x_integer();
        last_int_y = y_integer();
        return changed;
    }
};



class RANDOM {

private:
    uint64_t r_0;
    uint64_t previous_rand;
    uint64_t return_rand;

public:
    explicit RANDOM() {
        r_0 = 0;
        previous_rand = 0;
    }
    void set_seed(uint64_t seed) {r_0 = seed; previous_rand = seed;}
    uint64_t next_rand() {return_rand = previous_rand;
        previous_rand = (previous_rand * 279470273) % 4294967291; return return_rand;}
    uint64_t previous_rand_rand() {return previous_rand;}
};



class ROUND_TIMER {

private:
    uint64_t time_milisec_since_last_update;
    struct timeval tval;
    uint64_t interval_in_milisec;
    uint64_t tmp;

public:
    bool new_round_start() {
        if(gettimeofday(&tval, NULL) == 0) {
            tmp = (uint64_t)tval.tv_usec + (uint64_t)tval.tv_sec*usecond;
            if(tmp >= interval_in_milisec + time_milisec_since_last_update) {
                time_milisec_since_last_update = (uint64_t)tval.tv_usec + (uint64_t)tval.tv_sec*usecond;
                return true;
            }
        }
        return false;
    }
    explicit ROUND_TIMER (uint64_t fps) {interval_in_milisec = usecond/fps;}

};





class SERVER {

public:

    /********** connected players and observers **********/
    list<CONNECTED_CLIENT> playing_clients;
    list<CONNECTED_CLIENT> waiting_players_clients;
    list<CONNECTED_CLIENT> observers_clients;

private:


    /************server game settings*************/
    int8_t not_eliminated_players_number;
    uint32_t game_id;
    uint32_t game_maxx, game_maxy;
    bool game_ongoing;
    uint64_t FPS;
    uint64_t round_time;
    vector<EVENT> events_history;
    uint32_t next_event_number;
    Server_datagram server_datagram;
    vector<vector<bool>> GAME_BOARD;
    RANDOM random_numbers;
    uint32_t TURNING_SPEED;
    uint32_t last_save_event_no;
    bool events_initilised;
    uint64_t server_session_id;

    /****************server socket**********************/
    int sockfd_server;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    int server_port;
    struct sockaddr_storage client_addr;
    char datagram_buffer[DATAGRAM_SIZE + 1];
    char recv_client_buffer[RECV_CLIENT_DATAGRAM_SIZE + 1];
    socklen_t addr_len;


    /*********buffers for creating events************/
    char new_game_event_buffer[DATAGRAM_SIZE+1];
    char pixel_event_buffer[SIZE_BYTE_OF_PIXEL_EVENT];
    char player_eliminated_event_buffer[SIZE_BYTE_OF_PLAYER_ELIMINATE_EVENT];
    char game_over_event_buffer[SIZE_BYTE_OF_GAME_OVER_EVENT];

    vector<char> new_game_event_vector;
    vector<char> pixel_event_vector;
    vector<char> player_eliminated_event_vector;
    vector<char> game_over_event_vector;

    /***********client datagram temporary receive*************/
    uint64_t recv_session_id;
    int8_t recv_turn_direction;
    uint32_t recv_next_expected_event_no;
    string recv_player_name;


public:
    explicit SERVER(uint32_t _maxx, uint32_t _maxy, uint32_t _port_number,
                    uint32_t _ROUNDS_PER_SEC, uint32_t _TURNING_SPEED, uint32_t _set_seed);





    bool currently_game_ongoing() {return game_ongoing;}

    void disconnect_not_responding_clients();

    void execute_next_round();

    bool should_start_new_game();

    void receive_from_clients();

    void send_datagrams_to_client(CONNECTED_CLIENT cl); //parametr client
    void send_datagrams_to_all_players();

    void create_socket_and_bin();

    void pack_to_datagram_from_event(const uint32_t& start_event_no, int& size_send, uint32_t& next_event_number); //return (length to send , next_not_include_event_no))

    void start_new_game();
    void check_if_game_over() {
        if(not_eliminated_players_number <= number_of_player_when_game_over && game_ongoing){//TO DO 1
            create_GAME_OVER();
        }
    }

    bool client_with_name_exist(const string& name);

    void create_NEW_GAME();
    void create_PIXEL(const int8_t& player_number, const uint32_t& x, const uint32_t& y);
    void create_PLAYER_ELIMINATED(const int8_t& player_number);
    void create_GAME_OVER();

private:
    void copy_to_vector(const char* buffer, vector<char>& v, int len);
    void end_game();
    void initialize_new_game_board();
    bool field_is_occupied(const double& x, const double& y);
    void take_board_field(const uint32_t& x, const uint32_t& y);
    bool pack_event_to_datagram(EVENT event, int start);

    void disconnect_client(struct sockaddr adrr);
    bool exist_client_with_sockaddresinf(struct sockaddr adrr);
    void connect_new_client();
    void update_or_substitute_client(struct sockaddr adrr);
    void updating(list<CONNECTED_CLIENT>::iterator it) {
        if(recv_session_id == (*it).session_id && (*it).name == recv_player_name) {
            (*it).refresh_last_notification(recv_next_expected_event_no);
            (*it).set_turn_direction(recv_turn_direction);
            send_datagrams_to_client((*it));
        }
        else if(recv_session_id > (*it).session_id) {
            (*it).disconnect();
            disconnect_client((*it).adr);
            connect_new_client();
            //send_datagrams_to_client((*it));
        }
    }


};





#endif //UNTITLED_SERVER_H

