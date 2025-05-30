/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "CombatAI.h"
#include "CreatureAIImpl.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "Log.h"
#include "MotionMaster.h"
#include "MoveSplineInit.h"
#include "ObjectAccessor.h"
#include "PassiveAI.h"
#include "Player.h"
#include "ScriptedEscortAI.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"
#include "SpellInfo.h"
#include "Vehicle.h"
#include <G3D/Vector3.h>

/*######
##Quest 12848
######*/

namespace TheScarletEnclave::Chapter1
{
#define GCD_CAST    1

enum UnworthyInitiate
{
    SPELL_SOUL_PRISON_CHAIN         = 54612,
    SPELL_DK_INITIATE_VISUAL        = 51519,

    SPELL_ICY_TOUCH                 = 52372,
    SPELL_PLAGUE_STRIKE             = 52373,
    SPELL_BLOOD_STRIKE              = 52374,
    SPELL_DEATH_COIL                = 52375,

    SAY_EVENT_START                 = 0,
    SAY_EVENT_ATTACK                = 1,

    EVENT_ICY_TOUCH                 = 1,
    EVENT_PLAGUE_STRIKE             = 2,
    EVENT_BLOOD_STRIKE              = 3,
    EVENT_DEATH_COIL                = 4
};

enum UnworthyInitiatePhase
{
    PHASE_CHAINED,
    PHASE_TO_EQUIP,
    PHASE_EQUIPING,
    PHASE_TO_ATTACK,
    PHASE_ATTACKING,
};

uint32 acherus_soul_prison[12] =
{
    191577,
    191580,
    191581,
    191582,
    191583,
    191584,
    191585,
    191586,
    191587,
    191588,
    191589,
    191590
};

uint32 acherus_unworthy_initiate[5] =
{
    29519,
    29520,
    29565,
    29566,
    29567
};

class npc_unworthy_initiate : public CreatureScript
{
public:
    npc_unworthy_initiate() : CreatureScript("npc_unworthy_initiate") { }

    struct npc_unworthy_initiateAI : public ScriptedAI
    {
        npc_unworthy_initiateAI(Creature* creature) : ScriptedAI(creature)
        {
            Initialize();
            me->SetReactState(REACT_PASSIVE);
            if (!me->GetCurrentEquipmentId())
                me->SetCurrentEquipmentId(me->GetOriginalEquipmentId());

            wait_timer = 0;
            anchorX = 0.f;
            anchorY = 0.f;
        }

        void Initialize()
        {
            anchorGUID.Clear();
            phase = PHASE_CHAINED;
        }

        ObjectGuid playerGUID;
        UnworthyInitiatePhase phase;
        uint32 wait_timer;
        float anchorX, anchorY;
        ObjectGuid anchorGUID;

        EventMap events;

        void Reset() override
        {
            Initialize();
            events.Reset();
            me->SetFaction(FACTION_CREATURE);
            me->SetImmuneToPC(true);
            me->SetStandState(UNIT_STAND_STATE_KNEEL);
            me->LoadEquipment(0, true);
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            events.ScheduleEvent(EVENT_ICY_TOUCH, 1000, GCD_CAST);
            events.ScheduleEvent(EVENT_PLAGUE_STRIKE, 3000, GCD_CAST);
            events.ScheduleEvent(EVENT_BLOOD_STRIKE, 2000, GCD_CAST);
            events.ScheduleEvent(EVENT_DEATH_COIL, 5000, GCD_CAST);
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == 1)
            {
                wait_timer = 5000;
                me->LoadEquipment(1);
                me->CastSpell(me, SPELL_DK_INITIATE_VISUAL, true);

                if (Player* starter = ObjectAccessor::GetPlayer(*me, playerGUID))
                    Talk(SAY_EVENT_ATTACK, starter);

                phase = PHASE_TO_ATTACK;
            }
        }

        void EventStart(Creature* anchor, Player* target)
        {
            wait_timer = 5000;
            phase = PHASE_TO_EQUIP;

            me->SetStandState(UNIT_STAND_STATE_STAND);
            me->RemoveAurasDueToSpell(SPELL_SOUL_PRISON_CHAIN);

            float z;
            anchor->GetContactPoint(me, anchorX, anchorY, z, 1.0f);

            playerGUID = target->GetGUID();
            Talk(SAY_EVENT_START, target);
        }

