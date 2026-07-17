#include "fanzy_protocol.h"

fanzy_proto_packet_t fanzy_protocol_init_packet(void)
{
    return (fanzy_proto_packet_t){
        .sof = 0xAA,
        .msg_id = FANZY_PROTO_MSG_INIT,
        .length = 0,
    };
}
