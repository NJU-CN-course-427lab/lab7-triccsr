#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    uint64_t firstByteIndex;
    if (seg.header().syn==1){
        _synReceived=1;
        _isn=WrappingInt32(seg.header().seqno);
        firstByteIndex=0;
    }
    else{
        firstByteIndex=unwrap(seg.header().seqno,_isn,_reassembler.first_unknown_index())-1;
    }
    _reassembler.push_substring(seg.payload().copy(),firstByteIndex,seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(_synReceived==1)
        return wrap(_reassembler.first_unknown_index()+_reassembler.reassembler_reaches_eof()+1,_isn); 
    return nullopt;
}

size_t TCPReceiver::window_size() const { return _reassembler.window_size(); }
