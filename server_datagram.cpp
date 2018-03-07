#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <endian.h>
#include <iostream>
#include <string>
#include "datagram.h"
#include <boost/crc.hpp>





bool Server_datagram::read_datagram_from_client(const char* datagram, const int& recv_length,
                                                uint64_t& session_id, int8_t& turn_direction,
                                                uint32_t& next_expected_event_no, string& player_name) {
    if(recv_length < MIN_CLIENT_DATAGRAM_SIZE) {
        //cerr<<"name have invalid haracters\n";
        return false;
    }
    get_int64_bit_value_fbuffer(datagram, session_id, SESSION_ID_POS);
    get_int8_bit_value_fbuffer(datagram, turn_direction, TURN_DIRECTION_POS);
    get_int32_bit_value_fbuffer(datagram, next_expected_event_no, NEXT_EXP_EVENT_NUMBER_POS);

    if(turn_direction != 0 && turn_direction != -1 && turn_direction != 1)
        return false;




    if(recv_length == MIN_CLIENT_DATAGRAM_SIZE) {
        player_name = "";
    }
    else{

        get_string_fbuffer(datagram, player_name, PLAYER_NAME, recv_length-1);
        if(player_name.length() > MAX_PLAYER_NAME_LEN) {
            return false;
        }

        for(char c : player_name){
            if(c < MIN_VALID_PLAYER_NAME_SYMBOL ||
               c > MAX_VALID_PLAYER_NAME_SYMBOL) {
                return false;
            }
        }
    }
    return true;
}


bool Server_datagram::data_will_fit_to_datagram(const int &last_byte_pos) {
    return DATAGRAM_SIZE-1 >= last_byte_pos;
}


int Server_datagram::length_of_new_game_event_fields(const string &player_names) {
    auto length_of_names = (int)player_names.length();
    //length_of_names += 1; // '\0' after last name
    return SIZE_BYTE_OF_NEW_GAME_to_maxy + length_of_names - uint32_len;
}

bool Server_datagram::pack_game_id(char *datagram, const uint32_t &game_id) {
    next_free_byte = 0;
    pack_next_uint32_bit_value_buffer(datagram, game_id);
    go_to_next_event(uint32_len);
    return true;
}


bool Server_datagram::pack_new_game_to_datagram(char* datagram, const uint32_t& event_no, const uint32_t& maxx, const uint32_t& maxy,
                                                const list<string>& player_names) {
    string name_list = chain_list(player_names);
    int new_game_event_fields_len = length_of_new_game_event_fields(name_list);
    next_free_byte = 0;
    if(!data_will_fit_to_datagram(next_free_byte + new_game_event_fields_len + 2*uint32_len -1)) {
        return false;
    }
    pack_next_uint32_bit_value_buffer(datagram, new_game_event_fields_len);
    pack_next_uint32_bit_value_buffer(datagram, event_no);
    pack_next_int8_bit_value_buffer(datagram, EVENT_TYPE_NEW_GAME);
    pack_next_uint32_bit_value_buffer(datagram, maxx);
    pack_next_uint32_bit_value_buffer(datagram, maxy);
    pack_name_list_buffer(datagram, player_names);
    pack_next_uint32_bit_value_buffer(datagram, checksum_new_game(datagram, new_game_event_fields_len));
    go_to_next_event(new_game_event_fields_len + 2*uint32_len);
    return true;
}

bool Server_datagram::pack_pixel_to_datagram(char* datagram, const uint32_t& event_no, const int8_t& player_number,
                                             const uint32_t&x, const uint32_t&y) {

    next_free_byte = 0;
    if(!data_will_fit_to_datagram(next_free_byte + SIZE_BYTE_OF_PIXEL_EVENT -1)) {
        return false;
    }
    pack_next_uint32_bit_value_buffer(datagram, PIXEL_EVENTS_FIELDS_SIZE);
    pack_next_uint32_bit_value_buffer(datagram, event_no);
    pack_next_int8_bit_value_buffer(datagram, EVENT_TYPE_PIXEL);
    pack_next_int8_bit_value_buffer(datagram, player_number);
    pack_next_uint32_bit_value_buffer(datagram, x);
    pack_next_uint32_bit_value_buffer(datagram, y);
    pack_next_uint32_bit_value_buffer(datagram, checksum_pixel(datagram));//checksum_current_pixel(datagram)
    go_to_next_event(SIZE_BYTE_OF_PIXEL_EVENT);

    return true;
}

bool Server_datagram::pack_player_eliminate(char* datagram, const uint32_t& event_no,
                                            const int8_t& player_number) {
    next_free_byte = 0;
    if(!data_will_fit_to_datagram(next_free_byte + SIZE_BYTE_OF_PLAYER_ELIMINATE_EVENT -1))
        return false;
    pack_next_uint32_bit_value_buffer(datagram, PLAYER_ELIMINATED_EVENTS_FIELDS_SIZE);
    pack_next_uint32_bit_value_buffer(datagram, event_no);
    pack_next_int8_bit_value_buffer(datagram, EVENT_TYPE_PLAYER_ELIMINATED);
    pack_next_int8_bit_value_buffer(datagram, player_number);
    pack_next_uint32_bit_value_buffer(datagram, checksum_player_eliminated(datagram));
    go_to_next_event(SIZE_BYTE_OF_PLAYER_ELIMINATE_EVENT);
    return true;
}


bool Server_datagram::pack_game_over_to_datagram(char* datagram, const uint32_t& event_no) {
    next_free_byte = 0;
    if(!data_will_fit_to_datagram(next_free_byte + SIZE_BYTE_OF_GAME_OVER_EVENT -1))
        return false;
    pack_next_uint32_bit_value_buffer(datagram, GAME_OVER_EVENTS_FIELDS_SIZE);
    pack_next_uint32_bit_value_buffer(datagram, event_no);
    pack_next_int8_bit_value_buffer(datagram, EVENT_TYPE_GAME_OVER);
    pack_next_uint32_bit_value_buffer(datagram, checksum_game_over(datagram));
    go_to_next_event(SIZE_BYTE_OF_GAME_OVER_EVENT);
    return true;
}



uint32_t Server_datagram::checksum_new_game(const char* datagram, const size_t& event_len) {
    memset(crc32_pixel_buffer, 0 , DATAGRAM_SIZE);
    mempcpy(crc32_new_game_buffer, datagram, uint32_len + event_len);
    return compute_checksum(crc32_new_game_buffer, uint32_len + event_len);
}

uint32_t Server_datagram::checksum_pixel(const char* datagram) {
    reset_crc32_pixel_buffer();
    mempcpy(crc32_pixel_buffer, datagram, CRC32_PIXEL_POS);
    return compute_checksum(crc32_pixel_buffer, CRC32_PIXEL_POS);
}

uint32_t Server_datagram::checksum_player_eliminated(const char* datagram) {
    reset_crc32_player_eliminate_buffer();
    mempcpy(crc32_player_eliminate_buffer, datagram, CRC32_PLAYER_ELIMINATE_POS);
    return compute_checksum(crc32_player_eliminate_buffer, CRC32_PLAYER_ELIMINATE_POS);
}

uint32_t Server_datagram::checksum_game_over(const char* datagram) {
    reset_crc32_game_over_buffer();
    mempcpy(crc32_game_over_buffer, datagram, CRC32_GAME_OVER_POS);
    return compute_checksum(crc32_game_over_buffer, CRC32_GAME_OVER_POS);
}
