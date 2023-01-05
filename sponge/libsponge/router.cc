#include "router.hh"

#include <iostream>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 7, please replace with a real implementation that passes the
// automated checks run by `make check_lab7`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    // Your code here.
    _routes.push_back({route_prefix,prefix_length,next_hop,interface_num});
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    // Your code here.
    Route chosen={};
    int maxPrefix=-1;
    uint32_t dstIP=dgram.header().dst;
    for(const Route &route:_routes){
        //cerr<<(15ul>>32u)<<" "<<(0ul>>5u)<<" "<<(1ul>>3u)<<" "<<(0ul>>32u)<<" "<<(15ul>>31u)<<endl;
        //cerr<<"pre"<<32u-route.prefix_length<<" "<<dstIP<<" "<<route.route_prefix<<" "<<(dstIP>>(32u-route.prefix_length))<<" "<<(route.route_prefix>>(32u-route.prefix_length))<<endl;
        if((route.prefix_length==0||(dstIP>>(32ul-route.prefix_length))==(route.route_prefix>>(32ul-route.prefix_length)))&&route.prefix_length>maxPrefix){
            chosen=route;
            maxPrefix=route.prefix_length;
        }
    }
    //cerr<<"dgram: "<<dgram.header().ttl<<" chosen: "<<chosen.route_prefix<<" "<<chosen.interface_num<<endl;
    if(maxPrefix>=0 && dgram.header().ttl>1){
        dgram.header().ttl-=1;
        _interfaces[chosen.interface_num].send_datagram(dgram,(chosen.next_hop.has_value())?(chosen.next_hop.value()):(Address::from_ipv4_numeric(dstIP)));
    }
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
