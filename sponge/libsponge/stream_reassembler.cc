#include "stream_reassembler.hh"
#include <assert.h>
#include <iostream>
// Dummy implementation of a stream reassembler.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : 
_output(capacity), _capacity(capacity),_unassembledBytes(0),_firstUnknownIndex(0), _eofReceived(false), _eofIndex(0),_unassembledByteQueue(){
    _unassembledByteQueue.clear();
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const uint64_t index, const bool eof) {
    //cerr<<"reassembler push:"<<data<<",index="<<index<<",eof="<<eof<<endl;
    if(eof==true){
        _eofReceived=true;
        _eofIndex=index+data.size();
    }
    _unassembledByteQueue.resize(window_size(),make_pair(0,false));
    const uint64_t endIndex=min(index+data.size(),_firstUnknownIndex+_unassembledByteQueue.size());
    //const bool dataEndDropped=(endIndex<(index+data.size()));
    for(uint64_t i=max(index,_firstUnknownIndex);i<endIndex;++i){
        if(_unassembledByteQueue[i-_firstUnknownIndex].second==false){
            _unassembledByteQueue[i-_firstUnknownIndex]=make_pair(data[i-index],true);
            ++_unassembledBytes;   
        }
    }
    string writeString;
    while(_unassembledByteQueue.empty()==false&&_unassembledByteQueue.front().second==true){
        writeString.push_back(_unassembledByteQueue.front().first);
        ++_firstUnknownIndex;
        --_unassembledBytes;
        _unassembledByteQueue.pop_front();
    }
    if(!writeString.empty()){
        _output.write(writeString);
    }
    if(reassembler_reaches_eof()){
        _output.end_input();
    }
    //if(eof==true&&dataEndDropped==false){
    //    _eofInQueue=true;
    //}
    //if(_eofInQueue==true && _unassembledBytes==0 ){
    //    _output.end_input();
    //}
    //cerr<<"push ended:firstUnknownIndex="<<_firstUnknownIndex<<endl;
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembledBytes; }

bool StreamReassembler::empty() const { return _unassembledBytes==0; }

uint64_t StreamReassembler::first_unknown_index()const{return _firstUnknownIndex;}

size_t StreamReassembler::window_size()const{return _capacity-_output.buffer_size();}

bool StreamReassembler::reassembler_reaches_eof()const{return _eofReceived&&_firstUnknownIndex==_eofIndex;}