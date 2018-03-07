//
// Created by karol on 20.08.17.
//

#ifndef UNTITLED_CLIENT_H
#define UNTITLED_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string>
#include <list>
#include "datagram.h"
#include <vector>

#include <regex>
#include <sys/time.h>


using namespace std;

const int MESSAGE_FROM_GUI_LENGTH = 20;
const size_t CLIENT_DATAGRAM_NUMERIC_FIELDS_LENGTH = 13; //64 + 32 + 8 bits
const int number_of_players_min = 2;

enum analiza_datagramu {end_analize_datagram = true, end_client_program = false, next_event = true};






const uint64_t CLIENT_TO_SERVER_DATAGRAM_INTERVAL = 20000; // 20ms


class Clock {

private:
    uint64_t session_id;
    uint64_t time_milisec_since_last_update;
    struct timeval tval;
    uint64_t interval_in_milisec;
    uint64_t current_tmp;
    //void update_milisec ();

public:
    bool expired_last_time_interval();
    uint64_t get_session_id();
    Clock ();

};




class Client {

private:
    /*************player logic****************/
    int8_t current_turn_direction;
    int64_t last_sent_event_number;
    uint64_t session_id;
    string player_name;
    Clock client_clock = Clock();


    /************game logic***************/
    uint32_t current_game_id;
    uint32_t next_expected_event_no;
    uint32_t maxx = 500, maxy = 500;
    list<string> current_game_name_list; //cleared with new game, change during game
    vector<string> current_players_names_arr;
    vector<bool> players_not_eliminated;
    int8_t number_of_players;
    int8_t not_eliminated_players_number;
    bool game_continues;
    bool game_over;

    /********temporary data from datagram**********/
    char gui_buffer[MESSAGE_FROM_GUI_LENGTH];
    ssize_t recv_length;
    int8_t recv_event_type;
    uint32_t recv_x, recv_y;
    int8_t recv_player_number;
    uint32_t recv_game_id;
    uint32_t finished_game_id;
    uint32_t tmp_event_len;
    uint32_t recv_event_no;
    uint32_t recv_maxx, recv_maxy;
    Client_datagram client_datagram;
    size_t client_datagram_length;




private:
    string game_server_host, game_server_port;
    int server_sockfd;
    struct addrinfo server_ints, *servinfo;
    struct addrinfo *p;
    string game_ui_host, game_ui_port;
    int gui_sockfd;
    struct addrinfo gui_ints, *guiinfo;



    //int next_free_byte;

private:
    int8_t take_direction_from_gui_message(const string& button_received);

    bool verify_new_game(const int& maxx, const int& maxy, list<string>& player_names);
    bool verify_pixel_event(const int& player_number, const int& x, const int& y);
    bool verify_player_eliminate(const int& player_number);
    bool verify_game_over();

    bool x_in_board(const uint32_t& x) {return 0 <= x && x <= maxx;}

    bool y_in_board(const uint32_t& y) {return 0 <= y && y <= maxy;}

    bool player_number_correct(const int8_t& player_number)
        {return 0 <= player_number && player_number <= not_eliminated_players_number;}



public:
    char datagram_buffer[DATAGRAM_SIZE+1];


public:
    //bool try_rec_message_from_gui();
    bool send_new_game_to_gui();
    bool send_pixel_to_gui(const int &x, const int &y, const string &player_name);
    bool send_player_eliminate_to_gui(const string &player_name);


public:
    explicit Client (string name,
                     const string& game_server_host, const string& game_server_port,
                     const string& game_ui_host, const string& game_ui_port);
    bool should_sent_next_datagram();

    bool connect_to_server();
    bool connect_to_gui();

    bool update_direction();
    bool send_datagram_to_server();

    bool receive_datagram_from_server();


    bool try_to_apply_new_game();
    bool try_to_apply_pixel();
    bool try_to_apply_player_eliminated();
    bool try_to_apply_game_over();

    bool try_apply_event();



};

#endif //UNTITLED_CLIENT_H
