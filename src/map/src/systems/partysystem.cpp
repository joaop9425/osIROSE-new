#include "systems/partysystem.h"
#include "cli_partyreply.h"
#include "cli_partyreq.h"
#include "cmapclient.h"
#include "makevector.h"
#include "srv_partymember.h"
#include "srv_partyreply.h"
#include "srv_partyreq.h"

using namespace Systems;
using namespace RoseCommon;

PartySystem::PartySystem(SystemManager &m) : System(m) {
  m.registerDispatcher(ePacketType::PAKCS_PARTY_REQ, &PartySystem::processPartyReq);
  m.registerDispatcher(ePacketType::PAKCS_PARTY_REPLY, &PartySystem::processPartyReply);
  m.getEntityManager().on_component_removed<Party>([this](Entity entity, Party *Cparty) {
    this->logger_->trace("deleting the party for client {}", getId(entity));
    auto party = Cparty->party_;
    if (!party) return;
    if (!party->removeMember(entity)) return;
    Entity starter = entity;
    if (Cparty->isKicked_) starter = party->leader_;
    if (auto socket = getClient(entity))
      socket->send(SrvPartyReply::create(PartyReply::DESTROY, starter));
    if (party->members_.size() == 1) {
      party->members_[0].remove<Party>();
      return;
    }
    uint32_t leaver = entity.component<BasicInfo>()->tag_;
    uint32_t leader = party->leader_.component<BasicInfo>()->tag_;
    for (auto &it : party->members_) {
      if (auto socket = getClient(it))
        socket->send(SrvPartyMember::create(party->options_, PartyMember::Party(leaver, leader)));
    }
  });
  // TODO : use on_component_assign for new members?
}

void PartySystem::update(EntityManager &es, std::chrono::milliseconds) {
  for (Entity entity : es.entities_with_components<BasicInfo, Party>()) {
    Party *party = entity.component<Party>();
    if (!party->party_) {
      BasicInfo *info = entity.component<BasicInfo>();
      logger_->trace("Client {} has the Party component but no affiliated party. {}", info->id_, party->isRequested_);
      if (!party->isRequested_) entity.remove<Party>();
    }
  }
}

void PartySystem::addPartyMember(Entity leader, Entity newMember) {
  auto Cparty = leader.component<Party>();
  if (!Cparty) return;
  if (newMember.component<Party>()) return;
  std::shared_ptr<PartyBase> party = Cparty->party_;
  if (!party) party = Cparty->party_ = std::make_shared<PartyBase>(leader);
  newMember.assign<Party>(party);
  for (auto &it : party->members_) {
    if (auto socket = getClient(it))
      socket->send(SrvPartyMember::create(party->options_, PartyMember::Party(newMember)));
  }
  if (auto socket = getClient(newMember))
    socket->send(SrvPartyMember::create(party->options_, PartyMember::Party(party->members_)));
  party->addMember(newMember);
}

void PartySystem::changeLeader(Entity leader, Entity newLeader) {
  auto Cparty = leader.component<Party>();
  if (!Cparty) return;
  std::shared_ptr<PartyBase> party = Cparty->party_;
  if (!party) return;
  if (!party->changeLeader(newLeader)) return;
  for (auto it : party->members_) {
    if (auto socket = getClient(it))
      socket->send(SrvPartyReply::create(PartyReply::CHANGE_OWNER, newLeader));
  }
}

