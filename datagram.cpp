#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <endian.h>
#include <iostream>
#include <string>
#include "datagram.h"
#include <boost/crc.hpp>



Datagram::Datagram() {
    next_free_byte = 0;
    recv_length = 300;
    current_event_start = 0;
};


void Datagram::clear_datagram(char * datagram) {
    memset(datagram, 0, DATAGRAM_SIZE);
    current_event_start = 0;
    next_free_byte = 0;
    recv_length = 0;
}


void Datagram::pack_next_int8_bit_value_buffer(char* datagram, int8_t v) {
    datagram[next_free_byte] = ((uint8_t*)&v)[0];
    next_free_byte++;
}

void Datagram::pack_next_uint32_bit_value_buffer(char* datagram, uint32_t v) {
    uint32_t net_order = htobe32(v);
    for (int i=0; i<4 ;++i) {
        datagram[next_free_byte] = ((uint8_t*)&net_order)[i];
        next_free_byte++;
    }
}

void Datagram::pack_next_uint64_bit_value_buffer(char* datagram, uint64_t v) {
    uint64_t net_order = htobe64(v);
    for (int i=0; i<8 ;++i) {
        datagram[next_free_byte] = ((uint8_t*)&net_order)[i];
        next_free_byte++;
    }
}

void Datagram::pack_next_string_buffer(char* datagram, string s) {
    //mempcpy(datagram_buffer+next_free_byte, s.c_str(), s.length());
    for(unsigned int i = 0; i < s.length(); i++) {
        datagram[next_free_byte] = (char)s[i];
        next_free_byte++;
    }
}

void Datagram::pack_name_list_buffer(char *datagram, const list<string>& l) {
    for(const string& name: l) {
        pack_next_string_buffer(datagram, name);
        pack_char_to_datagram(datagram, '\0');
    }
}



void Datagram::get_int8_bit_value_fbuffer(const char* datagram, int8_t& v, int start_in_datagram) {
    uint8_t a[1];
    mempcpy(a, datagram + start_in_datagram, sizeof(uint8_t));
    v = *((uint8_t*) a);
}

void Datagram::get_int32_bit_value_fbuffer(const char* datagram, uint32_t& v, int start_in_datagram) {
    uint8_t a[4];
    mempcpy(a, datagram + start_in_datagram, sizeof(uint32_t));
    //v = *((uint32_t*) a); //OLD
    for(int i = 0; i < 4; i++) {
        ((uint8_t*)&v)[i] = ((uint8_t*) a)[i];
    }
    v = be32toh(v);
}

void Datagram::get_int64_bit_value_fbuffer(const char* datagram, uint64_t& v, int start_in_datagram) {
    uint8_t a[8];
    mempcpy(a, datagram + start_in_datagram, sizeof(uint64_t));
    //v = *((uint64_t*) a);
    for(int i = 0; i < 8; i++) {
        ((uint8_t*)&v)[i] = ((uint8_t*) a)[i];
    }
    v = be64toh(v);
}

void Datagram::get_string_fbuffer(const char* datagram, string& s, int start_in_datagram, int end_in_datagram) {
    size_t s_len = size_t (end_in_datagram - start_in_datagram + 1);
    char recv[s_len+1];//+1
    mempcpy(recv, datagram + start_in_datagram, s_len);
    recv[s_len] = '\0';
    s = string(recv);
}


bool Datagram::compare_checksums_crc32(const string &recv_fields, uint32_t recv_checksum) {
    boost::crc_32_type result;
    result.process_bytes(recv_fields.data(), recv_fields.length());
    return result.checksum() == recv_checksum;
}


uint32_t Datagram::compute_checksum(const char* buff, const int& len) {
    boost::crc_32_type result;
    result.process_bytes(buff, len);
    return (uint32_t)result.checksum();
}



bool Datagram::fields_fit_in_datagram(const int &last_byte_of_event) {
    return recv_length-1 >= last_byte_of_event;
}



void Datagram::copy_names_to_players_names_buffer(const char *datagram) {
    reset_players_names_buffer();
    mempcpy(players_names_buffer, datagram + current_event_start + EVENT_DATA_POS + 2*uint32_len, event_len - int8_len - 3*uint32_len);
    players_names_buffer_length = event_len - int8_len - 3*uint32_len;
}

bool Datagram::give_player_list(list<string>& player_names) {
    string t = "";
    for(int i = 0; i < players_names_buffer_length; i++) {
        if(MIN_VALID_PLAYER_NAME_SYMBOL <= players_names_buffer[i]
           && players_names_buffer[i] <= MAX_VALID_PLAYER_NAME_SYMBOL) {
            t += players_names_buffer[i];
        }
        else if(players_names_buffer[i] == '\0') {
            if(t == "")
                return false;
            player_names.push_back(t);
            t = "";
        }
        else
            return false;
    }
    return players_names_buffer[players_names_buffer_length-1] == '\0';
}



bool Datagram::event_checksum_correct(const char* datagram, size_t len_field) {
    if(!fields_fit_in_datagram(current_event_start + 2*uint32_len + len_field -1)) {
        return false;
    }
    boost::crc_32_type result;
    result.process_bytes(datagram + current_event_start ,len_field + uint32_len);
    get_int32_bit_value_fbuffer(datagram, len_crc32, current_event_start + len_field + uint32_len);
    return len_crc32 == result.checksum();
}


void Datagram::go_to_next_event(const int& full_event_length) {
    current_event_start += full_event_length;
}



uint32_t Datagram::checksum_current_new_game(const char* datagram, const size_t& event_len) {
    mempcpy(crc32_new_game_buffer, datagram + current_event_start, uint32_len + event_len);
    return compute_checksum(crc32_new_game_buffer, uint32_len + event_len);
}

uint32_t Datagram::checksum_current_pixel(const char* datagram) {
    reset_crc32_pixel_buffer();
    mempcpy(crc32_pixel_buffer, datagram + current_event_start, CRC32_PIXEL_POS);
    return compute_checksum(crc32_pixel_buffer, CRC32_PIXEL_POS);
}

uint32_t Datagram::checksum_current_player_eliminated(const char* datagram) {
    reset_crc32_player_eliminate_buffer();
    mempcpy(crc32_player_eliminate_buffer, datagram + current_event_start, CRC32_PLAYER_ELIMINATE_POS);
    return compute_checksum(crc32_player_eliminate_buffer, CRC32_PLAYER_ELIMINATE_POS);
}

uint32_t Datagram::checksum_current_game_over(const char* datagram) {
    reset_crc32_game_over_buffer();
    mempcpy(crc32_game_over_buffer, datagram + current_event_start, CRC32_GAME_OVER_POS);
    return compute_checksum(crc32_game_over_buffer, CRC32_GAME_OVER_POS);
}

string Datagram::chain_list(const list<string>& player_names) {
    string s = "";
    for(const string& name: player_names) {
        s += name + " ";
    }
    return s;
}



