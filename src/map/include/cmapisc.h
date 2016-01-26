#ifndef __CMAPISC_H__
#define __CMAPISC_H__

#include "croseisc.h"

class CMapISC : public CRoseISC
{
public:
        CMapISC( );
        CMapISC( tcp::socket _sock );

protected:
        bool HandlePacket( uint8_t* _buffer );
	virtual void OnConnected( );
};

#endif
