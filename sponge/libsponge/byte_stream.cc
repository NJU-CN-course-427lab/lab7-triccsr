#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity):
_capacity(capacity),_bytesRead(0),_bytesWritten(0),_byteQueue(),_inputHasEnded(false){
    _byteQueue.clear();
}

size_t ByteStream::write(const string &data) {
    size_t writeSize=min(data.size(),_capacity-_byteQueue.size());
    for(size_t i=0;i<writeSize;++i){
        _byteQueue.push_back(data[i]);
    }
    _bytesWritten+=writeSize;
    return writeSize;
}
//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string stringPeek;
    const size_t peekLen=min(len,_byteQueue.size());
    stringPeek.append(_byteQueue.begin(),_byteQueue.begin()+peekLen);
    return stringPeek;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    const size_t popLen=min(len,_byteQueue.size());
    _byteQueue.erase(_byteQueue.begin(),_byteQueue.begin()+popLen); 
    _bytesRead+=popLen;
}
//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string stringRead;    
    const size_t readLen=min(len,_byteQueue.size());
    stringRead.append(_byteQueue.begin(),_byteQueue.begin()+readLen);
    _byteQueue.erase(_byteQueue.begin(),_byteQueue.begin()+readLen);
    _bytesRead+=readLen;
    return stringRead;
}

void ByteStream::end_input() {_inputHasEnded=true;}

bool ByteStream::input_ended() const { return _inputHasEnded; }

size_t ByteStream::buffer_size() const { return _byteQueue.size(); }

bool ByteStream::buffer_empty() const { return _byteQueue.empty(); }

bool ByteStream::eof() const { return _inputHasEnded&&_byteQueue.empty(); }

size_t ByteStream::bytes_written() const { return _bytesWritten; }

size_t ByteStream::bytes_read() const { return _bytesRead; }

size_t ByteStream::remaining_capacity() const { return _capacity-_byteQueue.size(); }