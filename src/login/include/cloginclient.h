// Copyright 2016 Chirstopher Torres (Raven), L3nn0x
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _CLOGINCLIENT_H_
#define _CLOGINCLIENT_H_

#include "croseclient.h"
#include "cli_loginreq.h"
#include "cli_channellistreq.h"
#include "cli_srvselectreq.h"
#include "dataconsts.h"

class CLoginServer;

class CLoginClient : public RoseCommon::CRoseClient {
 public:
  CLoginClient();
  CLoginClient(CLoginServer* server, std::unique_ptr<Core::INetwork> _sock);

 protected:
  virtual bool handlePacket(uint8_t* _buffer) override;

  // Packet Helper Functions
  bool userLogin(RoseCommon::CliLoginReq&& P);
  bool channelList(RoseCommon::CliChannelListReq&& P);
  bool serverSelect(RoseCommon::CliSrvSelectReq&& P);

  void sendLoginReply(LoginReply::eResult Result);

  virtual void onDisconnected() override;

  enum class eSTATE {
    DEFAULT,
    LOGGEDIN,
    TRANSFERING,
  };

  uint16_t access_rights_;
  std::string username_;
  eSTATE login_state_;
  uint32_t userid_;
  uint32_t session_id_;
  CLoginServer *server_;
};

#endif