        void UpdateAI(uint32 diff) override
        {
            switch (phase)
            {
            case PHASE_CHAINED:
                if (!anchorGUID)
                {
                    if (Creature* anchor = me->FindNearestCreature(29521, 30))
                    {
                        anchor->AI()->SetGUID(me->GetGUID());
                        anchor->CastSpell(me, SPELL_SOUL_PRISON_CHAIN, true);
                        anchorGUID = anchor->GetGUID();
                    }
                    else
                        TC_LOG_ERROR("scripts", "npc_unworthy_initiateAI: unable to find anchor!");

                    float dist = 99.0f;
                    GameObject* prison = nullptr;

                    for (uint8 i = 0; i < 12; ++i)
                    {
                        if (GameObject* temp_prison = me->FindNearestGameObject(acherus_soul_prison[i], 30))
                        {
                            if (me->IsWithinDist(temp_prison, dist, false))
                            {
                                dist = me->GetDistance2d(temp_prison);
                                prison = temp_prison;
                            }
                        }
                    }

                    if (prison)
                        prison->ResetDoorOrButton();
                    else
                        TC_LOG_ERROR("scripts", "npc_unworthy_initiateAI: unable to find prison!");
                }
                break;
            case PHASE_TO_EQUIP:
                if (wait_timer)
                {
                    if (wait_timer > diff)
                        wait_timer -= diff;
                    else
                    {
                        me->GetMotionMaster()->MovePoint(1, anchorX, anchorY, me->GetPositionZ());
                        //TC_LOG_DEBUG("scripts", "npc_unworthy_initiateAI: move to %f %f %f", anchorX, anchorY, me->GetPositionZ());
                        phase = PHASE_EQUIPING;
                        wait_timer = 0;
                    }
                }
                break;
            case PHASE_TO_ATTACK:
                if (wait_timer)
                {
                    if (wait_timer > diff)
                        wait_timer -= diff;
                    else
                    {
                        me->SetFaction(FACTION_MONSTER);
                        me->SetImmuneToPC(false);
                        me->SetReactState(REACT_AGGRESSIVE);
                        phase = PHASE_ATTACKING;

                        if (Player* target = ObjectAccessor::GetPlayer(*me, playerGUID))
                            AttackStart(target);
                        wait_timer = 0;
                    }
                }
                break;
            case PHASE_ATTACKING:
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_ICY_TOUCH:
                        DoCastVictim(SPELL_ICY_TOUCH);
                        events.DelayEvents(1000, GCD_CAST);
                        events.ScheduleEvent(EVENT_ICY_TOUCH, 5000, GCD_CAST);
                        break;
                    case EVENT_PLAGUE_STRIKE:
                        DoCastVictim(SPELL_PLAGUE_STRIKE);
                        events.DelayEvents(1000, GCD_CAST);
                        events.ScheduleEvent(EVENT_PLAGUE_STRIKE, 5000, GCD_CAST);
                        break;
                    case EVENT_BLOOD_STRIKE:
                        DoCastVictim(SPELL_BLOOD_STRIKE);
                        events.DelayEvents(1000, GCD_CAST);
                        events.ScheduleEvent(EVENT_BLOOD_STRIKE, 5000, GCD_CAST);
                        break;
                    case EVENT_DEATH_COIL:
                        DoCastVictim(SPELL_DEATH_COIL);
                        events.DelayEvents(1000, GCD_CAST);
                        events.ScheduleEvent(EVENT_DEATH_COIL, 5000, GCD_CAST);
                        break;
                    }
                }

                DoMeleeAttackIfReady();
                break;
            default:
                break;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_unworthy_initiateAI(creature);
    }
};

