#include "ccharisc.h"
#include "crosepacket.h"

CCharISC::CCharISC() : CRoseISC()
{
        m_Log.SetIdentity( "CCharISC" );
}

CCharISC::CCharISC( tcp::socket _sock ) : CRoseISC( std::move( _sock ) )
{
        m_Log.SetIdentity( "CCharISC" );
}

bool CCharISC::HandlePacket( uint8_t* _buffer )
{
        CPacket* pak = (CPacket*)_buffer;
        switch ( pak->Header.Command )
        {
	case ePacketType::ISC_ALIVE: return true;
	case ePacketType::ISC_SERVER_AUTH: return true;
	case ePacketType::ISC_SERVER_REGISTER: return true;
	case ePacketType::ISC_TRANSFER: return true;
	case ePacketType::ISC_SHUTDOWN: return true;
        default:
        {
                CRoseISC::HandlePacket( _buffer );
                return false;
        }
        }
        return true;
}

void CCharISC::OnConnected()
{
	CRosePacket* pak = new CRosePacket(ePacketType::ISC_SERVER_REGISTER);

	ServerReg pServerReg;
	pServerReg.set_name("HardCoded");
	pServerReg.set_addr("127.0.0.1");
	pServerReg.set_port(29100);
	pServerReg.set_type(ServerReg_ServerType_CHAR);

	int _size = pServerReg.ByteSize(); 
	uint8_t* data = new uint8_t[_size];
	memset( data, 0, _size );
	if( pServerReg.SerializeToArray(data, _size) == false )
		m_Log.eicprintf( "Couldn't serialize the data\n" );
	pak->AddBytes(data, _size);

	m_Log.icprintf( "IN 0x%X ", pak->Header.Command );
	for (int i = 0; i < _size; i++)
		m_Log.dcprintf( "%02X ", pak->Data[i] );
	m_Log.dcprintf( "\n" );

	m_Log.icprintf("Header[%i, 0x%X] Size: %i\n", pak->Header.Size, pak->Header.Command, _size);

	m_Log.oicprintf( "Sent a packet on CRoseISC: Header[%i, 0x%X]\n", pak->Header.Size, pak->Header.Command );

	Send( (CPacket*)pak );
	delete[] data;
}