void PartySystem::processPartyReq(CMapClient &client, Entity entity, const CliPartyReq &packet) {
  logger_->trace("PartySystem::processPartyReq");
  Entity other;
  if (!(other = manager_.getEntity(packet.idXorTag())) || !getClient(other)) {
    logger_->debug("Client {} requested a party with the non existing char {}", getId(entity), packet.idXorTag());
    SrvPartyReply sendPacket;
    sendPacket.set_reply(PartyReply::NOT_FOUND).set_id(packet.idXorTag());
    client.send(std::move(sendPacket));
    return;
  }
  Party *party = nullptr;
  switch (packet.request()) {
    case PartyReq::MAKE:
      if (entity.component<Party>() && entity.component<Party>()->party_) {
        logger_->warn("Client {} tried to create a party when already in a party", getId(entity));
        return;
      }
      party = entity.assign<Party>();
      party->isRequested_ = true;
      {
        if (auto socket = getClient(other))
          socket->send(SrvPartyReq::create(static_cast<PartyReq::Request>(packet.request()), entity));
      }
      return;
    case PartyReq::JOIN:
      if (!entity.component<Party>() || !entity.component<Party>()->party_) {
        logger_->warn("Client {} tried to execute an action on its party but doesn't have a party yet", getId(entity));
        return;
      }
      party = entity.component<Party>();
      party->isRequested_ = true;
      {
        if (auto socket = getClient(other))
          socket->send(SrvPartyReq::create(static_cast<PartyReq::Request>(packet.request()), entity));
      }
      return;
    case PartyReq::LEFT:
      if (!entity.component<Party>()) {
        logger_->warn("Client {} tried to leave the party but isn't in one", getId(entity));
        return;
      }
      entity.remove<Party>();
      return;
    case PartyReq::CHANGE_OWNER:
      if (!entity.component<Party>() || !entity.component<Party>()->party_) {
        logger_->warn("Client {} tried to give up ownership but isn't in a party", getId(entity));
        return;
      } else if (entity.component<Party>()->party_->leader_ != entity) {
        logger_->warn("Client {} tried to give up ownership but doesn't have it", getId(entity));
        return;
      }
      changeLeader(entity, other);
      return;
    case PartyReq::KICK:
      if (!entity.component<Party>() || !entity.component<Party>()->party_) {
        logger_->warn("Client {} tried to kick a member party but isn't in one", getId(entity));
        return;
      } else if (!entity.component<Party>()->party_->isMember(other)) {
        logger_->warn("Client {} tried to kick a member that isn't in its party", getId(entity));
        return;
      }
      other.component<Party>()->isKicked_ = true;
      other.remove<Party>();
      return;
    default:
      logger_->warn("Client {} sent a non valid request code {}", getId(entity), packet.request());
  }
}

void PartySystem::processPartyReply(CMapClient &, Entity entity, const RoseCommon::CliPartyReply &packet) {
  logger_->trace("PartySystem::processPartyRequest");
  Entity other;
  if (!(other = manager_.getEntity(packet.idXorTag())) || !getClient(other)) {
    logger_->warn("Client {} replied to a party request of the non existing char {}", getId(entity), packet.idXorTag());
    return;
  }
  if (!other.component<Party>() || !other.component<Party>()->isRequested_) {
    logger_->warn("Client {} replied to a non existing party request", getId(entity));
    return;
  }
  switch (packet.reply()) {
    // if the guy asked is busy or declined the invitation, we send it to the guy asking
    case PartyReply::BUSY:
    case PartyReply::REJECT:
      other.component<Party>()->isRequested_ = false;
      if (!other.component<Party>()->party_) other.remove<Party>();
      {
        if (auto socket = getClient(other))
          socket->send(SrvPartyReply::create(static_cast<PartyReply::Reply>(packet.reply()), entity));
      }
      return;
    // if the guy asked accepts, we send it to the guy asking and we update the party
    case PartyReply::ACCEPT_MAKE:
    case PartyReply::ACCEPT_JOIN:
      other.component<Party>()->isRequested_ = false;
      {
        if (auto socket = getClient(other))
          socket->send(SrvPartyReply::create(static_cast<PartyReply::Reply>(packet.reply()), entity));
      }
      addPartyMember(other, entity);
      return;
    default:
      logger_->warn("Client {} sent a party reply with reply {}", getId(entity), (int)packet.reply());
  }
}