class npc_unworthy_initiate_anchor : public CreatureScript
{
public:
    npc_unworthy_initiate_anchor() : CreatureScript("npc_unworthy_initiate_anchor") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_unworthy_initiate_anchorAI(creature);
    }

    struct npc_unworthy_initiate_anchorAI : public PassiveAI
    {
        npc_unworthy_initiate_anchorAI(Creature* creature) : PassiveAI(creature) { }

        ObjectGuid prisonerGUID;

        void SetGUID(ObjectGuid const& guid, int32 /*id*/) override
        {
            prisonerGUID = guid;
        }

        ObjectGuid GetGUID(int32 /*id*/) const override
        {
            return prisonerGUID;
        }
    };
};

class go_acherus_soul_prison : public GameObjectScript
{
public:
    go_acherus_soul_prison() : GameObjectScript("go_acherus_soul_prison") { }

    struct go_acherus_soul_prisonAI : public GameObjectAI
    {
        go_acherus_soul_prisonAI(GameObject* go) : GameObjectAI(go) { }

        bool GossipHello(Player* player) override
        {
            if (Creature* anchor = me->FindNearestCreature(29521, 15))
                if (ObjectGuid prisonerGUID = anchor->AI()->GetGUID())
                    if (Creature* prisoner = ObjectAccessor::GetCreature(*player, prisonerGUID))
                        ENSURE_AI(npc_unworthy_initiate::npc_unworthy_initiateAI, prisoner->AI())->EventStart(anchor, player);

            return false;
        }
    };

    GameObjectAI* GetAI(GameObject* go) const override
    {
        return new go_acherus_soul_prisonAI(go);
    }
};

/*######
## npc_eye_of_acherus
######*/

enum EyeOfAcherusMisc
{
    SPELL_THE_EYE_OF_ACHERUS                = 51852,
    SPELL_EYE_OF_ACHERUS_VISUAL             = 51892,
    SPELL_EYE_OF_ACHERUS_FLIGHT_BOOST       = 51923,
    SPELL_EYE_OF_ACHERUS_FLIGHT             = 51890,
    SPELL_ROOT_SELF                         = 51860,

    EVENT_ANNOUNCE_LAUNCH_TO_DESTINATION    = 1,
    EVENT_UNROOT                            = 2,
    EVENT_LAUNCH_TOWARDS_DESTINATION        = 3,
    EVENT_GRANT_CONTROL                     = 4,

    SAY_LAUNCH_TOWARDS_DESTINATION          = 0,
    SAY_EYE_UNDER_CONTROL                   = 1,

    POINT_NEW_AVALON                        = 1
};

static constexpr uint8 const EyeOfAcherusPathSize = 4;
G3D::Vector3 const EyeOfAcherusPath[EyeOfAcherusPathSize] =
{
    { 2361.21f,  -5660.45f,  496.744f  },
    { 2341.571f, -5672.797f, 538.3942f },
    { 1957.4f,   -5844.1f,   273.867f  },
    { 1758.01f,  -5876.79f,  166.867f  }
};

struct npc_eye_of_acherus : public ScriptedAI
{
    npc_eye_of_acherus(Creature* creature) : ScriptedAI(creature)
    {
        creature->SetDisplayFromModel(0);
        creature->SetReactState(REACT_PASSIVE);
    }

    void JustAppeared() override
    {
        DoCastSelf(SPELL_ROOT_SELF);
        DoCastSelf(SPELL_EYE_OF_ACHERUS_VISUAL);
        _events.ScheduleEvent(EVENT_ANNOUNCE_LAUNCH_TO_DESTINATION, 7s);
    }

