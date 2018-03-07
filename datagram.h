#ifndef UNTITLED_DATAGRAM_H
#define UNTITLED_DATAGRAM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <endian.h>
#include <list>
#include <string>


using namespace std;

const int DATAGRAM_SIZE = 512;
const int MIN_CLIENT_DATAGRAM_SIZE = 8 + 1 + 4;//uint64 + int8 + uint32 + player_name

const int SESSION_ID_POS = 0;
const int TURN_DIRECTION_POS = 8;
const int NEXT_EXP_EVENT_NUMBER_POS = 9;
const int PLAYER_NAME = 13;

const int MAX_PLAYER_NAME_LEN = 64;
const int MIN_VALID_PLAYER_NAME_SYMBOL = 33;
const int MAX_VALID_PLAYER_NAME_SYMBOL = 126;

const int EVENT_TYPE_POS = 0;
const int LEN_POS = 0;
const int EVENT_NO = 4;
const int EVENT_TYPE = 8;
const int EVENT_DATA_POS = 9;
const size_t CRC32_PIXEL_POS = EVENT_DATA_POS + 9;
const size_t CRC32_PLAYER_ELIMINATE_POS = EVENT_DATA_POS + 1;
const size_t CRC32_GAME_OVER_POS = EVENT_DATA_POS;
const int uint64_len = 8;
const int uint32_len = 4;
const int int8_len = 1;

const int LAST_BYTE_OF_NEW_GAME_maxy = 16;
const int LAST_BYTE_OF_PIXEL_EVENT = CRC32_PIXEL_POS + uint32_len-1;
const int LAST_BYTE_OF_PLAYER_ELIMINATED_EVENT = CRC32_PLAYER_ELIMINATE_POS + uint32_len-1;
const int LAST_BYTE_OF_GAME_OVER_EVENT = CRC32_GAME_OVER_POS + uint32_len-1;


const int SIZE_BYTE_OF_NEW_GAME_to_maxy = 17;
const int SIZE_BYTE_OF_PIXEL_EVENT = CRC32_PIXEL_POS + uint32_len;
const int SIZE_BYTE_OF_PLAYER_ELIMINATE_EVENT = CRC32_PLAYER_ELIMINATE_POS + uint32_len;
const int SIZE_BYTE_OF_GAME_OVER_EVENT = CRC32_GAME_OVER_POS + uint32_len;


const uint32_t NEW_GAME_EVENTS_FIELDS_to_maxy = 13;
const uint32_t PIXEL_EVENTS_FIELDS_SIZE = 14;
const uint32_t PLAYER_ELIMINATED_EVENTS_FIELDS_SIZE = 6;
const uint32_t GAME_OVER_EVENTS_FIELDS_SIZE = 5;


const int8_t EVENT_TYPE_NEW_GAME = 0;
const int8_t EVENT_TYPE_PIXEL = 1;
const int8_t EVENT_TYPE_PLAYER_ELIMINATED = 2;
const int8_t EVENT_TYPE_GAME_OVER = 3;



enum unpack_event_flag {
    UNPACK_SUCCESFULL,
    CORRECT_CRC32,
     };


class Datagram {

protected:
    //char datagram_buffer[DATAGRAM_SIZE+1];// \0
    char crc32_new_game_buffer[DATAGRAM_SIZE];
    char crc32_pixel_buffer[CRC32_PIXEL_POS];
    char crc32_player_eliminate_buffer[CRC32_PLAYER_ELIMINATE_POS];
    char crc32_game_over_buffer[CRC32_GAME_OVER_POS];

    char players_names_buffer[DATAGRAM_SIZE];
    int players_names_buffer_length;

    int next_free_byte;
    int recv_length;
    int current_event_start;
    uint32_t event_len;
    uint32_t len_crc32;
    uint32_t crc32;


protected:
    void reset_crc32_new_game_buffer() {memset(crc32_new_game_buffer, 0 , DATAGRAM_SIZE);}
    void reset_crc32_pixel_buffer() {memset(crc32_pixel_buffer, 0 , CRC32_PIXEL_POS);}
    void reset_crc32_player_eliminate_buffer() {memset(crc32_player_eliminate_buffer, 0 , CRC32_PLAYER_ELIMINATE_POS);}
    void reset_crc32_game_over_buffer() {memset(crc32_game_over_buffer, 0 , CRC32_GAME_OVER_POS);}
    void reset_players_names_buffer() {{memset(players_names_buffer, 0 , DATAGRAM_SIZE);}}

    uint32_t checksum_current_new_game(const char* datagram, const size_t& event_len);
    uint32_t checksum_current_pixel(const char* datagram);
    uint32_t checksum_current_player_eliminated(const char* datagram);
    uint32_t checksum_current_game_over(const char* datagram);

