#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity();}

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _timeSinceLastSegmentReceived; }

void TCPConnection::unclean_shutdown(){//unclean shutdown
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _active=false;
}

void TCPConnection::send_rst(){
    send_from_sender(1,1);
    unclean_shutdown();
}

void TCPConnection::clean_shutdown_if_necessary(){
    if(!active()){
        return;
    }
    if (inbound_stream().input_ended()&&_sender.stream_in().eof()&&_sender.bytes_in_flight()==0){
        if((_linger_after_streams_finish==true&&time_since_last_segment_received()>=10*_cfg.rt_timeout)||_linger_after_streams_finish==false){
            send_from_sender(0,0);
            _active=false;
        }
    }
}

void TCPConnection::segment_received(const TCPSegment &seg) {
    if(!active()){
        return;
    }
    _timeSinceLastSegmentReceived=0;
    if(seg.header().rst==true){
        unclean_shutdown();
        return;
    }
    _receiver.segment_received(seg);
    if(seg.header().ack==true){
        if(_sender.next_seqno_absolute()==0){
            return;
        }
        _sender.ack_received(seg.header().ackno,seg.header().win);
    }
    if(seg.length_in_sequence_space()>0){//occupy some seq number
        send_from_sender(0,1);
    }
    else{
        send_from_sender(0,0);
    }
    if(inbound_stream().input_ended() && !_sender.stream_in().eof()){
        _linger_after_streams_finish=false;
    }
}

bool TCPConnection::active() const { return _active; }

void TCPConnection::send_from_sender(bool rst,bool mustSend){
    _sender.fill_window();
    if(mustSend && _sender.segments_out().empty()){
        _sender.send_empty_segment();
    }
    while(!_sender.segments_out().empty()){
        TCPSegment tcpSeg=_sender.segments_out().front();
        _sender.segments_out().pop();
        if(_receiver.ackno()!=nullopt){
            tcpSeg.header().ackno=_receiver.ackno().value();
            tcpSeg.header().ack=1;
            tcpSeg.header().win=min(_receiver.window_size(),0xfffful);
        }
        else{
            tcpSeg.header().ack=0;
        }
        tcpSeg.header().rst=rst;
        _segments_out.push(tcpSeg);
    }
}

size_t TCPConnection::write(const string &data) {
    if(!active()){
        return 0;
    }
    size_t actualWriteBytes=_sender.stream_in().write(data);
    send_from_sender(0,0);
    return actualWriteBytes;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _timeSinceLastSegmentReceived+=ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    if(_sender.consecutive_retransmissions()>_cfg.MAX_RETX_ATTEMPTS){
        send_rst();
        return;
    }
    if(!_sender.segments_out().empty()){
        send_from_sender(0,0);
    }
    clean_shutdown_if_necessary();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    send_from_sender(0,1);
}

void TCPConnection::connect() {
    send_from_sender(0,1);
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            send_rst();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