    void OnCharmed(bool /*isNew*/)  override
    {
        bool const charmed = me->IsCharmed();
        if (!charmed)
        {
            me->GetCharmerOrOwner()->RemoveAurasDueToSpell(SPELL_THE_EYE_OF_ACHERUS);
            me->GetCharmerOrOwner()->RemoveAurasDueToSpell(SPELL_EYE_OF_ACHERUS_FLIGHT_BOOST);
        }
        else
        {
            if (Player* owner = me->GetCharmerOrOwner()->ToPlayer())
            {
                me->GetCharmInfo()->InitPossessCreateSpells();
                owner->SendAutoRepeatCancel(me);
            }
            me->SetControlled(true, UNIT_STATE_ROOT); // Todo: remove me after root serverside spell has its effect
        }
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_ANNOUNCE_LAUNCH_TO_DESTINATION:
                    if (Unit* owner = me->GetCharmerOrOwner())
                        Talk(SAY_LAUNCH_TOWARDS_DESTINATION, owner);
                    _events.ScheduleEvent(EVENT_UNROOT, 1s + 200ms);
                    break;
                case EVENT_UNROOT:
                    me->SetControlled(false, UNIT_STATE_ROOT); // Todo: remove me after root serverside spell has its effect
                    me->RemoveAurasDueToSpell(SPELL_ROOT_SELF);
                    DoCastSelf(SPELL_EYE_OF_ACHERUS_FLIGHT_BOOST);
                    _events.ScheduleEvent(EVENT_LAUNCH_TOWARDS_DESTINATION, 1s + 200ms);
                    break;
                case EVENT_LAUNCH_TOWARDS_DESTINATION:
                {
                    Movement::PointsArray path(EyeOfAcherusPath, EyeOfAcherusPath + EyeOfAcherusPathSize);
                    Movement::MoveSplineInit init(me);
                    init.MovebyPath(path);
                    init.SetFly();
                    init.SetUncompressed();
                    init.SetSmooth();
                    if (Unit* owner = me->GetCharmerOrOwner())
                        init.SetVelocity(owner->GetSpeed(MOVE_RUN));

                    me->GetMotionMaster()->LaunchMoveSpline(std::move(init), POINT_NEW_AVALON, MOTION_SLOT_ACTIVE, POINT_MOTION_TYPE);
                    break;
                }
                case EVENT_GRANT_CONTROL:
                    me->SetControlled(false, UNIT_STATE_ROOT); // Todo: remove me after root serverside spell has its effect
                    me->RemoveAurasDueToSpell(SPELL_ROOT_SELF);
                    DoCastSelf(SPELL_EYE_OF_ACHERUS_FLIGHT);
                    me->RemoveAurasDueToSpell(SPELL_EYE_OF_ACHERUS_FLIGHT_BOOST);
                    if (Unit* owner = me->GetCharmerOrOwner())
                        Talk(SAY_EYE_UNDER_CONTROL, owner);
                    break;
                default:
                    break;
            }
        }
    }

    void MovementInform(uint32 movementType, uint32 pointId) override
    {
        if (movementType != POINT_MOTION_TYPE)
            return;

        switch (pointId)
        {
            case POINT_NEW_AVALON:
                me->SetControlled(true, UNIT_STATE_ROOT); // Todo: remove me after root serverside spell has its effect
                DoCastSelf(SPELL_ROOT_SELF);
                _events.ScheduleEvent(EVENT_GRANT_CONTROL, 2s + 500ms);
                break;
            default:
                break;
        }
    }

private:
    EventMap _events;
};

/*######
## npc_death_knight_initiate
######*/

enum Spells_DKI
{
    SPELL_DUEL                  = 52996,
    //SPELL_DUEL_TRIGGERED        = 52990,
    SPELL_DUEL_VICTORY          = 52994,
    SPELL_DUEL_FLAG             = 52991,
    SPELL_GROVEL                = 7267,
};

enum Says_VBM
{
    SAY_DUEL                    = 0,
};

enum Misc_VBN
{
    QUEST_DEATH_CHALLENGE       = 12733
};

class npc_death_knight_initiate : public CreatureScript
{
public:
    npc_death_knight_initiate() : CreatureScript("npc_death_knight_initiate") { }

    struct npc_death_knight_initiateAI : public CombatAI
    {
        npc_death_knight_initiateAI(Creature* creature) : CombatAI(creature)
        {
            Initialize();
        }

        void Initialize()
        {
            m_uiDuelerGUID.Clear();
            m_uiDuelTimer = 5000;
            m_bIsDuelInProgress = false;
            lose = false;
        }

        bool lose;
        ObjectGuid m_uiDuelerGUID;
        uint32 m_uiDuelTimer;
        bool m_bIsDuelInProgress;

        void Reset() override
        {
            Initialize();

            me->RestoreFaction();
            CombatAI::Reset();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_CAN_SWIM);
        }