    uint32_t compute_checksum(const char* buff, const int& len);
    //void go_to_next_event(int current_event_size) {current_event_start += current_event_size;}


public:
    void get_int8_bit_value_fbuffer(const char* datagram, int8_t& v, int start_in_datagram);
    void get_int32_bit_value_fbuffer(const char* datagram, uint32_t& v, int start_in_datagram);
    void get_int64_bit_value_fbuffer(const char* datagram, uint64_t& v, int start_in_datagram);
    void get_string_fbuffer(const char* datagram, string& s, int start_in_datagram, int end_in_datagram);

    bool compare_checksums_crc32(const string &recv_fields, uint32_t recv_checksum);
    bool fields_fit_in_datagram(const int &last_byte_of_event);

    void copy_names_to_players_names_buffer(const char* datagram);

    string chain_list(const list<string>& player_names);

public:
    void clear_datagram(char * datagram);

    void pack_next_int8_bit_value_buffer(char* datagram, int8_t v);
    void pack_next_uint32_bit_value_buffer(char* datagram, uint32_t v);
    void pack_next_uint64_bit_value_buffer(char* datagram, uint64_t v);
    void pack_next_string_buffer(char* datagram, string s);
    void pack_name_list_buffer(char* datagram, const list<string>& l);
    void pack_char_to_datagram(char* datagram, const char s) {datagram[next_free_byte] = s; next_free_byte++;}


    Datagram();
    int get_next_free_byte() {return next_free_byte;}
    void set_recv_length(int len) {recv_length = len;}
    int get_recv_length() {return recv_length ;}


    bool event_checksum_correct(const char* datagram, size_t len_field);
    void go_to_next_event(const int& full_event_length);
    bool give_player_list(list<string>& player_names);


};




class Client_datagram: public Datagram {
public:
    //klient
    //-odbiera client_datagram
    //-tworzy wlasny client_datagram
    void create_datagram_for_server(char* datagram,
                                     const uint64_t &session_id, const int8_t &turn_direction,
                                     const uint32_t &next_expected_event_no, const string& player_name);



    int get_next_event_start_position() {return current_event_start;};
    bool read_datagram_from_client(const char* datagram,
                                   uint64_t& session_id, int8_t& turn_direction,
                                   uint32_t& next_expected_event_no, string& player_name);

    bool get_game_id(const char* datagram, uint32_t& game_id);
    bool get_event_no(const char* datagram, uint32_t& event_no);

    bool get_next_event_length(const char* datagram, uint32_t& len);
    bool get_next_event_type(const char* datagram, int8_t& event_type);
    //gdy ponizsze funkcje zwracaja true, to znaczy ze wczytano sensowne dane, ale wciaz nalezy je sprawdzic
    bool get_next_event_new_game(const char* datagram, uint32_t& maxx, uint32_t& maxy, list<string>& player_names);
    bool get_next_event_pixel(const char* datagram, int8_t& player_number, uint32_t&x, uint32_t&y);
    bool get_next_event_player_eliminate(const char* datagram, int8_t& player_number);
    bool get_next_event_game_over(const char* datagram);

    bool datagram_starts_with_new_game(const char* datagram);

    void skip_event(const int& event_type) {
        if(event_type == EVENT_TYPE_PIXEL)
            current_event_start += SIZE_BYTE_OF_PIXEL_EVENT;
        if(event_type == EVENT_TYPE_PLAYER_ELIMINATED)
            current_event_start += SIZE_BYTE_OF_PLAYER_ELIMINATE_EVENT;
        if(event_type == EVENT_TYPE_GAME_OVER)
            current_event_start += SIZE_BYTE_OF_GAME_OVER_EVENT;
    }
    //void receive_datagram();
private:
    //-analizuje client_datagram odebrany
    //wysyla do gui
    //void apply_next_event();
    //void send_to_gui();




};



class Server_datagram: public Datagram {
public:
    bool read_datagram_from_client(const char* datagram, const int& recv_length,
                                   uint64_t& session_id, int8_t& turn_direction,
                                   uint32_t& next_expected_event_no, string& player_name);


    bool pack_game_id(char *datagram, const uint32_t &game_id);
    bool pack_new_game_to_datagram(char* datagram, const uint32_t& event_no, const uint32_t& maxx, const uint32_t& maxy, const list<string>& player_names);
    bool pack_pixel_to_datagram(char* datagram, const uint32_t& event_no, const int8_t& player_number, const uint32_t&x, const uint32_t&y);
    bool pack_player_eliminate(char* datagram, const uint32_t& event_no, const int8_t& player_number);
    bool pack_game_over_to_datagram(char* datagram, const uint32_t& event_no);


    uint32_t checksum_new_game(const char* datagram, const size_t& event_len);
    uint32_t checksum_pixel(const char* datagram);
    uint32_t checksum_player_eliminated(const char* datagram);
    uint32_t checksum_game_over(const char* datagram);


private:
    bool data_will_fit_to_datagram(const int &last_byte_pos);
    int length_of_new_game_event_fields(const string &player_names);

};


#endif //UNTITLED_DATAGRAM_H
