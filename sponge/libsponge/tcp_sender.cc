#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

#include <string>

#include <iostream>

using namespace std;
// Dummy implementation of a TCP sender

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check_lab4`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _outstandingSize; }


void TCPSender::fill_window() {
    while(1){
        TCPSegment seg;
        size_t maxWindowByte=max((_give0AChance||next_seqno_absolute()==0)?1ul:0ul,get_window_size());
        if(_give0AChance){
            _give0AChance=0;
        }
        size_t maxDataByte=min(TCPConfig::MAX_PAYLOAD_SIZE,maxWindowByte);
        //cerr<<"try send"<<next_seqno()<<" "<<next_seqno_absolute()<<" "<<maxDataByte<<endl;
        if(maxWindowByte==0)
            return;
        size_t actualSendByte=0;
        seg.header().seqno=next_seqno();
        if(next_seqno_absolute()==0){ // syn not sent
            seg.header().syn=1;
            seg.header().ack=0;
            actualSendByte=1;
        }
        else{
            actualSendByte=min(_stream.buffer_size(),maxDataByte);
            Buffer sendBuff{_stream.read(actualSendByte)};
            seg.payload()=sendBuff;
        }
        if(actualSendByte<maxWindowByte && _stream.eof() && !_sendFinished){
            seg.header().fin=1;
            actualSendByte+=1; 
            _sendFinished=1;
        }
        if(actualSendByte==0)
            return;
        //cerr<<"send seg="<<next_seqno()<<" len="<<seg.length_in_sequence_space() ;
        _next_seqno+=actualSendByte;
        _segments_out.push(seg);
        _outstandingSize+=actualSendByte;
        //cerr<<"outstanding"<<_outstandingSize<<endl;
        _outstandingSegments.push(seg);
        if(_timerRunning==false){
            start_timer();
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t absoluteAckno=unwrap(ackno,_isn,_latest_ackno);
    //std::cerr<<"ack_received get"<<absoluteAckno<<" "<<next_seqno_absolute()<<endl;
    if(absoluteAckno>next_seqno_absolute()){// Absolutely wrong ackno
        return ;
    }
    //cerr<<"receive ackno raw="<<ackno.raw_value()<<"abs="<<absoluteAckno<<endl;
    if(absoluteAckno>_latest_ackno||absoluteAckno+window_size>_receiverWindowRightBound){
        while(!_outstandingSegments.empty()&&unwrap(_outstandingSegments.front().header().seqno,_isn,_latest_ackno)+_outstandingSegments.front().length_in_sequence_space() <= absoluteAckno){
            //cerr<<"delete flight absseq="<<unwrap(_outstandingSegments.front().header().seqno,_isn,_latest_ackno)<<" len="<<_outstandingSegments.front().length_in_sequence_space();
            _outstandingSize-=_outstandingSegments.front().length_in_sequence_space();
            //cerr<<"outstanding"<<_outstandingSize<<endl;
            _outstandingSegments.pop();   
        }
//        if(!_outstandingSegments.empty()){
//            size_t receivedLen=absoluteAckno-unwrap(_outstandingSegments.front().header().seqno,_isn,_latest_ackno);
//            if(receivedLen>0){
//                _outstandingSegments.front().payload().remove_prefix(receivedLen);
//                _outstandingSize-=receivedLen;
//            }
//        }
        if(_timerRunning==true){
            if(_outstandingSegments.empty()){
                end_timer();
            }
            else{
                reset_timer();
                start_timer();
            }
        }
        _latest_ackno=absoluteAckno;
        _receiverWindowRightBound=max(_receiverWindowRightBound,absoluteAckno+window_size);
        _announceWindow0=_give0AChance=(window_size==0);

        //std::cerr<<"receive ack"<<_isn.raw_value()<<" "<<_latest_ackno<<" "<<ackno.raw_value()<<_receiverWindowRightBound<<endl;
    }
    return ;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _currentTime+=ms_since_last_tick;
    if(time_up()){
        if(!_outstandingSegments.empty()){
            _segments_out.push(_outstandingSegments.front());
            //cerr<<"resend"<<_outstandingSegments.front().header().seqno<<" "<<_outstandingSegments.front().payload().str()<<" "<<_outstandingSegments.front().length_in_sequence_space()<<endl;
        }
        if(_announceWindow0==false){
            _consecutiveRetransmissions+=1;
        }
        start_timer();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutiveRetransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment emptySeg;
    emptySeg.header().seqno=next_seqno();
    _segments_out.push(emptySeg);
}