        void SpellHit(WorldObject* pCaster, SpellInfo const* pSpell) override
        {
            if (!m_bIsDuelInProgress && pSpell->Id == SPELL_DUEL)
            {
                m_uiDuelerGUID = pCaster->GetGUID();
                Talk(SAY_DUEL, pCaster);
                m_bIsDuelInProgress = true;
            }
        }

       void DamageTaken(Unit* pDoneBy, uint32 &uiDamage) override
        {
            if (m_bIsDuelInProgress && pDoneBy && pDoneBy->IsControlledByPlayer())
            {
                if (pDoneBy->GetGUID() != m_uiDuelerGUID && pDoneBy->GetOwnerOrCreatorGUID() != m_uiDuelerGUID) // other players cannot help
                    uiDamage = 0;
                else if (uiDamage >= me->GetHealth())
                {
                    uiDamage = 0;

                    if (!lose)
                    {
                        pDoneBy->RemoveGameObject(SPELL_DUEL_FLAG, true);
                        pDoneBy->AttackStop();
                        me->CastSpell(pDoneBy, SPELL_DUEL_VICTORY, true);
                        lose = true;
                        me->CastSpell(me, SPELL_GROVEL, true);
                        me->RestoreFaction();
                    }
                }
            }
        }

        void UpdateAI(uint32 uiDiff) override
        {
            if (!UpdateVictim())
            {
                if (m_bIsDuelInProgress)
                {
                    if (m_uiDuelTimer <= uiDiff)
                    {
                        me->SetFaction(FACTION_UNDEAD_SCOURGE_2);

                        if (Unit* unit = ObjectAccessor::GetUnit(*me, m_uiDuelerGUID))
                            AttackStart(unit);
                    }
                    else
                        m_uiDuelTimer -= uiDiff;
                }
                return;
            }

            if (m_bIsDuelInProgress)
            {
                if (lose)
                {
                    if (!me->HasAura(SPELL_GROVEL))
                        EnterEvadeMode();
                    return;
                }
                else if (me->GetVictim() && me->EnsureVictim()->GetTypeId() == TYPEID_PLAYER && me->EnsureVictim()->HealthBelowPct(10))
                {
                    me->EnsureVictim()->CastSpell(me->GetVictim(), SPELL_GROVEL, true); // beg
                    me->EnsureVictim()->RemoveGameObject(SPELL_DUEL_FLAG, true);
                    EnterEvadeMode();
                    return;
                }
            }

            /// @todo spells

            CombatAI::UpdateAI(uiDiff);
        }

        bool GossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override
        {
            uint32 const action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
            ClearGossipMenuFor(player);
            if (action == GOSSIP_ACTION_INFO_DEF)
            {
                CloseGossipMenuFor(player);

                if (player->IsInCombat() || me->IsInCombat())
                    return true;

                if (npc_death_knight_initiateAI* pInitiateAI = CAST_AI(npc_death_knight_initiate::npc_death_knight_initiateAI, me->AI()))
                {
                    if (pInitiateAI->m_bIsDuelInProgress)
                        return true;
                }

                me->SetImmuneToPC(false);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_CAN_SWIM);

                player->CastSpell(me, SPELL_DUEL, false);
                player->CastSpell(player, SPELL_DUEL_FLAG, true);
            }
            return true;
        }

        bool GossipHello(Player* player) override
        {
            if (player->GetQuestStatus(QUEST_DEATH_CHALLENGE) == QUEST_STATUS_INCOMPLETE && me->IsFullHealth())
            {
                if (player->HealthBelowPct(10))
                    return true;

                if (player->IsInCombat() || me->IsInCombat())
                    return true;

                AddGossipItemFor(player, Player::GetDefaultGossipMenuForSource(me), 0, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
                SendGossipMenuFor(player, player->GetGossipTextId(me), me->GetGUID());
            }
            return true;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_death_knight_initiateAI(creature);
    }
};

/*######
## npc_dark_rider_of_acherus
######*/

enum DarkRiderOfAcherus
{
    SAY_DARK_RIDER              = 0,
    SPELL_DESPAWN_HORSE         = 51918
};

