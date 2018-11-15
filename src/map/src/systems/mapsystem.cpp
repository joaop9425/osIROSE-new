#include "cli_changemapreq.h"
#include "srv_changemapreply.h"
#include "srv_dropitem.h"
#include "srv_playerchar.h"
#include "srv_npcchar.h"
#include "srv_mobchar.h"
#include "cmapclient.h"
#include "cmapserver.h"
#include "systems/chatsystem.h"
#include "systems/mapsystem.h"
#include "systems/movementsystem.h"

using namespace Systems;
using namespace RoseCommon;

MapSystem::MapSystem(SystemManager &m) : System(m) {
  m.registerDispatcher(ePacketType::PAKCS_CHANGE_MAP_REQ, &MapSystem::processChangeMapReq);
}

void MapSystem::update(EntityManager &, std::chrono::milliseconds) {}

void MapSystem::processChangeMapReq(CMapClient &client, Entity entity, const RoseCommon::CliChangeMapReq &) {
  logger_->trace("MapSystem::processChangeMapReq");

  auto advanced = entity.component<AdvancedInfo>();
  auto basic = entity.component<BasicInfo>();
  auto info = entity.component<CharacterInfo>();

  client.send(SrvChangeMapReply::create(getId(entity), advanced->hp_, advanced->mp_, basic->xp_,
                                                               info->penaltyXp_, std::time(nullptr), 0));
  manager_.get<Systems::ChatSystem>()->sendMsg(entity, "You are client " + std::to_string(getId(entity)));
  manager_.send(entity, CMapServer::eSendType::NEARBY_BUT_ME,
                                         SrvPlayerChar::create(entity));
  const auto movement = manager_.get<Systems::MovementSystem>();
  auto &manager = manager_.getEntityManager();
  for (Entity e : manager.entities_with_components<BasicInfo, AdvancedInfo>()) {
    if (!movement->is_nearby(entity, e)) continue; // do not send non nearby stuff
    basic = e.component<BasicInfo>();
    if (basic->ownerId_ == 0 && e.component<Npc>()) {
        client.send(SrvNpcChar::create(e));
    } else if (e.component<Npc>()) {
        client.send(SrvMobChar::create(e));
    } else {
        if (e != entity && basic->isOnMap_.load())
            client.send(SrvPlayerChar::create(e));
    }
  }
  for (Entity e : manager.entities_with_components<Item>()) {
    if (!movement->is_nearby(entity, e)) continue; // do not send non nearby stuff
    client.send(SrvDropItem::create(e));
  }
  entity.component<BasicInfo>()->isOnMap_.store(true);
}
