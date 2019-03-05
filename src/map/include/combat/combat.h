#pragma once

#include <utility>
#include "dataconsts.h"
#include "cli_attack.h"
#include "cli_hp_req.h"
#include "cli_revive_req.h"

class EntitySystem;

namespace Combat {
  void attack(EntitySystem&, RoseCommon::Entity, const RoseCommon::Packet::CliAttack&);
  void hp_request(EntitySystem&, RoseCommon::Entity, const RoseCommon::Packet::CliHpReq&);
  void revive(EntitySystem&, RoseCommon::Entity, const RoseCommon::Packet::CliReviveReq&);
  
  void update(EntitySystem&, RoseCommon::Entity);
  
  std::pair<float, float> get_range_position(const EntitySystem& entitySystem, RoseCommon::Entity character, RoseCommon::Entity target, float range = 1);
  float get_range_to(const EntitySystem& entitySystem, RoseCommon::Entity character, RoseCommon::Entity target);
  
  RoseCommon::Entity get_closest_spawn(EntitySystem& entitySystem, RoseCommon::Entity player);
  RoseCommon::Entity get_saved_spawn(EntitySystem& entitySystem, RoseCommon::Entity player);
  RoseCommon::Entity get_start_spawn(EntitySystem& entitySystem);
}