class npc_dark_rider_of_acherus : public CreatureScript
{
    public:
        npc_dark_rider_of_acherus() : CreatureScript("npc_dark_rider_of_acherus") { }

        struct npc_dark_rider_of_acherusAI : public ScriptedAI
        {
            npc_dark_rider_of_acherusAI(Creature* creature) : ScriptedAI(creature)
            {
                Initialize();
            }

            void Initialize()
            {
                PhaseTimer = 4000;
                Phase = 0;
                Intro = false;
                TargetGUID.Clear();
            }

            void Reset() override
            {
                Initialize();
            }

            void UpdateAI(uint32 diff) override
            {
                if (!Intro || !TargetGUID)
                    return;

                if (PhaseTimer <= diff)
                {
                    switch (Phase)
                    {
                       case 0:
                            Talk(SAY_DARK_RIDER);
                            PhaseTimer = 5000;
                            Phase = 1;
                            break;
                        case 1:
                            if (Unit* target = ObjectAccessor::GetUnit(*me, TargetGUID))
                                DoCast(target, SPELL_DESPAWN_HORSE, true);
                            PhaseTimer = 3000;
                            Phase = 2;
                            break;
                        case 2:
                            me->SetVisible(false);
                            PhaseTimer = 2000;
                            Phase = 3;
                            break;
                        case 3:
                            me->DespawnOrUnsummon();
                            break;
                        default:
                            break;
                    }
                }
                else
                    PhaseTimer -= diff;
            }

            void InitDespawnHorse(Unit* who)
            {
                if (!who)
                    return;

                TargetGUID = who->GetGUID();
                me->SetWalk(true);
                me->SetSpeedRate(MOVE_RUN, 0.4f);
                me->GetMotionMaster()->MoveChase(who);
                me->SetTarget(TargetGUID);
                Intro = true;
            }

        private:
            uint32 PhaseTimer;
            uint32 Phase;
            bool Intro;
            ObjectGuid TargetGUID;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_dark_rider_of_acherusAI(creature);
        }
};

/*######
## npc_salanar_the_horseman
######*/

enum SalanarTheHorseman
{
    GOSSIP_SALANAR_MENU               = 9739,
    GOSSIP_SALANAR_OPTION             = 0,
    SALANAR_SAY                       = 0,
    QUEST_INTO_REALM_OF_SHADOWS       = 12687,
    NPC_DARK_RIDER_OF_ACHERUS         = 28654,
    NPC_SALANAR_IN_REALM_OF_SHADOWS   = 28788,
    SPELL_EFFECT_STOLEN_HORSE         = 52263,
    SPELL_DELIVER_STOLEN_HORSE        = 52264,
    SPELL_CALL_DARK_RIDER             = 52266,
    SPELL_EFFECT_OVERTAKE             = 52349,
    SPELL_REALM_OF_SHADOWS            = 52693
};

class npc_salanar_the_horseman : public CreatureScript
{
public:
    npc_salanar_the_horseman() : CreatureScript("npc_salanar_the_horseman") { }

    struct npc_salanar_the_horsemanAI : public ScriptedAI
    {
        npc_salanar_the_horsemanAI(Creature* creature) : ScriptedAI(creature) { }

        bool GossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
        {
            if (menuId == GOSSIP_SALANAR_MENU && gossipListId == GOSSIP_SALANAR_OPTION)
            {
                player->CastSpell(player, SPELL_REALM_OF_SHADOWS, true);
                player->PlayerTalkClass->SendCloseGossip();
            }
            return false;
        }

