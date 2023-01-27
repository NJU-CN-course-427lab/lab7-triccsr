#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

void NetworkInterface::get_EthernetFrame(const InternetDatagram &dgram,const EthernetAddress &dst,EthernetFrame &frame){
    frame.header()={dst,_ethernet_address,EthernetHeader::TYPE_IPv4};
    frame.payload()=dgram.serialize();
}
void NetworkInterface::new_arp_map(uint32_t ip,const EthernetAddress &ethernetAddress){
    _aRPCacheMap[ip]=ethernetAddress;
    _aRPDeleteQueue.push(make_pair(ip,_msTime));
    if(_ipToBeSent.count(ip)){
        int len=_ipToBeSent[ip].size();
        for(int i=0;i<len;++i){
            _ipToBeSent[ip][i].header().dst=ethernetAddress;
            _frames_out.push(_ipToBeSent[ip][i]);
        }
        _ipToBeSent.erase(ip);
    }
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    if(_aRPCacheMap.count(next_hop_ip)>0){
        EthernetFrame ethernetFrame;
        get_EthernetFrame(dgram,_aRPCacheMap.at(next_hop_ip),ethernetFrame);
        _frames_out.push(ethernetFrame);
    }
    else if(_ipQuerySet.count(next_hop_ip)==0){
        EthernetFrame broadcast;
        ARPMessage arpMessage;
        arpMessage.opcode=ARPMessage::OPCODE_REQUEST;
        arpMessage.sender_ethernet_address=_ethernet_address;
        arpMessage.sender_ip_address=_ip_address.ipv4_numeric();
        arpMessage.target_ip_address=next_hop_ip;
        broadcast.header()={ETHERNET_BROADCAST,_ethernet_address,EthernetHeader::TYPE_ARP};
        broadcast.payload()=arpMessage.serialize();
        _frames_out.push(broadcast);
        _ipQuerySet.insert(next_hop_ip);
        _ipQueryDeleteQueue.push(make_pair(next_hop_ip,_msTime));
        EthernetFrame queueFrame;
        get_EthernetFrame(dgram,ETHERNET_BROADCAST,queueFrame);
        if(_ipToBeSent.count(next_hop_ip)==0){
            _ipToBeSent.insert(make_pair(next_hop_ip,vector<EthernetFrame> ()));
        }
        _ipToBeSent[next_hop_ip].push_back(queueFrame);
    }
    //DUMMY_CODE(dgram, next_hop, next_hop_ip);
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    //DUMMY_CODE(frame);
    if(frame.header().dst!=ETHERNET_BROADCAST&&frame.header().dst!=_ethernet_address){
        return nullopt;
    }
    if(frame.header().type==EthernetHeader::TYPE_IPv4){
        InternetDatagram dgram;
        if(dgram.parse(frame.payload())==ParseResult::NoError){
            return dgram;
        }
    }
    else if(frame.header().type==EthernetHeader::TYPE_ARP){
        ARPMessage aRPMessage;
        if(aRPMessage.parse(frame.payload())!=ParseResult::NoError){
            return nullopt;
        }
        if(aRPMessage.opcode==ARPMessage::OPCODE_REQUEST&&aRPMessage.target_ip_address==_ip_address.ipv4_numeric()){
            ARPMessage sendARP;
            sendARP.opcode=sendARP.OPCODE_REPLY;
            sendARP.sender_ethernet_address=_ethernet_address;
            sendARP.sender_ip_address=_ip_address.ipv4_numeric();
            sendARP.target_ethernet_address=aRPMessage.sender_ethernet_address;
            sendARP.target_ip_address=aRPMessage.sender_ip_address;
            EthernetFrame reply;
            reply.header()={aRPMessage.sender_ethernet_address,_ethernet_address,EthernetHeader::TYPE_ARP};
            reply.payload()=sendARP.serialize();
            _frames_out.push(reply);
            new_arp_map(aRPMessage.sender_ip_address,aRPMessage.sender_ethernet_address);
        }
        else if(aRPMessage.opcode==ARPMessage::OPCODE_REPLY){
            new_arp_map(aRPMessage.sender_ip_address,aRPMessage.sender_ethernet_address);
        }
    }
    return nullopt;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    _msTime+=ms_since_last_tick;
    while(!_aRPDeleteQueue.empty()&&_aRPDeleteQueue.front().second+CACHE_TIME<=_msTime){
        if(_aRPCacheMap.count(_aRPDeleteQueue.front().first)){
            _aRPCacheMap.erase(_aRPDeleteQueue.front().first);
        }
        _aRPDeleteQueue.pop();
    }
    while(!_ipQueryDeleteQueue.empty()&&_ipQueryDeleteQueue.front().second+QUERY_TIME<=_msTime){
        int ip=_ipQueryDeleteQueue.front().first;
        if(_ipQuerySet.count(ip)){
            _ipQuerySet.erase(ip);
        }
        _ipQueryDeleteQueue.pop();
    }
}
