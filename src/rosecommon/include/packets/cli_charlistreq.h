#pragma once

#include "packetfactory.h"
#include "entitycomponents.h"


namespace RoseCommon {

REGISTER_RECV_PACKET(ePacketType::PAKCS_CHAR_LIST_REQ, CliCharListReq)
REGISTER_SEND_PACKET(ePacketType::PAKCS_CHAR_LIST_REQ, CliCharListReq)
class CliCharListReq : public CRosePacket {
	public:
		CliCharListReq();
		CliCharListReq(CRoseReader reader);
	public:

		virtual ~CliCharListReq() = default;


		static CliCharListReq create();
		static CliCharListReq create(uint8_t *buffer);

	protected:
		virtual void pack(CRoseWriter&) const override;
		virtual uint16_t get_size() const override;
};

}