        void SpellHit(WorldObject* caster, SpellInfo const* spell) override
        {
            if (spell->Id == SPELL_DELIVER_STOLEN_HORSE)
            {
                if (caster->GetTypeId() == TYPEID_UNIT && caster->ToCreature()->IsVehicle())
                {
                    Creature* creatureCaster = caster->ToCreature();

                    if (Unit* charmer = creatureCaster->GetCharmer())
                    {
                        if (charmer->HasAura(SPELL_EFFECT_STOLEN_HORSE))
                        {
                            charmer->RemoveAurasDueToSpell(SPELL_EFFECT_STOLEN_HORSE);
                            creatureCaster->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                            creatureCaster->SetFaction(FACTION_FRIENDLY);
                            DoCast(creatureCaster, SPELL_CALL_DARK_RIDER, true);
                            if (Creature* Dark_Rider = me->FindNearestCreature(NPC_DARK_RIDER_OF_ACHERUS, 15))
                                ENSURE_AI(npc_dark_rider_of_acherus::npc_dark_rider_of_acherusAI, Dark_Rider->AI())->InitDespawnHorse(creatureCaster);
                        }
                    }
                }
            }
        }

        void MoveInLineOfSight(Unit* who) override
        {
            ScriptedAI::MoveInLineOfSight(who);

            if (who->GetTypeId() == TYPEID_UNIT && who->IsVehicle() && me->IsWithinDistInMap(who, 5.0f))
            {
                if (Unit* charmer = who->GetCharmer())
                {
                    if (Player* player = charmer->ToPlayer())
                    {
                        // for quest Into the Realm of Shadows(QUEST_INTO_REALM_OF_SHADOWS)
                        if (me->GetEntry() == NPC_SALANAR_IN_REALM_OF_SHADOWS && player->GetQuestStatus(QUEST_INTO_REALM_OF_SHADOWS) == QUEST_STATUS_INCOMPLETE)
                        {
                            player->GroupEventHappens(QUEST_INTO_REALM_OF_SHADOWS, me);
                            Talk(SALANAR_SAY);
                            charmer->RemoveAurasDueToSpell(SPELL_EFFECT_OVERTAKE);
                            if (Creature* creature = who->ToCreature())
                            {
                                creature->DespawnOrUnsummon();
                                //creature->Respawn(true);
                            }
                        }

                        player->RemoveAurasDueToSpell(SPELL_REALM_OF_SHADOWS);
                    }
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_salanar_the_horsemanAI(creature);
    }
};

/*######
## npc_ros_dark_rider
######*/

class npc_ros_dark_rider : public CreatureScript
{
public:
    npc_ros_dark_rider() : CreatureScript("npc_ros_dark_rider") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_ros_dark_riderAI(creature);
    }

    struct npc_ros_dark_riderAI : public ScriptedAI
    {
        npc_ros_dark_riderAI(Creature* creature) : ScriptedAI(creature) { }

        void JustEngagedWith(Unit* /*who*/) override
        {
            me->ExitVehicle();
        }

        void Reset() override
        {
            Creature* deathcharger = me->FindNearestCreature(28782, 30);
            if (!deathcharger)
                return;

            deathcharger->RestoreFaction();
            deathcharger->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
            deathcharger->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            if (!me->GetVehicle() && deathcharger->IsVehicle() && deathcharger->GetVehicleKit()->HasEmptySeat(0))
                me->EnterVehicle(deathcharger);
        }

        void JustDied(Unit* killer) override
        {
            Creature* deathcharger = me->FindNearestCreature(28782, 30);
            if (!deathcharger || !killer)
                return;

            if (killer->GetTypeId() == TYPEID_PLAYER && deathcharger->GetTypeId() == TYPEID_UNIT && deathcharger->IsVehicle())
            {
                deathcharger->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                deathcharger->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                deathcharger->SetFaction(2096);
            }
        }
    };

};

// correct way: 52312 52314 52555 ...
enum TheGiftThatKeepsOnGiving
{
    SAY_LINE_0 = 0,

    NPC_GHOULS = 28845,
    NPC_GHOSTS = 28846,
};

class npc_dkc1_gothik : public CreatureScript
{
public:
    npc_dkc1_gothik() : CreatureScript("npc_dkc1_gothik") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_dkc1_gothikAI(creature);
    }

    struct npc_dkc1_gothikAI : public ScriptedAI
    {
        npc_dkc1_gothikAI(Creature* creature) : ScriptedAI(creature) { }

        void MoveInLineOfSight(Unit* who) override

        {
            ScriptedAI::MoveInLineOfSight(who);

            if (who->GetEntry() == NPC_GHOULS && me->IsWithinDistInMap(who, 10.0f))
            {
                if (Unit* owner = who->GetOwner())
                {
                    if (Player* player = owner->ToPlayer())
                    {
                        Creature* creature = who->ToCreature();
                        if (player->GetQuestStatus(12698) == QUEST_STATUS_INCOMPLETE)
                            creature->CastSpell(owner, 52517, true);

                        /// @todo Creatures must not be removed, but, must instead
                        //      stand next to Gothik and be commanded into the pit
                        //      and dig into the ground.
                        creature->DespawnOrUnsummon();

                        if (player->GetQuestStatus(12698) == QUEST_STATUS_COMPLETE)
                            owner->RemoveAllMinionsByEntry(NPC_GHOSTS);
                    }
                }
            }
        }
    };

};

struct npc_scarlet_ghoul : public ScriptedAI
{
    npc_scarlet_ghoul(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_DEFENSIVE);
    }

    void JustAppeared() override
    {
        CreatureAI::JustAppeared();

        if (urand(0, 1))
            if (Unit* owner = me->GetOwner())
                Talk(SAY_LINE_0, owner);
    }

    void FindMinions(Unit* owner)
    {
        std::list<Creature*> MinionList;
        owner->GetAllMinionsByEntry(MinionList, NPC_GHOULS);

        if (!MinionList.empty())
        {
            for (Creature* creature : MinionList)
            {
                if (creature->GetOwner()->GetGUID() == me->GetOwner()->GetGUID())
                {
                    if (creature->IsInCombat() && creature->getAttackerForHelper())
                    {
                        AttackStart(creature->getAttackerForHelper());
                    }
                }
            }
        }
    }

    void UpdateAI(uint32 /*diff*/) override
    {
        if (!me->IsInCombat())
        {
            if (Unit* owner = me->GetOwner())
            {
                Player* plrOwner = owner->ToPlayer();
                if (plrOwner && plrOwner->IsInCombat())
                {
                    if (plrOwner->getAttackerForHelper() && plrOwner->getAttackerForHelper()->GetEntry() == NPC_GHOSTS)
                        AttackStart(plrOwner->getAttackerForHelper());
                    else
                        FindMinions(owner);
                }
            }
        }

        if (!UpdateVictim() || !me->GetVictim())
            return;

        //ScriptedAI::UpdateAI(diff);
        //Check if we have a current target
        if (me->EnsureVictim()->GetEntry() == NPC_GHOSTS)
        {
            if (me->isAttackReady())
            {
                //If we are within range melee the target
                if (me->IsWithinMeleeRange(me->GetVictim()))
                {
                    me->AttackerStateUpdate(me->GetVictim());
                    me->resetAttackTimer();
                }
            }
        }
    }
};

enum GiftOfTheHarvester
{
    SPELL_GHOUL_TRANFORM    = 52490,
    SPELL_GHOST_TRANSFORM   = 52505
};

class spell_gift_of_the_harvester : public SpellScript
{
    bool Validate(SpellInfo const* /*spell*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_GHOUL_TRANFORM,
                SPELL_GHOST_TRANSFORM
            });
    }

    void HandleScriptEffect(SpellEffIndex /*effIndex*/)
    {
        Unit* originalCaster = GetOriginalCaster();
        Unit* target = GetHitUnit();

        if (originalCaster && target)
            originalCaster->CastSpell(target, RAND(SPELL_GHOUL_TRANFORM, SPELL_GHOST_TRANSFORM), true);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gift_of_the_harvester::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};
}

void AddSC_the_scarlet_enclave_chapter_1()
{
    using namespace TheScarletEnclave::Chapter1;
    new npc_unworthy_initiate();
    new npc_unworthy_initiate_anchor();
    new go_acherus_soul_prison();
    RegisterCreatureAI(npc_eye_of_acherus);
    new npc_death_knight_initiate();
    new npc_salanar_the_horseman();
    new npc_dark_rider_of_acherus();
    new npc_ros_dark_rider();
    new npc_dkc1_gothik();
    RegisterCreatureAI(npc_scarlet_ghoul);
    RegisterSpellScript(spell_gift_of_the_harvester);
}
