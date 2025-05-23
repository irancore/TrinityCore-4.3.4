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

/*
 * Scripts for spells with SPELLFAMILY_GENERIC spells used by items.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_item_".
 */

#include "ScriptMgr.h"
#include "Battleground.h"
#include "CreatureAIImpl.h"
#include "Containers.h"
#include "DBCStores.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "SkillDiscovery.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"
#include "SpellMgr.h"
#include "SpellScript.h"

namespace Spells::Items
{
// Generic script for handling item dummy effects which trigger another spell.
class spell_item_trigger_spell : public SpellScriptLoader
{
    private:
        uint32 _triggeredSpellId;

    public:
        spell_item_trigger_spell(char const* name, uint32 triggeredSpellId) : SpellScriptLoader(name), _triggeredSpellId(triggeredSpellId) { }

        class spell_item_trigger_spell_SpellScript : public SpellScript
        {
        private:
            uint32 _triggeredSpellId;

        public:
            spell_item_trigger_spell_SpellScript(uint32 triggeredSpellId) : SpellScript(), _triggeredSpellId(triggeredSpellId) { }

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ _triggeredSpellId });
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (Item* item = GetCastItem())
                    caster->CastSpell(caster, _triggeredSpellId, item);
            }

            void Register() override
            {
                OnEffectHit.Register(&spell_item_trigger_spell_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_trigger_spell_SpellScript(_triggeredSpellId);
        }
};

enum AegisOfPreservation
{
    SPELL_AEGIS_HEAL   = 23781
};

// 23780 - Aegis of Preservation
class spell_item_aegis_of_preservation : public SpellScriptLoader
{
    public:
        spell_item_aegis_of_preservation() : SpellScriptLoader("spell_item_aegis_of_preservation") { }

        class spell_item_aegis_of_preservation_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_AEGIS_HEAL });
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
            {
                PreventDefaultAction();
                GetTarget()->CastSpell(GetTarget(), SPELL_AEGIS_HEAL, aurEff);
            }

            void Register() override
            {
                OnEffectProc.Register(&spell_item_aegis_of_preservation_AuraScript::HandleProc, EFFECT_1, SPELL_AURA_PROC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_aegis_of_preservation_AuraScript();
        }
};

enum AlchemistStone
{
    SPELL_ALCHEMISTS_STONE_EXTRA_HEAL       = 21399,
    SPELL_ALCHEMISTS_STONE_EXTRA_MANA       = 21400
};

// Item - 13503: Alchemist's Stone
// Item - 35748: Guardian's Alchemist Stone
// Item - 35749: Sorcerer's Alchemist Stone
// Item - 35750: Redeemer's Alchemist Stone
// Item - 35751: Assassin's Alchemist Stone
// Item - 44322: Mercurial Alchemist Stone
// Item - 44323: Indestructible Alchemist's Stone
// Item - 44324: Mighty Alchemist's Stone

// 17619 - Alchemist's Stone
class spell_item_alchemists_stone : public SpellScriptLoader
{
    public:
        spell_item_alchemists_stone() : SpellScriptLoader("spell_item_alchemists_stone") { }

        class spell_item_alchemists_stone_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_ALCHEMISTS_STONE_EXTRA_HEAL,
                    SPELL_ALCHEMISTS_STONE_EXTRA_MANA
                });
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();
                SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
                if (!spellInfo)
                    return;

                Unit* caster = eventInfo.GetActionTarget();
                for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                {
                    uint32 spellId;
                    switch (spellInfo->Effects[i].Effect)
                    {
                        case SPELL_EFFECT_HEAL:
                            spellId = SPELL_ALCHEMISTS_STONE_EXTRA_HEAL;
                            break;
                        case SPELL_EFFECT_ENERGIZE:
                            spellId = SPELL_ALCHEMISTS_STONE_EXTRA_MANA;
                            break;
                        default:
                            continue;
                    }

                    int32 amount = CalculatePct(spellInfo->Effects[i].CalcValue(caster), 40);
                    caster->CastSpell(nullptr, spellId, CastSpellExtraArgs(aurEff).AddSpellBP0(amount));
                }
            }

            void Register() override
            {
                OnEffectProc.Register(&spell_item_alchemists_stone_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_alchemists_stone_AuraScript();
        }
};

enum AngerCapacitor
{
    SPELL_MOTE_OF_ANGER             = 71432,
    SPELL_MANIFEST_ANGER_MAIN_HAND  = 71433,
    SPELL_MANIFEST_ANGER_OFF_HAND   = 71434
};

// Item - 50351: Tiny Abomination in a Jar
// 71406 - Anger Capacitor

// Item - 50706: Tiny Abomination in a Jar (Heroic)
// 71545 - Anger Capacitor
template <uint8 StackAmount>
class spell_item_anger_capacitor : public SpellScriptLoader
{
    public:
        spell_item_anger_capacitor(char const* ScriptName) : SpellScriptLoader(ScriptName) { }

        template <uint8 Stacks>
        class spell_item_anger_capacitor_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_MOTE_OF_ANGER,
                    SPELL_MANIFEST_ANGER_MAIN_HAND,
                    SPELL_MANIFEST_ANGER_OFF_HAND
                });
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();
                Unit* caster = eventInfo.GetActor();
                Unit* target = eventInfo.GetProcTarget();

                caster->CastSpell((Unit*)nullptr, SPELL_MOTE_OF_ANGER, true);
                Aura const* motes = caster->GetAura(SPELL_MOTE_OF_ANGER);
                if (!motes || motes->GetStackAmount() < Stacks)
                    return;

                caster->RemoveAurasDueToSpell(SPELL_MOTE_OF_ANGER);
                uint32 spellId = SPELL_MANIFEST_ANGER_MAIN_HAND;
                if (Player* player = caster->ToPlayer())
                    if (player->GetWeaponForAttack(OFF_ATTACK, true) && urand(0, 1))
                        spellId = SPELL_MANIFEST_ANGER_OFF_HAND;

                caster->CastSpell(target, spellId, aurEff);
            }

            void Register() override
            {
                OnEffectProc.Register(&spell_item_anger_capacitor_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_anger_capacitor_AuraScript<StackAmount>();
        }
};

// 26400 - Arcane Shroud
class spell_item_arcane_shroud : public SpellScriptLoader
{
    public:
        spell_item_arcane_shroud() : SpellScriptLoader("spell_item_arcane_shroud") { }

        class spell_item_arcane_shroud_AuraScript : public AuraScript
        {
            void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
            {
                int32 diff = GetUnitOwner()->getLevel() - 60;
                if (diff > 0)
                    amount += 2 * diff;
            }

            void Register() override
            {
                DoEffectCalcAmount.Register(&spell_item_arcane_shroud_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_MOD_THREAT);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_arcane_shroud_AuraScript();
        }
};

enum AuraOfMadness
{
    SPELL_SOCIOPATH         = 39511, // Sociopath: +35 strength(Paladin, Rogue, Druid, Warrior)
    SPELL_DELUSIONAL        = 40997, // Delusional: +70 attack power(Rogue, Hunter, Paladin, Warrior, Druid)
    SPELL_KLEPTOMANIA       = 40998, // Kleptomania: +35 agility(Warrior, Rogue, Paladin, Hunter, Druid)
    SPELL_MEGALOMANIA       = 40999, // Megalomania: +41 damage / healing(Druid, Shaman, Priest, Warlock, Mage, Paladin)
    SPELL_PARANOIA          = 41002, // Paranoia: +35 spell / melee / ranged crit strike rating(All classes)
    SPELL_MANIC             = 41005, // Manic: +35 haste(spell, melee and ranged) (All classes)
    SPELL_NARCISSISM        = 41009, // Narcissism: +35 intellect(Druid, Shaman, Priest, Warlock, Mage, Paladin, Hunter)
    SPELL_MARTYR_COMPLEX    = 41011, // Martyr Complex: +35 stamina(All classes)
    SPELL_DEMENTIA          = 41404, // Dementia: Every 5 seconds either gives you +5/-5%  damage/healing. (Druid, Shaman, Priest, Warlock, Mage, Paladin)

    SPELL_DEMENTIA_POS      = 41406,
    SPELL_DEMENTIA_NEG      = 41409,

    SAY_MADNESS             = 21954
};

// Item - 31859: Darkmoon Card: Madness
// 39446 - Aura of Madness
class spell_item_aura_of_madness : public SpellScriptLoader
{
    public:
        spell_item_aura_of_madness() : SpellScriptLoader("spell_item_aura_of_madness") { }

        class spell_item_aura_of_madness_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_SOCIOPATH,
                    SPELL_DELUSIONAL,
                    SPELL_KLEPTOMANIA,
                    SPELL_MEGALOMANIA,
                    SPELL_PARANOIA,
                    SPELL_MANIC,
                    SPELL_NARCISSISM,
                    SPELL_MARTYR_COMPLEX,
                    SPELL_DEMENTIA
                }) && sObjectMgr->GetBroadcastText(SAY_MADNESS);
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                static std::vector<uint32> const triggeredSpells[MAX_CLASSES] =
                {
                    //CLASS_NONE
                    { },
                    //CLASS_WARRIOR
                    { SPELL_SOCIOPATH, SPELL_DELUSIONAL, SPELL_KLEPTOMANIA, SPELL_PARANOIA, SPELL_MANIC, SPELL_MARTYR_COMPLEX },
                    //CLASS_PALADIN
                    { SPELL_SOCIOPATH, SPELL_DELUSIONAL, SPELL_KLEPTOMANIA, SPELL_MEGALOMANIA, SPELL_PARANOIA, SPELL_MANIC, SPELL_NARCISSISM, SPELL_MARTYR_COMPLEX, SPELL_DEMENTIA },
                    //CLASS_HUNTER
                    { SPELL_DELUSIONAL, SPELL_MEGALOMANIA, SPELL_PARANOIA, SPELL_MANIC, SPELL_NARCISSISM, SPELL_MARTYR_COMPLEX, SPELL_DEMENTIA },
                    //CLASS_ROGUE
                    { SPELL_SOCIOPATH, SPELL_DELUSIONAL, SPELL_KLEPTOMANIA, SPELL_PARANOIA, SPELL_MANIC, SPELL_MARTYR_COMPLEX },
                    //CLASS_PRIEST
                    { SPELL_MEGALOMANIA, SPELL_PARANOIA, SPELL_MANIC, SPELL_NARCISSISM, SPELL_MARTYR_COMPLEX, SPELL_DEMENTIA },
                    //CLASS_DEATH_KNIGHT
                    { SPELL_SOCIOPATH, SPELL_DELUSIONAL, SPELL_KLEPTOMANIA, SPELL_PARANOIA, SPELL_MANIC, SPELL_MARTYR_COMPLEX },
                    //CLASS_SHAMAN
                    { SPELL_MEGALOMANIA, SPELL_PARANOIA, SPELL_MANIC, SPELL_NARCISSISM, SPELL_MARTYR_COMPLEX, SPELL_DEMENTIA },
                    //CLASS_MAGE
                    { SPELL_MEGALOMANIA, SPELL_PARANOIA, SPELL_MANIC, SPELL_NARCISSISM, SPELL_MARTYR_COMPLEX, SPELL_DEMENTIA },
                    //CLASS_WARLOCK
                    { SPELL_MEGALOMANIA, SPELL_PARANOIA, SPELL_MANIC, SPELL_NARCISSISM, SPELL_MARTYR_COMPLEX, SPELL_DEMENTIA },
                    //CLASS_UNK
                    { },
                    //CLASS_DRUID
                    { SPELL_SOCIOPATH, SPELL_DELUSIONAL, SPELL_KLEPTOMANIA, SPELL_MEGALOMANIA, SPELL_PARANOIA, SPELL_MANIC, SPELL_NARCISSISM, SPELL_MARTYR_COMPLEX, SPELL_DEMENTIA }
                };

                PreventDefaultAction();
                Unit* caster = eventInfo.GetActor();
                uint32 spellId = Trinity::Containers::SelectRandomContainerElement(triggeredSpells[caster->getClass()]);
                caster->CastSpell(caster, spellId, aurEff);

                if (roll_chance_i(10))
                    caster->Unit::Say(SAY_MADNESS);
            }

            void Register() override
            {
                OnEffectProc.Register(&spell_item_aura_of_madness_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_aura_of_madness_AuraScript();
        }
};

// 41404 - Dementia
class spell_item_dementia : public SpellScriptLoader
{
    public:
        spell_item_dementia() : SpellScriptLoader("spell_item_dementia") { }

        class spell_item_dementia_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_DEMENTIA_POS,
                    SPELL_DEMENTIA_NEG
                });
            }

            void HandlePeriodicDummy(AuraEffect const* aurEff)
            {
                PreventDefaultAction();
                GetTarget()->CastSpell(GetTarget(), RAND(SPELL_DEMENTIA_POS, SPELL_DEMENTIA_NEG), aurEff);
            }

            void Register() override
            {
                OnEffectPeriodic.Register(&spell_item_dementia_AuraScript::HandlePeriodicDummy, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_dementia_AuraScript();
        }
};

// 64411 - Blessing of Ancient Kings (Val'anyr, Hammer of Ancient Kings)
enum BlessingOfAncientKings
{
    SPELL_PROTECTION_OF_ANCIENT_KINGS   = 64413
};

class spell_item_blessing_of_ancient_kings : public SpellScriptLoader
{
    public:
        spell_item_blessing_of_ancient_kings() : SpellScriptLoader("spell_item_blessing_of_ancient_kings") { }

        class spell_item_blessing_of_ancient_kings_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_PROTECTION_OF_ANCIENT_KINGS });
            }

            bool CheckProc(ProcEventInfo& eventInfo)
            {
                return eventInfo.GetProcTarget() != nullptr;
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();

                HealInfo* healInfo = eventInfo.GetHealInfo();
                if (!healInfo || !healInfo->GetHeal())
                    return;

                int32 absorb = int32(CalculatePct(healInfo->GetHeal(), 15.0f));
                if (AuraEffect* protEff = eventInfo.GetProcTarget()->GetAuraEffect(SPELL_PROTECTION_OF_ANCIENT_KINGS, 0, eventInfo.GetActor()->GetGUID()))
                {
                    // The shield can grow to a maximum size of 20,000 damage absorbtion
                    protEff->SetAmount(std::min<int32>(protEff->GetAmount() + absorb, 20000));

                    // Refresh and return to prevent replacing the aura
                    protEff->GetBase()->RefreshDuration();
                }
                else
                    GetTarget()->CastSpell(eventInfo.GetProcTarget(), SPELL_PROTECTION_OF_ANCIENT_KINGS, CastSpellExtraArgs(aurEff).AddSpellBP0(absorb));
            }

            void Register() override
            {
                DoCheckProc.Register(&spell_item_blessing_of_ancient_kings_AuraScript::CheckProc);
                OnEffectProc.Register(&spell_item_blessing_of_ancient_kings_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_blessing_of_ancient_kings_AuraScript();
        }
};

// 64415 Val'anyr Hammer of Ancient Kings - Equip Effect
class spell_item_valanyr_hammer_of_ancient_kings : public AuraScript
{
    bool CheckProc(ProcEventInfo& eventInfo)
    {
        return eventInfo.GetHealInfo() && eventInfo.GetHealInfo()->GetEffectiveHeal() > 0;
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_item_valanyr_hammer_of_ancient_kings::CheckProc);
    }
};

enum DeadlyPrecision
{
    SPELL_DEADLY_PRECISION = 71564
};

// 71564 - Deadly Precision
class spell_item_deadly_precision : public SpellScriptLoader
{
    public:
        spell_item_deadly_precision() : SpellScriptLoader("spell_item_deadly_precision") { }

        class spell_item_deadly_precision_charm_AuraScript : public AuraScript
        {
            void HandleStackDrop(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
            {
                PreventDefaultAction();
                GetTarget()->RemoveAuraFromStack(GetId(), GetTarget()->GetGUID());
            }

            void Register() override
            {
                OnEffectProc.Register(&spell_item_deadly_precision_charm_AuraScript::HandleStackDrop, EFFECT_0, SPELL_AURA_MOD_RATING);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_deadly_precision_charm_AuraScript();
        }
};

// 71563 - Deadly Precision Dummy
class spell_item_deadly_precision_dummy : public SpellScriptLoader
{
    public:
        spell_item_deadly_precision_dummy() : SpellScriptLoader("spell_item_deadly_precision_dummy") { }

        class spell_item_deadly_precision_dummy_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_DEADLY_PRECISION });
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                SpellInfo const* spellInfo = sSpellMgr->AssertSpellInfo(SPELL_DEADLY_PRECISION);
                GetCaster()->CastSpell(GetCaster(), spellInfo->Id, CastSpellExtraArgs(true).AddSpellMod(SPELLVALUE_AURA_STACK, spellInfo->StackAmount));
            }

            void Register() override
            {
                OnEffectHit.Register(&spell_item_deadly_precision_dummy_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_deadly_precision_dummy_SpellScript();
        }
};

enum DeathbringersWill
{
    SPELL_STRENGTH_OF_THE_TAUNKA        = 71484, // +600 Strength
    SPELL_AGILITY_OF_THE_VRYKUL         = 71485, // +600 Agility
    SPELL_POWER_OF_THE_TAUNKA           = 71486, // +1200 Attack Power
    SPELL_AIM_OF_THE_IRON_DWARVES       = 71491, // +600 Critical
    SPELL_SPEED_OF_THE_VRYKUL           = 71492, // +600 Haste

    SPELL_AGILITY_OF_THE_VRYKUL_HERO    = 71556, // +700 Agility
    SPELL_POWER_OF_THE_TAUNKA_HERO      = 71558, // +1400 Attack Power
    SPELL_AIM_OF_THE_IRON_DWARVES_HERO  = 71559, // +700 Critical
    SPELL_SPEED_OF_THE_VRYKUL_HERO      = 71560, // +700 Haste
    SPELL_STRENGTH_OF_THE_TAUNKA_HERO   = 71561  // +700 Strength
};

// Item - 50362: Deathbringer's Will
// 71519 - Item - Icecrown 25 Normal Melee Trinket

// Item - 50363: Deathbringer's Will
// 71562 - Item - Icecrown 25 Heroic Melee Trinket
template <uint32 StrengthSpellId, uint32 AgilitySpellId, uint32 APSpellId, uint32 CriticalSpellId, uint32 HasteSpellId>
class spell_item_deathbringers_will : public SpellScriptLoader
{
    public:
        spell_item_deathbringers_will(char const* ScriptName) : SpellScriptLoader(ScriptName) { }

        template <uint32 Strength, uint32 Agility, uint32 AttackPower, uint32 Critical, uint32 Haste>
        class spell_item_deathbringers_will_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    Strength,
                    Agility,
                    AttackPower,
                    Critical,
                    Haste
                });
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                static std::vector<uint32> const triggeredSpells[MAX_CLASSES] =
                {
                    //CLASS_NONE
                    { },
                    //CLASS_WARRIOR
                    { Strength, Critical, Haste },
                    //CLASS_PALADIN
                    { Strength, Critical, Haste },
                    //CLASS_HUNTER
                    { Agility, Critical, AttackPower },
                    //CLASS_ROGUE
                    { Agility, Haste, AttackPower },
                    //CLASS_PRIEST
                    { },
                    //CLASS_DEATH_KNIGHT
                    { Strength, Critical, Haste },
                    //CLASS_SHAMAN
                    { Agility, Haste, AttackPower },
                    //CLASS_MAGE
                    { },
                    //CLASS_WARLOCK
                    { },
                    //CLASS_UNK
                    { },
                    //CLASS_DRUID
                    { Strength, Agility, Haste }
                };

                PreventDefaultAction();
                Unit* caster = eventInfo.GetActor();
                std::vector<uint32> const& randomSpells = triggeredSpells[caster->getClass()];
                if (randomSpells.empty())
                    return;

                uint32 spellId = Trinity::Containers::SelectRandomContainerElement(randomSpells);
                caster->CastSpell(caster, spellId, aurEff);
            }

            void Register() override
            {
                OnEffectProc.Register(&spell_item_deathbringers_will_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_deathbringers_will_AuraScript<StrengthSpellId, AgilitySpellId, APSpellId, CriticalSpellId, HasteSpellId>();
        }
};

// 47770 - Roll Dice
class spell_item_decahedral_dwarven_dice : public SpellScriptLoader
{
    public:
        spell_item_decahedral_dwarven_dice() : SpellScriptLoader("spell_item_decahedral_dwarven_dice") { }

        class spell_item_decahedral_dwarven_dice_SpellScript : public SpellScript
        {
            enum
            {
                TEXT_DECAHEDRAL_DWARVEN_DICE = 26147
            };

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                if (!sObjectMgr->GetBroadcastText(TEXT_DECAHEDRAL_DWARVEN_DICE))
                    return false;
                return true;
            }

            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                GetCaster()->TextEmote(TEXT_DECAHEDRAL_DWARVEN_DICE, GetHitUnit());

                static uint32 const minimum = 1;
                static uint32 const maximum = 100;

                GetCaster()->ToPlayer()->DoRandomRoll(minimum, maximum);
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_decahedral_dwarven_dice_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_decahedral_dwarven_dice_SpellScript();
        }
};

// 8342  - Defibrillate (Goblin Jumper Cables) have 33% chance on success
// 22999 - Defibrillate (Goblin Jumper Cables XL) have 50% chance on success
// 54732 - Defibrillate (Gnomish Army Knife) have 67% chance on success
enum Defibrillate
{
    SPELL_GOBLIN_JUMPER_CABLES_FAIL     = 8338,
    SPELL_GOBLIN_JUMPER_CABLES_XL_FAIL  = 23055
};

class spell_item_defibrillate : public SpellScriptLoader
{
    public:
        spell_item_defibrillate(char const* name, uint8 chance, uint32 failSpell = 0) : SpellScriptLoader(name), _chance(chance), _failSpell(failSpell) { }

        class spell_item_defibrillate_SpellScript : public SpellScript
    {
                 public:
            spell_item_defibrillate_SpellScript(uint8 chance, uint32 failSpell) : SpellScript(), _chance(chance), _failSpell(failSpell) { }

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return !_failSpell || ValidateSpellInfo({ _failSpell });
            }

            void HandleScript(SpellEffIndex effIndex)
            {
                if (roll_chance_i(_chance))
                {
                    PreventHitDefaultEffect(effIndex);
                    if (_failSpell)
                        GetCaster()->CastSpell(GetCaster(), _failSpell, GetCastItem());
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_defibrillate_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_RESURRECT);
            }

        private:
            uint8 _chance;
            uint32 _failSpell;
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_defibrillate_SpellScript(_chance, _failSpell);
        }

    private:
        uint8 _chance;
        uint32 _failSpell;
};

enum DesperateDefense
{
    SPELL_DESPERATE_RAGE    = 33898
};

// 33896 - Desperate Defense
class spell_item_desperate_defense : public SpellScriptLoader
{
    public:
        spell_item_desperate_defense() : SpellScriptLoader("spell_item_desperate_defense") { }

        class spell_item_desperate_defense_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_DESPERATE_RAGE });
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
            {
                PreventDefaultAction();
                GetTarget()->CastSpell(GetTarget(), SPELL_DESPERATE_RAGE, aurEff);
            }

            void Register() override
            {
                OnEffectProc.Register(&spell_item_desperate_defense_AuraScript::HandleProc, EFFECT_2, SPELL_AURA_PROC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_desperate_defense_AuraScript();
        }
};

// http://www.wowhead.com/item=6522 Deviate Fish
// 8063 Deviate Fish
enum DeviateFishSpells
{
    SPELL_SLEEPY            = 8064,
    SPELL_INVIGORATE        = 8065,
    SPELL_SHRINK            = 8066,
    SPELL_PARTY_TIME        = 8067,
    SPELL_HEALTHY_SPIRIT    = 8068,
};

class spell_item_deviate_fish : public SpellScriptLoader
{
    public:
        spell_item_deviate_fish() : SpellScriptLoader("spell_item_deviate_fish") { }

        class spell_item_deviate_fish_SpellScript : public SpellScript
        {
            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_SLEEPY, SPELL_INVIGORATE, SPELL_SHRINK, SPELL_PARTY_TIME, SPELL_HEALTHY_SPIRIT });
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                uint32 spellId = urand(SPELL_SLEEPY, SPELL_HEALTHY_SPIRIT);
                caster->CastSpell(caster, spellId, true);
            }

            void Register() override
            {
                OnEffectHit.Register(&spell_item_deviate_fish_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_deviate_fish_SpellScript();
        }
};

enum DiscerningEyeBeastMisc
{
    SPELL_DISCERNING_EYE_BEAST = 59914
};

// 71610, 71641 - Echoes of Light (Althor's Abacus)
class spell_item_echoes_of_light : public SpellScriptLoader
{
    public:
        spell_item_echoes_of_light() : SpellScriptLoader("spell_item_echoes_of_light") { }

        class spell_item_echoes_of_light_SpellScript : public SpellScript
        {
            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (targets.size() < 2)
                    return;

                targets.sort(Trinity::HealthPctOrderPred());

                WorldObject* target = targets.front();
                targets.clear();
                targets.push_back(target);
            }

            void Register() override
            {
                OnObjectAreaTargetSelect.Register(&spell_item_echoes_of_light_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ALLY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_echoes_of_light_SpellScript();
        }
};

// 7434 - Fate Rune of Unsurpassed Vigor
enum FateRuneOfUnsurpassedVigor
{
    SPELL_UNSURPASSED_VIGOR = 25733
};

class spell_item_fate_rune_of_unsurpassed_vigor : public SpellScriptLoader
{
    public:
        spell_item_fate_rune_of_unsurpassed_vigor() : SpellScriptLoader("spell_item_fate_rune_of_unsurpassed_vigor") { }

        class spell_item_fate_rune_of_unsurpassed_vigor_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_UNSURPASSED_VIGOR });
            }

            void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
            {
                PreventDefaultAction();
                GetTarget()->CastSpell(GetTarget(), SPELL_UNSURPASSED_VIGOR, true);
            }

            void Register() override
            {
                OnEffectProc.Register(&spell_item_fate_rune_of_unsurpassed_vigor_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_fate_rune_of_unsurpassed_vigor_AuraScript();
        }
};

// http://www.wowhead.com/item=47499 Flask of the North
// 67019 Flask of the North
enum FlaskOfTheNorthSpells
{
    SPELL_FLASK_OF_THE_NORTH_SP = 67016,
    SPELL_FLASK_OF_THE_NORTH_AP = 67017,
    SPELL_FLASK_OF_THE_NORTH_STR = 67018,
};

class spell_item_flask_of_the_north : public SpellScriptLoader
{
    public:
        spell_item_flask_of_the_north() : SpellScriptLoader("spell_item_flask_of_the_north") { }

        class spell_item_flask_of_the_north_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_FLASK_OF_THE_NORTH_SP, SPELL_FLASK_OF_THE_NORTH_AP, SPELL_FLASK_OF_THE_NORTH_STR });
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                std::vector<uint32> possibleSpells;
                switch (caster->getClass())
                {
                    case CLASS_WARLOCK:
                    case CLASS_MAGE:
                    case CLASS_PRIEST:
                        possibleSpells.push_back(SPELL_FLASK_OF_THE_NORTH_SP);
                        break;
                    case CLASS_DEATH_KNIGHT:
                    case CLASS_WARRIOR:
                        possibleSpells.push_back(SPELL_FLASK_OF_THE_NORTH_STR);
                        break;
                    case CLASS_ROGUE:
                    case CLASS_HUNTER:
                        possibleSpells.push_back(SPELL_FLASK_OF_THE_NORTH_AP);
                        break;
                    case CLASS_DRUID:
                    case CLASS_PALADIN:
                        possibleSpells.push_back(SPELL_FLASK_OF_THE_NORTH_SP);
                        possibleSpells.push_back(SPELL_FLASK_OF_THE_NORTH_STR);
                        break;
                    case CLASS_SHAMAN:
                        possibleSpells.push_back(SPELL_FLASK_OF_THE_NORTH_SP);
                        possibleSpells.push_back(SPELL_FLASK_OF_THE_NORTH_AP);
                        break;
                }

                caster->CastSpell(caster, possibleSpells[urand(0, (possibleSpells.size() - 1))], true);
            }

            void Register() override
            {
                OnEffectHit.Register(&spell_item_flask_of_the_north_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_flask_of_the_north_SpellScript();
        }
};

enum FrozenShadoweave
{
    SPELL_SHADOWMEND    = 39373
};

// 39372 - Frozen Shadoweave
// Frozen Shadoweave set 3p bonus
class spell_item_frozen_shadoweave : public SpellScriptLoader
{
    public:
        spell_item_frozen_shadoweave() : SpellScriptLoader("spell_item_frozen_shadoweave") { }

        class spell_item_frozen_shadoweave_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_SHADOWMEND });
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();
                DamageInfo* damageInfo = eventInfo.GetDamageInfo();
                if (!damageInfo || !damageInfo->GetDamage())
                    return;

                int32 amount = CalculatePct(static_cast<int32>(damageInfo->GetDamage()), aurEff->GetAmount());
                Unit* caster = eventInfo.GetActor();
                caster->CastSpell(nullptr, SPELL_SHADOWMEND, CastSpellExtraArgs(aurEff).AddSpellBP0(amount));
            }

            void Register() override
            {
                OnEffectProc.Register(&spell_item_frozen_shadoweave_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_frozen_shadoweave_AuraScript();
        }
};

// http://www.wowhead.com/item=10645 Gnomish Death Ray
// 13280 Gnomish Death Ray
enum GnomishDeathRay
{
    SPELL_GNOMISH_DEATH_RAY_SELF = 13493,
    SPELL_GNOMISH_DEATH_RAY_TARGET = 13279,
};

class spell_item_gnomish_death_ray : public SpellScriptLoader
{
    public:
        spell_item_gnomish_death_ray() : SpellScriptLoader("spell_item_gnomish_death_ray") { }

        class spell_item_gnomish_death_ray_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_GNOMISH_DEATH_RAY_SELF, SPELL_GNOMISH_DEATH_RAY_TARGET });
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (Unit* target = GetHitUnit())
                {
                    if (urand(0, 99) < 15)
                        caster->CastSpell(caster, SPELL_GNOMISH_DEATH_RAY_SELF, true);    // failure
                    else
                        caster->CastSpell(target, SPELL_GNOMISH_DEATH_RAY_TARGET, true);
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_gnomish_death_ray_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_gnomish_death_ray_SpellScript();
        }
};

// Item 10721: Gnomish Harm Prevention Belt
// 13234 - Harm Prevention Belt
enum HarmPreventionBelt
{
    SPELL_FORCEFIELD_COLLAPSE = 13235
};

class spell_item_harm_prevention_belt : public SpellScriptLoader
{
    public:
        spell_item_harm_prevention_belt() : SpellScriptLoader("spell_item_harm_prevention_belt") { }

        class spell_item_harm_prevention_belt_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_FORCEFIELD_COLLAPSE });
            }

            void HandleProc(ProcEventInfo& /*eventInfo*/)
            {
                GetTarget()->CastSpell((Unit*)nullptr, SPELL_FORCEFIELD_COLLAPSE, true);
            }

            void Register() override
            {
                OnProc.Register(&spell_item_harm_prevention_belt_AuraScript::HandleProc);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_harm_prevention_belt_AuraScript();
        }
};

// Item 23004 - Idol of Longevity
// 28847 - Healing Touch Refund
enum IdolOfLongevity
{
    SPELL_HEALING_TOUCH_MANA    = 28848
};

class spell_item_healing_touch_refund : public SpellScriptLoader
{
    public:
        spell_item_healing_touch_refund() : SpellScriptLoader("spell_item_healing_touch_refund") { }

        class spell_item_healing_touch_refund_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_HEALING_TOUCH_MANA });
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();
                eventInfo.GetActor()->CastSpell(nullptr, SPELL_HEALING_TOUCH_MANA, aurEff);
            }

            void Register() override
            {
                OnEffectProc.Register(&spell_item_healing_touch_refund_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_healing_touch_refund_AuraScript();
        }
};

enum Heartpierce
{
    SPELL_INVIGORATION_MANA         = 71881,
    SPELL_INVIGORATION_ENERGY       = 71882,
    SPELL_INVIGORATION_RAGE         = 71883,
    SPELL_INVIGORATION_RP           = 71884,

    SPELL_INVIGORATION_RP_HERO      = 71885,
    SPELL_INVIGORATION_RAGE_HERO    = 71886,
    SPELL_INVIGORATION_ENERGY_HERO  = 71887,
    SPELL_INVIGORATION_MANA_HERO    = 71888
};

// Item - 49982: Heartpierce
// 71880 - Item - Icecrown 25 Normal Dagger Proc

// Item - 50641: Heartpierce (Heroic)
// 71892 - Item - Icecrown 25 Heroic Dagger Proc
template <uint32 EnergySpellId, uint32 ManaSpellId, uint32 RageSpellId, uint32 RPSpellId>
class spell_item_heartpierce : public SpellScriptLoader
{
    public:
        spell_item_heartpierce(char const* ScriptName) : SpellScriptLoader(ScriptName) { }

        template <uint32 Energy, uint32 Mana, uint32 Rage, uint32 RunicPower>
        class spell_item_heartpierce_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    Energy,
                    Mana,
                    Rage,
                    RunicPower
                });
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();
                Unit* caster = eventInfo.GetActor();

                uint32 spellId;
                switch (caster->GetPowerType())
                {
                    case POWER_MANA:
                        spellId = Mana;
                        break;
                    case POWER_ENERGY:
                        spellId = Energy;
                        break;
                    case POWER_RAGE:
                        spellId = Rage;
                        break;
                    // Death Knights can't use daggers, but oh well
                    case POWER_RUNIC_POWER:
                        spellId = RunicPower;
                        break;
                    default:
                        return;
                }

                caster->CastSpell(nullptr, spellId, aurEff);
            }

            void Register() override
            {
                OnEffectProc.Register(&spell_item_heartpierce_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_heartpierce_AuraScript<EnergySpellId, ManaSpellId, RageSpellId, RPSpellId>();
        }
};

// 40971 - Bonus Healing (Crystal Spire of Karabor)
class spell_item_crystal_spire_of_karabor : public SpellScriptLoader
{
    public:
        spell_item_crystal_spire_of_karabor() : SpellScriptLoader("spell_item_crystal_spire_of_karabor") { }

        class spell_item_crystal_spire_of_karabor_AuraScript : public AuraScript
        {
            bool CheckProc(ProcEventInfo& eventInfo)
            {
                int32 pct = GetSpellInfo()->Effects[EFFECT_0].BasePoints;
                if (HealInfo* healInfo = eventInfo.GetHealInfo())
                    if (Unit* healTarget = healInfo->GetTarget())
                        if (healTarget->GetHealth() - healInfo->GetEffectiveHeal() <= healTarget->CountPctFromMaxHealth(pct))
                            return true;

                return false;
            }

            void Register() override
            {
                DoCheckProc.Register(&spell_item_crystal_spire_of_karabor_AuraScript::CheckProc);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_crystal_spire_of_karabor_AuraScript();
        }
};

// http://www.wowhead.com/item=27388 Mr. Pinchy
// 33060 Make a Wish
enum MakeAWish
{
    SPELL_MR_PINCHYS_BLESSING       = 33053,
    SPELL_SUMMON_MIGHTY_MR_PINCHY   = 33057,
    SPELL_SUMMON_FURIOUS_MR_PINCHY  = 33059,
    SPELL_TINY_MAGICAL_CRAWDAD      = 33062,
    SPELL_MR_PINCHYS_GIFT           = 33064,
};

class spell_item_make_a_wish : public SpellScriptLoader
{
    public:
        spell_item_make_a_wish() : SpellScriptLoader("spell_item_make_a_wish") { }

        class spell_item_make_a_wish_SpellScript : public SpellScript
        {
            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_MR_PINCHYS_BLESSING,
                    SPELL_SUMMON_MIGHTY_MR_PINCHY,
                    SPELL_SUMMON_FURIOUS_MR_PINCHY,
                    SPELL_TINY_MAGICAL_CRAWDAD,
                    SPELL_MR_PINCHYS_GIFT
                });
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                uint32 spellId = SPELL_MR_PINCHYS_GIFT;
                switch (urand(1, 5))
                {
                    case 1: spellId = SPELL_MR_PINCHYS_BLESSING; break;
                    case 2: spellId = SPELL_SUMMON_MIGHTY_MR_PINCHY; break;
                    case 3: spellId = SPELL_SUMMON_FURIOUS_MR_PINCHY; break;
                    case 4: spellId = SPELL_TINY_MAGICAL_CRAWDAD; break;
                }
                caster->CastSpell(caster, spellId, true);
            }

            void Register() override
            {
                OnEffectHit.Register(&spell_item_make_a_wish_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_make_a_wish_SpellScript();
        }
};

enum MarkOfConquest
{
    SPELL_MARK_OF_CONQUEST_ENERGIZE     = 39599
};

// Item - 27920: Mark of Conquest
// Item - 27921: Mark of Conquest
// 33510 - Health Restore
class spell_item_mark_of_conquest : public SpellScriptLoader
{
    public:
        spell_item_mark_of_conquest() : SpellScriptLoader("spell_item_mark_of_conquest") { }

        class spell_item_mark_of_conquest_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_MARK_OF_CONQUEST_ENERGIZE });
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                if (eventInfo.GetTypeMask() & (PROC_FLAG_DEAL_RANGED_ATTACK | PROC_FLAG_DEAL_RANGED_ABILITY))
                {
                    // in that case, do not cast heal spell
                    PreventDefaultAction();
                    // but mana instead
                    eventInfo.GetActor()->CastSpell(nullptr, SPELL_MARK_OF_CONQUEST_ENERGIZE, aurEff);
                }
            }

            void Register() override
            {
                OnEffectProc.Register(&spell_item_mark_of_conquest_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_mark_of_conquest_AuraScript();
        }
};

// http://www.wowhead.com/item=32686 Mingo's Fortune Giblets
// 40802 Mingo's Fortune Generator
class spell_item_mingos_fortune_generator : public SpellScriptLoader
{
    public:
        spell_item_mingos_fortune_generator() : SpellScriptLoader("spell_item_mingos_fortune_generator") { }

        class spell_item_mingos_fortune_generator_SpellScript : public SpellScript
        {
            void HandleDummy(SpellEffIndex effIndex)
            {
                // Selecting one from Bloodstained Fortune item
                uint32 newitemid;
                switch (urand(1, 20))
                {
                    case 1:  newitemid = 32688; break;
                    case 2:  newitemid = 32689; break;
                    case 3:  newitemid = 32690; break;
                    case 4:  newitemid = 32691; break;
                    case 5:  newitemid = 32692; break;
                    case 6:  newitemid = 32693; break;
                    case 7:  newitemid = 32700; break;
                    case 8:  newitemid = 32701; break;
                    case 9:  newitemid = 32702; break;
                    case 10: newitemid = 32703; break;
                    case 11: newitemid = 32704; break;
                    case 12: newitemid = 32705; break;
                    case 13: newitemid = 32706; break;
                    case 14: newitemid = 32707; break;
                    case 15: newitemid = 32708; break;
                    case 16: newitemid = 32709; break;
                    case 17: newitemid = 32710; break;
                    case 18: newitemid = 32711; break;
                    case 19: newitemid = 32712; break;
                    case 20: newitemid = 32713; break;
                    default:
                        return;
                }

                CreateItem(effIndex, newitemid);
            }

            void Register() override
            {
                OnEffectHit.Register(&spell_item_mingos_fortune_generator_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_mingos_fortune_generator_SpellScript();
        }
};

// 71875, 71877 - Item - Black Bruise: Necrotic Touch Proc
enum NecroticTouch
{
    SPELL_ITEM_NECROTIC_TOUCH_PROC  = 71879
};

class spell_item_necrotic_touch : public SpellScriptLoader
{
    public:
        spell_item_necrotic_touch() : SpellScriptLoader("spell_item_necrotic_touch") { }

        class spell_item_necrotic_touch_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_ITEM_NECROTIC_TOUCH_PROC });
            }

            bool CheckProc(ProcEventInfo& eventInfo)
            {
                return eventInfo.GetProcTarget() && eventInfo.GetProcTarget()->IsAlive();
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();
                DamageInfo* damageInfo = eventInfo.GetDamageInfo();
                if (!damageInfo || !damageInfo->GetDamage())
                    return;

                int32 bp = CalculatePct(static_cast<int32>(damageInfo->GetDamage()), aurEff->GetAmount());
                GetTarget()->CastSpell(eventInfo.GetProcTarget(), SPELL_ITEM_NECROTIC_TOUCH_PROC, CastSpellExtraArgs(aurEff).AddSpellBP0(bp));
            }

            void Register() override
            {
                DoCheckProc.Register(&spell_item_necrotic_touch_AuraScript::CheckProc);
                OnEffectProc.Register(&spell_item_necrotic_touch_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_necrotic_touch_AuraScript();
        }
};

// http://www.wowhead.com/item=10720 Gnomish Net-o-Matic Projector
// 13120 Net-o-Matic
enum NetOMaticSpells
{
    SPELL_NET_O_MATIC_TRIGGERED1 = 16566,
    SPELL_NET_O_MATIC_TRIGGERED2 = 13119,
    SPELL_NET_O_MATIC_TRIGGERED3 = 13099,
};

class spell_item_net_o_matic : public SpellScriptLoader
{
    public:
        spell_item_net_o_matic() : SpellScriptLoader("spell_item_net_o_matic") { }

        class spell_item_net_o_matic_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_NET_O_MATIC_TRIGGERED1,
                    SPELL_NET_O_MATIC_TRIGGERED2,
                    SPELL_NET_O_MATIC_TRIGGERED3
                });
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit* target = GetHitUnit())
                {
                    uint32 spellId = SPELL_NET_O_MATIC_TRIGGERED3;
                    uint32 roll = urand(0, 99);
                    if (roll < 2)                            // 2% for 30 sec self root (off-like chance unknown)
                        spellId = SPELL_NET_O_MATIC_TRIGGERED1;
                    else if (roll < 4)                       // 2% for 20 sec root, charge to target (off-like chance unknown)
                        spellId = SPELL_NET_O_MATIC_TRIGGERED2;

                    GetCaster()->CastSpell(target, spellId, true);
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_net_o_matic_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_net_o_matic_SpellScript();
        }
};

// http://www.wowhead.com/item=8529 Noggenfogger Elixir
// 16589 Noggenfogger Elixir
enum NoggenfoggerElixirSpells
{
    SPELL_NOGGENFOGGER_ELIXIR_TRIGGERED1 = 16595,
    SPELL_NOGGENFOGGER_ELIXIR_TRIGGERED2 = 16593,
    SPELL_NOGGENFOGGER_ELIXIR_TRIGGERED3 = 16591,
};

class spell_item_noggenfogger_elixir : public SpellScriptLoader
{
    public:
        spell_item_noggenfogger_elixir() : SpellScriptLoader("spell_item_noggenfogger_elixir") { }

        class spell_item_noggenfogger_elixir_SpellScript : public SpellScript
        {
            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_NOGGENFOGGER_ELIXIR_TRIGGERED1,
                    SPELL_NOGGENFOGGER_ELIXIR_TRIGGERED2,
                    SPELL_NOGGENFOGGER_ELIXIR_TRIGGERED3
                });
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                uint32 spellId = SPELL_NOGGENFOGGER_ELIXIR_TRIGGERED3;
                switch (urand(1, 3))
                {
                    case 1: spellId = SPELL_NOGGENFOGGER_ELIXIR_TRIGGERED1; break;
                    case 2: spellId = SPELL_NOGGENFOGGER_ELIXIR_TRIGGERED2; break;
                }

                caster->CastSpell(caster, spellId, true);
            }

            void Register() override
            {
                OnEffectHit.Register(&spell_item_noggenfogger_elixir_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_noggenfogger_elixir_SpellScript();
        }
};

// 29601 - Enlightenment (Pendant of the Violet Eye)
class spell_item_pendant_of_the_violet_eye : public SpellScriptLoader
{
    public:
        spell_item_pendant_of_the_violet_eye() : SpellScriptLoader("spell_item_pendant_of_the_violet_eye") { }

        class spell_item_pendant_of_the_violet_eye_AuraScript : public AuraScript
        {
            bool CheckProc(ProcEventInfo& eventInfo)
            {
                if (SpellInfo const* spellInfo = eventInfo.GetSpellInfo())
                    return spellInfo->PowerType == POWER_MANA || (spellInfo->ManaCost != 0 && spellInfo->ManaCostPercentage != 0 && spellInfo->ManaCostPerlevel != 0);

                return false;
            }

            void Register() override
            {
                DoCheckProc.Register(&spell_item_pendant_of_the_violet_eye_AuraScript::CheckProc);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_pendant_of_the_violet_eye_AuraScript();
        }
};

enum PersistentShieldMisc
{
    SPELL_PERSISTENT_SHIELD_TRIGGERED = 26470
};

// 26467 - Persistent Shield
class spell_item_persistent_shield : public SpellScriptLoader
{
    public:
        spell_item_persistent_shield() : SpellScriptLoader("spell_item_persistent_shield") { }

        class spell_item_persistent_shield_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_PERSISTENT_SHIELD_TRIGGERED });
            }

            bool CheckProc(ProcEventInfo& eventInfo)
            {
                return eventInfo.GetHealInfo() && eventInfo.GetHealInfo()->GetHeal();
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                Unit* caster = eventInfo.GetActor();
                Unit* target = eventInfo.GetProcTarget();
                int32 bp0 = CalculatePct(eventInfo.GetHealInfo()->GetHeal(), 15);

                // Scarab Brooch does not replace stronger shields
                if (AuraEffect const* shield = target->GetAuraEffect(SPELL_PERSISTENT_SHIELD_TRIGGERED, EFFECT_0, caster->GetGUID()))
                    if (shield->GetAmount() > bp0)
                        return;

                caster->CastSpell(target, SPELL_PERSISTENT_SHIELD_TRIGGERED, CastSpellExtraArgs(aurEff).AddSpellBP0(bp0));
            }

            void Register() override
            {
                DoCheckProc.Register(&spell_item_persistent_shield_AuraScript::CheckProc);
                OnEffectProc.Register(&spell_item_persistent_shield_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_persistent_shield_AuraScript();
        }
};

enum PetHealing
{
    SPELL_HEALTH_LINK   = 37382
};

// 37381 - Pet Healing
// Hunter T5 2P Bonus
// Warlock T5 2P Bonus
class spell_item_pet_healing : public SpellScriptLoader
{
    public:
        spell_item_pet_healing() : SpellScriptLoader("spell_item_pet_healing") { }

        class spell_item_pet_healing_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_HEALTH_LINK });
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();
                DamageInfo* damageInfo = eventInfo.GetDamageInfo();
                if (!damageInfo || !damageInfo->GetDamage())
                    return;

                int32 bp = CalculatePct(static_cast<int32>(damageInfo->GetDamage()), aurEff->GetAmount());
                Unit* caster = eventInfo.GetActor();
                caster->CastSpell(nullptr, SPELL_HEALTH_LINK, CastSpellExtraArgs(aurEff).AddSpellBP0(bp));
            }

            void Register() override
            {
                OnEffectProc.Register(&spell_item_pet_healing_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_pet_healing_AuraScript();
        }
};

// 17512 - Piccolo of the Flaming Fire
class spell_item_piccolo_of_the_flaming_fire : public SpellScriptLoader
{
    public:
        spell_item_piccolo_of_the_flaming_fire() : SpellScriptLoader("spell_item_piccolo_of_the_flaming_fire") { }

        class spell_item_piccolo_of_the_flaming_fire_SpellScript : public SpellScript
        {
            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                if (Player* target = GetHitPlayer())
                    target->HandleEmoteCommand(EMOTE_STATE_DANCE);
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_piccolo_of_the_flaming_fire_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_piccolo_of_the_flaming_fire_SpellScript();
        }
};

enum PowerCircle
{
    SPELL_LIMITLESS_POWER = 45044
};

// 45043 - Power Circle (Shifting Naaru Sliver)
class spell_item_power_circle : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_LIMITLESS_POWER });
    }

    bool CheckCaster(Unit* target)
    {
        return target->GetGUID() == GetCasterGUID();
    }

    void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->CastSpell(nullptr, SPELL_LIMITLESS_POWER, true);
        if (Aura* buff = GetTarget()->GetAura(SPELL_LIMITLESS_POWER))
            buff->SetDuration(GetDuration());
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->RemoveAurasDueToSpell(SPELL_LIMITLESS_POWER);
    }

    void Register() override
    {
        DoCheckAreaTarget.Register(&spell_item_power_circle::CheckCaster);

        AfterEffectApply.Register(&spell_item_power_circle::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove.Register(&spell_item_power_circle::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// http://www.wowhead.com/item=6657 Savory Deviate Delight
// 8213 Savory Deviate Delight
enum SavoryDeviateDelight
{
    SPELL_FLIP_OUT_MALE     = 8219,
    SPELL_FLIP_OUT_FEMALE   = 8220,
    SPELL_YAAARRRR_MALE     = 8221,
    SPELL_YAAARRRR_FEMALE   = 8222,
};

class spell_item_savory_deviate_delight : public SpellScriptLoader
{
    public:
        spell_item_savory_deviate_delight() : SpellScriptLoader("spell_item_savory_deviate_delight") { }

        class spell_item_savory_deviate_delight_SpellScript : public SpellScript
        {
            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_FLIP_OUT_MALE,
                    SPELL_FLIP_OUT_FEMALE,
                    SPELL_YAAARRRR_MALE,
                    SPELL_YAAARRRR_FEMALE
                });
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                uint32 spellId = 0;
                switch (urand(1, 2))
                {
                    // Flip Out - ninja
                    case 1: spellId = (caster->getGender() == GENDER_MALE ? SPELL_FLIP_OUT_MALE : SPELL_FLIP_OUT_FEMALE); break;
                    // Yaaarrrr - pirate
                    case 2: spellId = (caster->getGender() == GENDER_MALE ? SPELL_YAAARRRR_MALE : SPELL_YAAARRRR_FEMALE); break;
                }
                caster->CastSpell(caster, spellId, true);
            }

            void Register() override
            {
                OnEffectHit.Register(&spell_item_savory_deviate_delight_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_savory_deviate_delight_SpellScript();
        }
};

// 48129 - Scroll of Recall
// 60320 - Scroll of Recall II
// 60321 - Scroll of Recall III
enum ScrollOfRecall
{
    SPELL_SCROLL_OF_RECALL_I                = 48129,
    SPELL_SCROLL_OF_RECALL_II               = 60320,
    SPELL_SCROLL_OF_RECALL_III              = 60321,
    SPELL_LOST                              = 60444,
    SPELL_SCROLL_OF_RECALL_FAIL_ALLIANCE_1  = 60323,
    SPELL_SCROLL_OF_RECALL_FAIL_HORDE_1     = 60328,
};

class spell_item_scroll_of_recall : public SpellScriptLoader
{
    public:
        spell_item_scroll_of_recall() : SpellScriptLoader("spell_item_scroll_of_recall") { }

        class spell_item_scroll_of_recall_SpellScript : public SpellScript
        {
            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            void HandleScript(SpellEffIndex effIndex)
            {
                Unit* caster = GetCaster();
                uint8 maxSafeLevel = 0;
                switch (GetSpellInfo()->Id)
                {
                    case SPELL_SCROLL_OF_RECALL_I:  // Scroll of Recall
                        maxSafeLevel = 40;
                        break;
                    case SPELL_SCROLL_OF_RECALL_II:  // Scroll of Recall II
                        maxSafeLevel = 70;
                        break;
                    case SPELL_SCROLL_OF_RECALL_III:  // Scroll of Recal III
                        maxSafeLevel = 80;
                        break;
                    default:
                        break;
                }

                if (caster->getLevel() > maxSafeLevel)
                {
                    caster->CastSpell(caster, SPELL_LOST, true);

                    // ALLIANCE from 60323 to 60330 - HORDE from 60328 to 60335
                    uint32 spellId = SPELL_SCROLL_OF_RECALL_FAIL_ALLIANCE_1;
                    if (GetCaster()->ToPlayer()->GetTeam() == HORDE)
                        spellId = SPELL_SCROLL_OF_RECALL_FAIL_HORDE_1;

                    GetCaster()->CastSpell(GetCaster(), spellId + urand(0, 7), true);

                    PreventHitDefaultEffect(effIndex);
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_scroll_of_recall_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_TELEPORT_UNITS);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_scroll_of_recall_SpellScript();
        }
};

// 71169 - Shadow's Fate (Shadowmourne questline)
enum ShadowsFate
{
    SPELL_SOUL_FEAST        = 71203,
    NPC_SINDRAGOSA          = 36853
};

class spell_item_unsated_craving : public SpellScriptLoader
{
    public:
        spell_item_unsated_craving() : SpellScriptLoader("spell_item_unsated_craving") { }

        class spell_item_unsated_craving_AuraScript : public AuraScript
        {
            bool CheckProc(ProcEventInfo& procInfo)
            {
                Unit* caster = procInfo.GetActor();
                if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
                    return false;

                Unit* target = procInfo.GetActionTarget();
                if (!target || target->GetTypeId() != TYPEID_UNIT || target->IsCritter() || (target->GetEntry() != NPC_SINDRAGOSA && target->IsSummon()))
                    return false;

                return true;
            }

            void Register() override
            {
                DoCheckProc.Register(&spell_item_unsated_craving_AuraScript::CheckProc);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_unsated_craving_AuraScript();
        }
};

class spell_item_shadows_fate : public SpellScriptLoader
{
    public:
        spell_item_shadows_fate() : SpellScriptLoader("spell_item_shadows_fate") { }

        class spell_item_shadows_fate_AuraScript : public AuraScript
        {
            void HandleProc(ProcEventInfo& procInfo)
            {
                PreventDefaultAction();

                Unit* caster = procInfo.GetActor();
                Unit* target = GetCaster();
                if (!caster || !target)
                    return;

                caster->CastSpell(target, SPELL_SOUL_FEAST, TRIGGERED_FULL_MASK);
            }

            void Register() override
            {
                OnProc.Register(&spell_item_shadows_fate_AuraScript::HandleProc);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_shadows_fate_AuraScript();
        }
};

enum Shadowmourne
{
    SPELL_SHADOWMOURNE_CHAOS_BANE_DAMAGE    = 71904,
    SPELL_SHADOWMOURNE_SOUL_FRAGMENT        = 71905,
    SPELL_SHADOWMOURNE_VISUAL_LOW           = 72521,
    SPELL_SHADOWMOURNE_VISUAL_HIGH          = 72523,
    SPELL_SHADOWMOURNE_CHAOS_BANE_BUFF      = 73422,
};

// 71903 - Item - Shadowmourne Legendary
class spell_item_shadowmourne : public SpellScriptLoader
{
    public:
        spell_item_shadowmourne() : SpellScriptLoader("spell_item_shadowmourne") { }

        class spell_item_shadowmourne_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_SHADOWMOURNE_CHAOS_BANE_DAMAGE,
                    SPELL_SHADOWMOURNE_SOUL_FRAGMENT,
                    SPELL_SHADOWMOURNE_CHAOS_BANE_BUFF
                });
            }

            bool CheckProc(ProcEventInfo& eventInfo)
            {
                if (GetTarget()->HasAura(SPELL_SHADOWMOURNE_CHAOS_BANE_BUFF)) // cant collect shards while under effect of Chaos Bane buff
                    return false;
                return eventInfo.GetProcTarget() && eventInfo.GetProcTarget()->IsAlive();
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();
                GetTarget()->CastSpell(GetTarget(), SPELL_SHADOWMOURNE_SOUL_FRAGMENT, aurEff);

                // this can't be handled in AuraScript of SoulFragments because we need to know victim
                if (Aura* soulFragments = GetTarget()->GetAura(SPELL_SHADOWMOURNE_SOUL_FRAGMENT))
                {
                    if (soulFragments->GetStackAmount() >= 10)
                    {
                        GetTarget()->CastSpell(eventInfo.GetProcTarget(), SPELL_SHADOWMOURNE_CHAOS_BANE_DAMAGE, aurEff);
                        soulFragments->Remove();
                    }
                }
            }

            void Register() override
            {
                DoCheckProc.Register(&spell_item_shadowmourne_AuraScript::CheckProc);
                OnEffectProc.Register(&spell_item_shadowmourne_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_shadowmourne_AuraScript();
        }
};

// 71905 - Soul Fragment
class spell_item_shadowmourne_soul_fragment : public SpellScriptLoader
{
    public:
        spell_item_shadowmourne_soul_fragment() : SpellScriptLoader("spell_item_shadowmourne_soul_fragment") { }

        class spell_item_shadowmourne_soul_fragment_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_SHADOWMOURNE_VISUAL_LOW,
                    SPELL_SHADOWMOURNE_VISUAL_HIGH,
                    SPELL_SHADOWMOURNE_CHAOS_BANE_BUFF
                });
            }

            void OnStackChange(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                switch (GetStackAmount())
                {
                    case 1:
                        target->CastSpell(target, SPELL_SHADOWMOURNE_VISUAL_LOW, true);
                        break;
                    case 6:
                        target->RemoveAurasDueToSpell(SPELL_SHADOWMOURNE_VISUAL_LOW);
                        target->CastSpell(target, SPELL_SHADOWMOURNE_VISUAL_HIGH, true);
                        break;
                    case 10:
                        target->RemoveAurasDueToSpell(SPELL_SHADOWMOURNE_VISUAL_HIGH);
                        target->CastSpell(target, SPELL_SHADOWMOURNE_CHAOS_BANE_BUFF, true);
                        break;
                    default:
                        break;
                }
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                target->RemoveAurasDueToSpell(SPELL_SHADOWMOURNE_VISUAL_LOW);
                target->RemoveAurasDueToSpell(SPELL_SHADOWMOURNE_VISUAL_HIGH);
            }

            void Register() override
            {
                AfterEffectApply.Register(&spell_item_shadowmourne_soul_fragment_AuraScript::OnStackChange, EFFECT_0, SPELL_AURA_MOD_STAT, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
                AfterEffectRemove.Register(&spell_item_shadowmourne_soul_fragment_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_MOD_STAT, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_shadowmourne_soul_fragment_AuraScript();
        }
};

// http://www.wowhead.com/item=7734 Six Demon Bag
// 14537 Six Demon Bag
enum SixDemonBagSpells
{
    SPELL_FROSTBOLT                 = 11538,
    SPELL_POLYMORPH                 = 14621,
    SPELL_SUMMON_FELHOUND_MINION    = 14642,
    SPELL_FIREBALL                  = 15662,
    SPELL_CHAIN_LIGHTNING           = 21179,
    SPELL_ENVELOPING_WINDS          = 25189,
};

class spell_item_six_demon_bag : public SpellScriptLoader
{
    public:
        spell_item_six_demon_bag() : SpellScriptLoader("spell_item_six_demon_bag") { }

        class spell_item_six_demon_bag_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_FROSTBOLT,
                    SPELL_POLYMORPH,
                    SPELL_SUMMON_FELHOUND_MINION,
                    SPELL_FIREBALL,
                    SPELL_CHAIN_LIGHTNING,
                    SPELL_ENVELOPING_WINDS
                });
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (Unit* target = GetHitUnit())
                {
                    uint32 spellId = 0;
                    uint32 rand = urand(0, 99);
                    if (rand < 25)                      // Fireball (25% chance)
                        spellId = SPELL_FIREBALL;
                    else if (rand < 50)                 // Frostball (25% chance)
                        spellId = SPELL_FROSTBOLT;
                    else if (rand < 70)                 // Chain Lighting (20% chance)
                        spellId = SPELL_CHAIN_LIGHTNING;
                    else if (rand < 80)                 // Polymorph (10% chance)
                    {
                        spellId = SPELL_POLYMORPH;
                        if (urand(0, 100) <= 30)        // 30% chance to self-cast
                            target = caster;
                    }
                    else if (rand < 95)                 // Enveloping Winds (15% chance)
                        spellId = SPELL_ENVELOPING_WINDS;
                    else                                // Summon Felhund minion (5% chance)
                    {
                        spellId = SPELL_SUMMON_FELHOUND_MINION;
                        target = caster;
                    }

                    caster->CastSpell(target, spellId, GetCastItem());
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_six_demon_bag_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_six_demon_bag_SpellScript();
        }
};

enum SwiftHandJusticeMisc
{
    SPELL_SWIFT_HAND_OF_JUSTICE_HEAL = 59913
};

// 59906 - Swift Hand of Justice Dummy
class spell_item_swift_hand_justice_dummy : public SpellScriptLoader
{
    public:
        spell_item_swift_hand_justice_dummy() : SpellScriptLoader("spell_item_swift_hand_justice_dummy") { }

        class spell_item_swift_hand_justice_dummy_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_SWIFT_HAND_OF_JUSTICE_HEAL });
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();

                Unit* caster = eventInfo.GetActor();
                int32 amount = caster->CountPctFromMaxHealth(aurEff->GetAmount());
                caster->CastSpell(nullptr, SPELL_SWIFT_HAND_OF_JUSTICE_HEAL, CastSpellExtraArgs(aurEff).AddSpellBP0(amount));
            }

            void Register() override
            {
                OnEffectProc.Register(&spell_item_swift_hand_justice_dummy_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_swift_hand_justice_dummy_AuraScript();
        }
};

enum TotemOfFlowingWater
{
    SPELL_LESSER_HEALING_WAVE_MANA      = 28850
};

// Item - 23005: Totem of Flowing Water
// 28849 - Lesser Healing Wave
class spell_item_totem_of_flowing_water : public SpellScriptLoader
{
    public:
        spell_item_totem_of_flowing_water() : SpellScriptLoader("spell_item_totem_of_flowing_water") { }

        class spell_item_totem_of_flowing_water_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_LESSER_HEALING_WAVE_MANA });
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();
                eventInfo.GetActor()->CastSpell(nullptr, SPELL_LESSER_HEALING_WAVE_MANA,  aurEff);
            }

            void Register() override
            {
                OnEffectProc.Register(&spell_item_totem_of_flowing_water_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_totem_of_flowing_water_AuraScript();
        }

};

// 28862 - The Eye of Diminution
class spell_item_the_eye_of_diminution : public SpellScriptLoader
{
    public:
        spell_item_the_eye_of_diminution() : SpellScriptLoader("spell_item_the_eye_of_diminution") { }

        class spell_item_the_eye_of_diminution_AuraScript : public AuraScript
        {
            void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
            {
                int32 diff = GetUnitOwner()->getLevel() - 60;
                if (diff > 0)
                    amount += diff;
            }

            void Register() override
            {
                DoEffectCalcAmount.Register(&spell_item_the_eye_of_diminution_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_MOD_THREAT);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_the_eye_of_diminution_AuraScript();
        }
};

// http://www.wowhead.com/item=44012 Underbelly Elixir
// 59640 Underbelly Elixir
enum UnderbellyElixirSpells
{
    SPELL_UNDERBELLY_ELIXIR_TRIGGERED1 = 59645,
    SPELL_UNDERBELLY_ELIXIR_TRIGGERED2 = 59831,
    SPELL_UNDERBELLY_ELIXIR_TRIGGERED3 = 59843,
};

class spell_item_underbelly_elixir : public SpellScriptLoader
{
    public:
        spell_item_underbelly_elixir() : SpellScriptLoader("spell_item_underbelly_elixir") { }

        class spell_item_underbelly_elixir_SpellScript : public SpellScript
        {
            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_UNDERBELLY_ELIXIR_TRIGGERED1,
                    SPELL_UNDERBELLY_ELIXIR_TRIGGERED2,
                    SPELL_UNDERBELLY_ELIXIR_TRIGGERED3
                });
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                uint32 spellId = SPELL_UNDERBELLY_ELIXIR_TRIGGERED3;
                switch (urand(1, 3))
                {
                    case 1: spellId = SPELL_UNDERBELLY_ELIXIR_TRIGGERED1; break;
                    case 2: spellId = SPELL_UNDERBELLY_ELIXIR_TRIGGERED2; break;
                }
                caster->CastSpell(caster, spellId, true);
            }

            void Register() override
            {
                OnEffectHit.Register(&spell_item_underbelly_elixir_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_underbelly_elixir_SpellScript();
        }
};

// 47776 - Roll 'dem Bones
class spell_item_worn_troll_dice : public SpellScriptLoader
{
    public:
        spell_item_worn_troll_dice() : SpellScriptLoader("spell_item_worn_troll_dice") { }

        class spell_item_worn_troll_dice_SpellScript : public SpellScript
        {
            enum
            {
                TEXT_WORN_TROLL_DICE = 26152
            };

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                if (!sObjectMgr->GetBroadcastText(TEXT_WORN_TROLL_DICE))
                    return false;
                return true;
            }

            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                GetCaster()->TextEmote(TEXT_WORN_TROLL_DICE, GetHitUnit());

                static uint32 const minimum = 1;
                static uint32 const maximum = 6;

                // roll twice
                GetCaster()->ToPlayer()->DoRandomRoll(minimum, maximum);
                GetCaster()->ToPlayer()->DoRandomRoll(minimum, maximum);
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_worn_troll_dice_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_worn_troll_dice_SpellScript();
        }
};

enum AirRifleSpells
{
    SPELL_AIR_RIFLE_HOLD_VISUAL = 65582,
    SPELL_AIR_RIFLE_SHOOT       = 67532,
    SPELL_AIR_RIFLE_SHOOT_SELF  = 65577,
};

class spell_item_red_rider_air_rifle : public SpellScriptLoader
{
    public:
        spell_item_red_rider_air_rifle() : SpellScriptLoader("spell_item_red_rider_air_rifle") { }

        class spell_item_red_rider_air_rifle_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spell*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_AIR_RIFLE_HOLD_VISUAL,
                    SPELL_AIR_RIFLE_SHOOT,
                    SPELL_AIR_RIFLE_SHOOT_SELF
                });
            }

            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                Unit* caster = GetCaster();
                if (Unit* target = GetHitUnit())
                {
                    caster->CastSpell(caster, SPELL_AIR_RIFLE_HOLD_VISUAL, true);
                    // needed because this spell shares GCD with its triggered spells (which must not be cast with triggered flag)
                    if (Player* player = caster->ToPlayer())
                        player->GetSpellHistory()->CancelGlobalCooldown(GetSpellInfo());
                    if (urand(0, 4))
                        caster->CastSpell(target, SPELL_AIR_RIFLE_SHOOT, false);
                    else
                        caster->CastSpell(caster, SPELL_AIR_RIFLE_SHOOT_SELF, false);
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_red_rider_air_rifle_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_red_rider_air_rifle_SpellScript();
        }
};

enum GenericData
{
    SPELL_ARCANITE_DRAGONLING           = 19804,
    SPELL_BATTLE_CHICKEN                = 13166,
    SPELL_MECHANICAL_DRAGONLING         = 4073,
    SPELL_MITHRIL_MECHANICAL_DRAGONLING = 12749,
};

enum CreateHeartCandy
{
    ITEM_HEART_CANDY_1 = 21818,
    ITEM_HEART_CANDY_2 = 21817,
    ITEM_HEART_CANDY_3 = 21821,
    ITEM_HEART_CANDY_4 = 21819,
    ITEM_HEART_CANDY_5 = 21816,
    ITEM_HEART_CANDY_6 = 21823,
    ITEM_HEART_CANDY_7 = 21822,
    ITEM_HEART_CANDY_8 = 21820,
};

class spell_item_create_heart_candy : public SpellScriptLoader
{
    public:
        spell_item_create_heart_candy() : SpellScriptLoader("spell_item_create_heart_candy") { }

        class spell_item_create_heart_candy_SpellScript : public SpellScript
        {
            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                if (Player* target = GetHitPlayer())
                {
                    static const uint32 items[] = {ITEM_HEART_CANDY_1, ITEM_HEART_CANDY_2, ITEM_HEART_CANDY_3, ITEM_HEART_CANDY_4, ITEM_HEART_CANDY_5, ITEM_HEART_CANDY_6, ITEM_HEART_CANDY_7, ITEM_HEART_CANDY_8};
                    target->AddItem(items[urand(0, 7)], 1);
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_create_heart_candy_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_create_heart_candy_SpellScript();
        }
};

class spell_item_book_of_glyph_mastery : public SpellScriptLoader
{
    public:
        spell_item_book_of_glyph_mastery() : SpellScriptLoader("spell_item_book_of_glyph_mastery") { }

        class spell_item_book_of_glyph_mastery_SpellScript : public SpellScript
        {
            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            SpellCastResult CheckRequirement()
            {
                if (HasDiscoveredAllSpells(GetSpellInfo()->Id, GetCaster()->ToPlayer()))
                {
                    SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_LEARNED_EVERYTHING);
                    return SPELL_FAILED_CUSTOM_ERROR;
                }

                return SPELL_CAST_OK;
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                Player* caster = GetCaster()->ToPlayer();
                uint32 spellId = GetSpellInfo()->Id;

                // learn random explicit discovery recipe (if any)
                if (uint32 discoveredSpellId = GetExplicitDiscoverySpell(spellId, caster))
                    caster->LearnSpell(discoveredSpellId, false);
            }

            void Register() override
            {
                OnCheckCast.Register(&spell_item_book_of_glyph_mastery_SpellScript::CheckRequirement);
                OnEffectHitTarget.Register(&spell_item_book_of_glyph_mastery_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_book_of_glyph_mastery_SpellScript();
        }
};

enum GiftOfTheHarvester
{
    NPC_GHOUL   = 28845,
    MAX_GHOULS  = 5,
};

class spell_item_gift_of_the_harvester : public SpellScriptLoader
{
    public:
        spell_item_gift_of_the_harvester() : SpellScriptLoader("spell_item_gift_of_the_harvester") { }

        class spell_item_gift_of_the_harvester_SpellScript : public SpellScript
        {
            SpellCastResult CheckRequirement()
            {
                std::list<Creature*> ghouls;
                GetCaster()->GetAllMinionsByEntry(ghouls, NPC_GHOUL);
                if (ghouls.size() >= MAX_GHOULS)
                {
                    SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_TOO_MANY_GHOULS);
                    return SPELL_FAILED_CUSTOM_ERROR;
                }

                return SPELL_CAST_OK;
            }

            void Register() override
            {
                OnCheckCast.Register(&spell_item_gift_of_the_harvester_SpellScript::CheckRequirement);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_gift_of_the_harvester_SpellScript();
        }
};

enum Sinkholes
{
    NPC_SOUTH_SINKHOLE      = 25664,
    NPC_NORTHEAST_SINKHOLE  = 25665,
    NPC_NORTHWEST_SINKHOLE  = 25666,
};

class spell_item_map_of_the_geyser_fields : public SpellScriptLoader
{
    public:
        spell_item_map_of_the_geyser_fields() : SpellScriptLoader("spell_item_map_of_the_geyser_fields") { }

        class spell_item_map_of_the_geyser_fields_SpellScript : public SpellScript
        {
            SpellCastResult CheckSinkholes()
            {
                Unit* caster = GetCaster();
                if (caster->FindNearestCreature(NPC_SOUTH_SINKHOLE, 30.0f, true) ||
                    caster->FindNearestCreature(NPC_NORTHEAST_SINKHOLE, 30.0f, true) ||
                    caster->FindNearestCreature(NPC_NORTHWEST_SINKHOLE, 30.0f, true))
                    return SPELL_CAST_OK;

                SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_MUST_BE_CLOSE_TO_SINKHOLE);
                return SPELL_FAILED_CUSTOM_ERROR;
            }

            void Register() override
            {
                OnCheckCast.Register(&spell_item_map_of_the_geyser_fields_SpellScript::CheckSinkholes);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_map_of_the_geyser_fields_SpellScript();
        }
};

enum VanquishedClutchesSpells
{
    SPELL_CRUSHER       = 64982,
    SPELL_CONSTRICTOR   = 64983,
    SPELL_CORRUPTOR     = 64984,
};

class spell_item_vanquished_clutches : public SpellScriptLoader
{
    public:
        spell_item_vanquished_clutches() : SpellScriptLoader("spell_item_vanquished_clutches") { }

        class spell_item_vanquished_clutches_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_CRUSHER,
                    SPELL_CONSTRICTOR,
                    SPELL_CORRUPTOR
                });
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                uint32 spellId = RAND(SPELL_CRUSHER, SPELL_CONSTRICTOR, SPELL_CORRUPTOR);
                Unit* caster = GetCaster();
                caster->CastSpell(caster, spellId, true);
            }

            void Register() override
            {
                OnEffectHit.Register(&spell_item_vanquished_clutches_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_vanquished_clutches_SpellScript();
        }
};

enum AshbringerSounds
{
    SOUND_ASHBRINGER_1  = 8906,                             // "I was pure once"
    SOUND_ASHBRINGER_2  = 8907,                             // "Fought for righteousness"
    SOUND_ASHBRINGER_3  = 8908,                             // "I was once called Ashbringer"
    SOUND_ASHBRINGER_4  = 8920,                             // "Betrayed by my order"
    SOUND_ASHBRINGER_5  = 8921,                             // "Destroyed by Kel'Thuzad"
    SOUND_ASHBRINGER_6  = 8922,                             // "Made to serve"
    SOUND_ASHBRINGER_7  = 8923,                             // "My son watched me die"
    SOUND_ASHBRINGER_8  = 8924,                             // "Crusades fed his rage"
    SOUND_ASHBRINGER_9  = 8925,                             // "Truth is unknown to him"
    SOUND_ASHBRINGER_10 = 8926,                             // "Scarlet Crusade  is pure no longer"
    SOUND_ASHBRINGER_11 = 8927,                             // "Balnazzar's crusade corrupted my son"
    SOUND_ASHBRINGER_12 = 8928,                             // "Kill them all!"
};

class spell_item_ashbringer : public SpellScriptLoader
{
    public:
        spell_item_ashbringer() : SpellScriptLoader("spell_item_ashbringer") { }

        class spell_item_ashbringer_SpellScript : public SpellScript
        {
            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            void OnDummyEffect(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);

                Player* player = GetCaster()->ToPlayer();
                uint32 sound_id = RAND( SOUND_ASHBRINGER_1, SOUND_ASHBRINGER_2, SOUND_ASHBRINGER_3, SOUND_ASHBRINGER_4, SOUND_ASHBRINGER_5, SOUND_ASHBRINGER_6,
                                SOUND_ASHBRINGER_7, SOUND_ASHBRINGER_8, SOUND_ASHBRINGER_9, SOUND_ASHBRINGER_10, SOUND_ASHBRINGER_11, SOUND_ASHBRINGER_12 );

                // Ashbringers effect (spellID 28441) retriggers every 5 seconds, with a chance of making it say one of the above 12 sounds
                if (urand(0, 60) < 1)
                    player->PlayDirectSound(sound_id, player);
            }

            void Register() override
            {
                OnEffectHit.Register(&spell_item_ashbringer_SpellScript::OnDummyEffect, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_ashbringer_SpellScript();
        }
};

enum MagicEater
{
    SPELL_WILD_MAGIC                             = 58891,
    SPELL_WELL_FED_1                             = 57288,
    SPELL_WELL_FED_2                             = 57139,
    SPELL_WELL_FED_3                             = 57111,
    SPELL_WELL_FED_4                             = 57286,
    SPELL_WELL_FED_5                             = 57291,
};

class spell_magic_eater_food : public SpellScriptLoader
{
    public:
        spell_magic_eater_food() : SpellScriptLoader("spell_magic_eater_food") { }

        class spell_magic_eater_food_AuraScript : public AuraScript
        {
            void HandleTriggerSpell(AuraEffect const* /*aurEff*/)
            {
                PreventDefaultAction();
                Unit* target = GetTarget();
                switch (urand(0, 5))
                {
                    case 0:
                        target->CastSpell(target, SPELL_WILD_MAGIC, true);
                        break;
                    case 1:
                        target->CastSpell(target, SPELL_WELL_FED_1, true);
                        break;
                    case 2:
                        target->CastSpell(target, SPELL_WELL_FED_2, true);
                        break;
                    case 3:
                        target->CastSpell(target, SPELL_WELL_FED_3, true);
                        break;
                    case 4:
                        target->CastSpell(target, SPELL_WELL_FED_4, true);
                        break;
                    case 5:
                        target->CastSpell(target, SPELL_WELL_FED_5, true);
                        break;
                }
            }

            void Register() override
            {
                OnEffectPeriodic.Register(&spell_magic_eater_food_AuraScript::HandleTriggerSpell, EFFECT_1, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_magic_eater_food_AuraScript();
        }
};

class spell_item_shimmering_vessel : public SpellScriptLoader
{
    public:
        spell_item_shimmering_vessel() : SpellScriptLoader("spell_item_shimmering_vessel") { }

        class spell_item_shimmering_vessel_SpellScript : public SpellScript
        {
            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                if (Creature* target = GetHitCreature())
                    target->setDeathState(JUST_RESPAWNED);
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_shimmering_vessel_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_shimmering_vessel_SpellScript();
        }
};

enum PurifyHelboarMeat
{
    SPELL_SUMMON_PURIFIED_HELBOAR_MEAT      = 29277,
    SPELL_SUMMON_TOXIC_HELBOAR_MEAT         = 29278,
};

class spell_item_purify_helboar_meat : public SpellScriptLoader
{
    public:
        spell_item_purify_helboar_meat() : SpellScriptLoader("spell_item_purify_helboar_meat") { }

        class spell_item_purify_helboar_meat_SpellScript : public SpellScript
        {
            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            bool Validate(SpellInfo const* /*spell*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_SUMMON_PURIFIED_HELBOAR_MEAT,
                    SPELL_SUMMON_TOXIC_HELBOAR_MEAT
                });
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                Unit* caster = GetCaster();
                caster->CastSpell(caster, roll_chance_i(50) ? SPELL_SUMMON_PURIFIED_HELBOAR_MEAT : SPELL_SUMMON_TOXIC_HELBOAR_MEAT, true);
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_purify_helboar_meat_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_purify_helboar_meat_SpellScript();
        }
};

enum CrystalPrison
{
    OBJECT_IMPRISONED_DOOMGUARD     = 179644,
};

class spell_item_crystal_prison_dummy_dnd : public SpellScriptLoader
{
    public:
        spell_item_crystal_prison_dummy_dnd() : SpellScriptLoader("spell_item_crystal_prison_dummy_dnd") { }

        class spell_item_crystal_prison_dummy_dnd_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spell*/) override
            {
                if (!sObjectMgr->GetGameObjectTemplate(OBJECT_IMPRISONED_DOOMGUARD))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                if (Creature* target = GetHitCreature())
                    if (target->isDead() && !target->IsPet())
                    {
                        GetCaster()->SummonGameObject(OBJECT_IMPRISONED_DOOMGUARD, *target, QuaternionData(), uint32(target->GetRespawnTime() - GameTime::GetGameTime()));
                        target->DespawnOrUnsummon();
                    }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_crystal_prison_dummy_dnd_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_crystal_prison_dummy_dnd_SpellScript();
        }
};

enum ReindeerTransformation
{
    SPELL_FLYING_REINDEER_310                   = 44827,
    SPELL_FLYING_REINDEER_280                   = 44825,
    SPELL_FLYING_REINDEER_60                    = 44824,
    SPELL_REINDEER_100                          = 25859,
    SPELL_REINDEER_60                           = 25858,
};

class spell_item_reindeer_transformation : public SpellScriptLoader
{
    public:
        spell_item_reindeer_transformation() : SpellScriptLoader("spell_item_reindeer_transformation") { }

        class spell_item_reindeer_transformation_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spell*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_FLYING_REINDEER_310,
                    SPELL_FLYING_REINDEER_280,
                    SPELL_FLYING_REINDEER_60,
                    SPELL_REINDEER_100,
                    SPELL_REINDEER_60
                });
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                Unit* caster = GetCaster();
                if (caster->HasAuraType(SPELL_AURA_MOUNTED))
                {
                    float flyspeed = caster->GetSpeedRate(MOVE_FLIGHT);
                    float speed = caster->GetSpeedRate(MOVE_RUN);

                    caster->RemoveAurasByType(SPELL_AURA_MOUNTED);
                    //5 different spells used depending on mounted speed and if mount can fly or not

                    if (flyspeed >= 4.1f)
                        // Flying Reindeer
                        caster->CastSpell(caster, SPELL_FLYING_REINDEER_310, true); //310% flying Reindeer
                    else if (flyspeed >= 3.8f)
                        // Flying Reindeer
                        caster->CastSpell(caster, SPELL_FLYING_REINDEER_280, true); //280% flying Reindeer
                    else if (flyspeed >= 1.6f)
                        // Flying Reindeer
                        caster->CastSpell(caster, SPELL_FLYING_REINDEER_60, true); //60% flying Reindeer
                    else if (speed >= 2.0f)
                        // Reindeer
                        caster->CastSpell(caster, SPELL_REINDEER_100, true); //100% ground Reindeer
                    else
                        // Reindeer
                        caster->CastSpell(caster, SPELL_REINDEER_60, true); //60% ground Reindeer
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_reindeer_transformation_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_item_reindeer_transformation_SpellScript();
    }
};

enum NighInvulnerability
{
    SPELL_NIGH_INVULNERABILITY                  = 30456,
    SPELL_COMPLETE_VULNERABILITY                = 30457,
};

class spell_item_nigh_invulnerability : public SpellScriptLoader
{
    public:
        spell_item_nigh_invulnerability() : SpellScriptLoader("spell_item_nigh_invulnerability") { }

        class spell_item_nigh_invulnerability_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spell*/) override
            {
                return ValidateSpellInfo({ SPELL_NIGH_INVULNERABILITY, SPELL_COMPLETE_VULNERABILITY });
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                Unit* caster = GetCaster();
                if (Item* castItem = GetCastItem())
                {
                    if (roll_chance_i(86))                  // Nigh-Invulnerability   - success
                        caster->CastSpell(caster, SPELL_NIGH_INVULNERABILITY, castItem);
                    else                                    // Complete Vulnerability - backfire in 14% casts
                        caster->CastSpell(caster, SPELL_COMPLETE_VULNERABILITY, castItem);
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_nigh_invulnerability_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_nigh_invulnerability_SpellScript();
        }
};

enum Poultryzer
{
    SPELL_POULTRYIZER_SUCCESS    = 30501,
    SPELL_POULTRYIZER_BACKFIRE   = 30504,
};

class spell_item_poultryizer : public SpellScriptLoader
{
    public:
        spell_item_poultryizer() : SpellScriptLoader("spell_item_poultryizer") { }

        class spell_item_poultryizer_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spell*/) override
            {
                return ValidateSpellInfo({ SPELL_POULTRYIZER_SUCCESS, SPELL_POULTRYIZER_BACKFIRE });
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                if (GetCastItem() && GetHitUnit())
                    GetCaster()->CastSpell(GetHitUnit(), roll_chance_i(80) ? SPELL_POULTRYIZER_SUCCESS : SPELL_POULTRYIZER_BACKFIRE, GetCastItem());
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_poultryizer_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_poultryizer_SpellScript();
        }
};

enum SocretharsStone
{
    SPELL_SOCRETHAR_TO_SEAT     = 35743,
    SPELL_SOCRETHAR_FROM_SEAT   = 35744,
};

class spell_item_socrethars_stone : public SpellScriptLoader
{
    public:
        spell_item_socrethars_stone() : SpellScriptLoader("spell_item_socrethars_stone") { }

        class spell_item_socrethars_stone_SpellScript : public SpellScript
        {
            bool Load() override
            {
                return (GetCaster()->GetAreaId() == 3900 || GetCaster()->GetAreaId() == 3742);
            }
            bool Validate(SpellInfo const* /*spell*/) override
            {
                return ValidateSpellInfo({ SPELL_SOCRETHAR_TO_SEAT, SPELL_SOCRETHAR_FROM_SEAT });
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                Unit* caster = GetCaster();
                switch (caster->GetAreaId())
                {
                    case 3900:
                        caster->CastSpell(caster, SPELL_SOCRETHAR_TO_SEAT, true);
                        break;
                    case 3742:
                        caster->CastSpell(caster, SPELL_SOCRETHAR_FROM_SEAT, true);
                        break;
                    default:
                        return;
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_socrethars_stone_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_socrethars_stone_SpellScript();
        }
};

enum DemonBroiledSurprise
{
    QUEST_SUPER_HOT_STEW                    = 11379,
    SPELL_CREATE_DEMON_BROILED_SURPRISE     = 43753,
    NPC_ABYSSAL_FLAMEBRINGER                = 19973,
};

class spell_item_demon_broiled_surprise : public SpellScriptLoader
{
    public:
        spell_item_demon_broiled_surprise() : SpellScriptLoader("spell_item_demon_broiled_surprise") { }

        class spell_item_demon_broiled_surprise_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spell*/) override
            {
                return ValidateSpellInfo({ SPELL_CREATE_DEMON_BROILED_SURPRISE }) &&
                    sObjectMgr->GetCreatureTemplate(NPC_ABYSSAL_FLAMEBRINGER) &&
                    sObjectMgr->GetQuestTemplate(QUEST_SUPER_HOT_STEW);
            }

            bool Load() override
            {
               return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                Unit* player = GetCaster();
                player->CastSpell(player, SPELL_CREATE_DEMON_BROILED_SURPRISE, false);
            }

            SpellCastResult CheckRequirement()
            {
                Player* player = GetCaster()->ToPlayer();
                if (player->GetQuestStatus(QUEST_SUPER_HOT_STEW) != QUEST_STATUS_INCOMPLETE)
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

                if (Creature* creature = player->FindNearestCreature(NPC_ABYSSAL_FLAMEBRINGER, 10, false))
                    if (creature->isDead())
                        return SPELL_CAST_OK;
                return SPELL_FAILED_NOT_HERE;
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_demon_broiled_surprise_SpellScript::HandleDummy, EFFECT_1, SPELL_EFFECT_DUMMY);
                OnCheckCast.Register(&spell_item_demon_broiled_surprise_SpellScript::CheckRequirement);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_demon_broiled_surprise_SpellScript();
        }
};

enum CompleteRaptorCapture
{
    SPELL_RAPTOR_CAPTURE_CREDIT     = 42337,
};

class spell_item_complete_raptor_capture : public SpellScriptLoader
{
    public:
        spell_item_complete_raptor_capture() : SpellScriptLoader("spell_item_complete_raptor_capture") { }

        class spell_item_complete_raptor_capture_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spell*/) override
            {
                return ValidateSpellInfo({ SPELL_RAPTOR_CAPTURE_CREDIT });
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                Unit* caster = GetCaster();
                if (GetHitCreature())
                {
                    GetHitCreature()->DespawnOrUnsummon();

                    //cast spell Raptor Capture Credit
                    caster->CastSpell(caster, SPELL_RAPTOR_CAPTURE_CREDIT, true);
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_complete_raptor_capture_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_complete_raptor_capture_SpellScript();
        }
};

enum ImpaleLeviroth
{
    NPC_LEVIROTH                = 26452,
    SPELL_LEVIROTH_SELF_IMPALE  = 49882
};

class spell_item_impale_leviroth : public SpellScriptLoader
{
    public:
        spell_item_impale_leviroth() : SpellScriptLoader("spell_item_impale_leviroth") { }

        class spell_item_impale_leviroth_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spell*/) override
            {
                if (!sObjectMgr->GetCreatureTemplate(NPC_LEVIROTH))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Creature* target = GetHitCreature())
                    if (target->GetEntry() == NPC_LEVIROTH && !target->HealthBelowPct(95))
                    {
                        target->CastSpell(target, SPELL_LEVIROTH_SELF_IMPALE, true);
                        target->ResetPlayerDamageReq();
                    }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_impale_leviroth_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_impale_leviroth_SpellScript();
        }
};

enum BrewfestMountTransformation
{
    SPELL_MOUNT_RAM_100                         = 43900,
    SPELL_MOUNT_RAM_60                          = 43899,
    SPELL_MOUNT_KODO_100                        = 49379,
    SPELL_MOUNT_KODO_60                         = 49378,
    SPELL_BREWFEST_MOUNT_TRANSFORM              = 49357,
    SPELL_BREWFEST_MOUNT_TRANSFORM_REVERSE      = 52845,
};

class spell_item_brewfest_mount_transformation : public SpellScriptLoader
{
    public:
        spell_item_brewfest_mount_transformation() : SpellScriptLoader("spell_item_brewfest_mount_transformation") { }

        class spell_item_brewfest_mount_transformation_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spell*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_MOUNT_RAM_100,
                    SPELL_MOUNT_RAM_60,
                    SPELL_MOUNT_KODO_100,
                    SPELL_MOUNT_KODO_60
                });
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                Player* caster = GetCaster()->ToPlayer();
                if (caster->HasAuraType(SPELL_AURA_MOUNTED))
                {
                    caster->RemoveAurasByType(SPELL_AURA_MOUNTED);
                    uint32 spell_id;

                    switch (GetSpellInfo()->Id)
                    {
                        case SPELL_BREWFEST_MOUNT_TRANSFORM:
                            if (caster->GetSpeedRate(MOVE_RUN) >= 2.0f)
                                spell_id = caster->GetTeam() == ALLIANCE ? SPELL_MOUNT_RAM_100 : SPELL_MOUNT_KODO_100;
                            else
                                spell_id = caster->GetTeam() == ALLIANCE ? SPELL_MOUNT_RAM_60 : SPELL_MOUNT_KODO_60;
                            break;
                        case SPELL_BREWFEST_MOUNT_TRANSFORM_REVERSE:
                            if (caster->GetSpeedRate(MOVE_RUN) >= 2.0f)
                                spell_id = caster->GetTeam() == HORDE ? SPELL_MOUNT_RAM_100 : SPELL_MOUNT_KODO_100;
                            else
                                spell_id = caster->GetTeam() == HORDE ? SPELL_MOUNT_RAM_60 : SPELL_MOUNT_KODO_60;
                            break;
                        default:
                            return;
                    }
                    caster->CastSpell(caster, spell_id, true);
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_brewfest_mount_transformation_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_brewfest_mount_transformation_SpellScript();
        }
};

enum NitroBoosts
{
    SPELL_NITRO_BOOSTS_SUCCESS       = 54861,
    SPELL_NITRO_BOOSTS_BACKFIRE      = 54621,
    SPELL_NITRO_BOOSTS_PARACHUTE     = 54649,
};

class spell_item_nitro_boosts : public SpellScriptLoader
{
    public:
        spell_item_nitro_boosts() : SpellScriptLoader("spell_item_nitro_boosts") { }

        class spell_item_nitro_boosts_SpellScript : public SpellScript
        {
            bool Load() override
            {
                if (!GetCastItem())
                    return false;
                return true;
            }

            bool Validate(SpellInfo const* /*spell*/) override
            {
                return ValidateSpellInfo({ SPELL_NITRO_BOOSTS_SUCCESS, SPELL_NITRO_BOOSTS_BACKFIRE });
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                Unit* caster = GetCaster();
                bool success = true;
                if (!caster->GetMap()->IsDungeon())
                    success = roll_chance_i(95); // nitro boosts can only fail in flying-enabled locations on 4.3.4
                caster->CastSpell(caster, success ? SPELL_NITRO_BOOSTS_SUCCESS : SPELL_NITRO_BOOSTS_BACKFIRE, GetCastItem());
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_nitro_boosts_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_nitro_boosts_SpellScript();
        }
};

class spell_item_nitro_boosts_backfire : public SpellScriptLoader
{
    public:
        spell_item_nitro_boosts_backfire() : SpellScriptLoader("spell_item_nitro_boosts_backfire") { }

        class spell_item_nitro_boosts_backfire_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spell*/) override
            {
                return ValidateSpellInfo({ SPELL_NITRO_BOOSTS_PARACHUTE });
            }

            void HandleApply(AuraEffect const* /*effect*/, AuraEffectHandleModes /*mode*/)
            {
                lastZ = GetTarget()->GetPositionZ();
            }

            void HandlePeriodicDummy(AuraEffect const* effect)
            {
                PreventDefaultAction();
                float curZ = GetTarget()->GetPositionZ();
                if (curZ < lastZ)
                {
                    if (roll_chance_i(80)) // we don't have enough sniffs to verify this, guesstimate
                        GetTarget()->CastSpell(GetTarget(), SPELL_NITRO_BOOSTS_PARACHUTE, effect);
                    GetAura()->Remove();
                }
                else
                    lastZ = curZ;
            }

            void Register() override
            {
                OnEffectApply.Register(&spell_item_nitro_boosts_backfire_AuraScript::HandleApply, EFFECT_1, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
                OnEffectPeriodic.Register(&spell_item_nitro_boosts_backfire_AuraScript::HandlePeriodicDummy, EFFECT_1, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }

            float lastZ = INVALID_HEIGHT;
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_nitro_boosts_backfire_AuraScript();
        }
};

enum TeachLanguage
{
    SPELL_LEARN_GNOMISH_BINARY      = 50242,
    SPELL_LEARN_GOBLIN_BINARY       = 50246,
};

class spell_item_teach_language : public SpellScriptLoader
{
    public:
        spell_item_teach_language() : SpellScriptLoader("spell_item_teach_language") { }

        class spell_item_teach_language_SpellScript : public SpellScript
        {
            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            bool Validate(SpellInfo const* /*spell*/) override
            {
                return ValidateSpellInfo({ SPELL_LEARN_GNOMISH_BINARY, SPELL_LEARN_GOBLIN_BINARY });
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                Player* caster = GetCaster()->ToPlayer();

                if (roll_chance_i(34))
                    caster->CastSpell(caster, caster->GetTeam() == ALLIANCE ? SPELL_LEARN_GNOMISH_BINARY : SPELL_LEARN_GOBLIN_BINARY, true);
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_teach_language_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_teach_language_SpellScript();
        }
};

enum RocketBoots
{
    SPELL_ROCKET_BOOTS_PROC      = 30452,
};

class spell_item_rocket_boots : public SpellScriptLoader
{
    public:
        spell_item_rocket_boots() : SpellScriptLoader("spell_item_rocket_boots") { }

        class spell_item_rocket_boots_SpellScript : public SpellScript
        {
            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            bool Validate(SpellInfo const* /*spell*/) override
            {
                return ValidateSpellInfo({ SPELL_ROCKET_BOOTS_PROC });
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                Player* caster = GetCaster()->ToPlayer();
                if (Battleground* bg = caster->GetBattleground())
                    bg->EventPlayerDroppedFlag(caster);

                caster->GetSpellHistory()->ResetCooldown(SPELL_ROCKET_BOOTS_PROC);
                caster->CastSpell(caster, SPELL_ROCKET_BOOTS_PROC, true);
            }

            SpellCastResult CheckCast()
            {
                if (GetCaster()->IsInWater())
                    return SPELL_FAILED_ONLY_ABOVEWATER;
                return SPELL_CAST_OK;
            }

            void Register() override
            {
                OnCheckCast.Register(&spell_item_rocket_boots_SpellScript::CheckCast);
                OnEffectHitTarget.Register(&spell_item_rocket_boots_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_rocket_boots_SpellScript();
        }
};

enum PygmyOil
{
    SPELL_PYGMY_OIL_PYGMY_AURA      = 53806,
    SPELL_PYGMY_OIL_SMALLER_AURA    = 53805,
};

class spell_item_pygmy_oil : public SpellScriptLoader
{
    public:
        spell_item_pygmy_oil() : SpellScriptLoader("spell_item_pygmy_oil") { }

        class spell_item_pygmy_oil_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spell*/) override
            {
                return ValidateSpellInfo({ SPELL_PYGMY_OIL_PYGMY_AURA, SPELL_PYGMY_OIL_SMALLER_AURA });
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                Unit* caster = GetCaster();
                if (Aura* aura = caster->GetAura(SPELL_PYGMY_OIL_PYGMY_AURA))
                    aura->RefreshDuration();
                else
                {
                    aura = caster->GetAura(SPELL_PYGMY_OIL_SMALLER_AURA);
                    if (!aura || aura->GetStackAmount() < 5 || !roll_chance_i(50))
                         caster->CastSpell(caster, SPELL_PYGMY_OIL_SMALLER_AURA, true);
                    else
                    {
                        aura->Remove();
                        caster->CastSpell(caster, SPELL_PYGMY_OIL_PYGMY_AURA, true);
                    }
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_pygmy_oil_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_pygmy_oil_SpellScript();
        }
};

class spell_item_unusual_compass : public SpellScriptLoader
{
    public:
        spell_item_unusual_compass() : SpellScriptLoader("spell_item_unusual_compass") { }

        class spell_item_unusual_compass_SpellScript : public SpellScript
        {
            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                Unit* caster = GetCaster();
                caster->SetFacingTo(frand(0.0f, 2.0f * float(M_PI)));
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_unusual_compass_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_unusual_compass_SpellScript();
        }
};

enum ChickenCover
{
    SPELL_CHICKEN_NET               = 51959,
    SPELL_CAPTURE_CHICKEN_ESCAPE    = 51037,
    QUEST_CHICKEN_PARTY             = 12702,
    QUEST_FLOWN_THE_COOP            = 12532,
};

class spell_item_chicken_cover : public SpellScriptLoader
{
    public:
        spell_item_chicken_cover() : SpellScriptLoader("spell_item_chicken_cover") { }

        class spell_item_chicken_cover_SpellScript : public SpellScript
        {
            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            bool Validate(SpellInfo const* /*spell*/) override
            {
                return ValidateSpellInfo({ SPELL_CHICKEN_NET, SPELL_CAPTURE_CHICKEN_ESCAPE }) &&
                    sObjectMgr->GetQuestTemplate(QUEST_CHICKEN_PARTY) &&
                    sObjectMgr->GetQuestTemplate(QUEST_FLOWN_THE_COOP);
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                Player* caster = GetCaster()->ToPlayer();
                if (Unit* target = GetHitUnit())
                {
                    if (!target->HasAura(SPELL_CHICKEN_NET) && (caster->GetQuestStatus(QUEST_CHICKEN_PARTY) == QUEST_STATUS_INCOMPLETE || caster->GetQuestStatus(QUEST_FLOWN_THE_COOP) == QUEST_STATUS_INCOMPLETE))
                    {
                        caster->CastSpell(caster, SPELL_CAPTURE_CHICKEN_ESCAPE, true);
                        target->KillSelf();
                    }
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_chicken_cover_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_chicken_cover_SpellScript();
        }
};

class spell_item_muisek_vessel : public SpellScriptLoader
{
    public:
        spell_item_muisek_vessel() : SpellScriptLoader("spell_item_muisek_vessel") { }

        class spell_item_muisek_vessel_SpellScript : public SpellScript
        {
            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Creature* target = GetHitCreature())
                    if (target->isDead())
                        target->DespawnOrUnsummon();
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_muisek_vessel_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_muisek_vessel_SpellScript();
        }
};

enum GreatmothersSoulcather
{
    SPELL_FORCE_CAST_SUMMON_GNOME_SOUL = 46486,
};
class spell_item_greatmothers_soulcatcher : public SpellScriptLoader
{
public:
    spell_item_greatmothers_soulcatcher() : SpellScriptLoader("spell_item_greatmothers_soulcatcher") { }

    class spell_item_greatmothers_soulcatcher_SpellScript : public SpellScript
    {
             void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            if (GetHitUnit())
                GetCaster()->CastSpell(GetCaster(), SPELL_FORCE_CAST_SUMMON_GNOME_SOUL);
        }

        void Register() override
        {
            OnEffectHitTarget.Register(&spell_item_greatmothers_soulcatcher_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_item_greatmothers_soulcatcher_SpellScript();
    }
};

enum ShardOfTheScale
{
    SPELL_PURIFIED_CAUTERIZING_HEAL = 69733,
    SPELL_PURIFIED_SEARING_FLAMES   = 69729,

    SPELL_SHINY_CAUTERIZING_HEAL    = 69734,
    SPELL_SHINY_SEARING_FLAMES      = 69730
};

// Item - 49310: Purified Shard of the Scale
// 69755 - Purified Shard of the Scale - Equip Effect

// Item - 49488: Shiny Shard of the Scale
// 69739 - Shiny Shard of the Scale - Equip Effect
template <uint32 HealProcSpellId, uint32 DamageProcSpellId>
class spell_item_shard_of_the_scale : public SpellScriptLoader
{
    public:
        spell_item_shard_of_the_scale(char const* ScriptName) : SpellScriptLoader(ScriptName) { }

        template <uint32 HealProc, uint32 DamageProc>
        class spell_item_shard_of_the_scale_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    HealProc,
                    DamageProc
                });
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();
                Unit* caster = eventInfo.GetActor();
                Unit* target = eventInfo.GetProcTarget();

                if (eventInfo.GetTypeMask() & PROC_FLAG_DEAL_HELPFUL_SPELL)
                    caster->CastSpell(target, HealProc, aurEff);

                if (eventInfo.GetTypeMask() & PROC_FLAG_DEAL_HARMFUL_SPELL)
                    caster->CastSpell(target, DamageProc, aurEff);
            }

            void Register() override
            {
                OnEffectProc.Register(&spell_item_shard_of_the_scale_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_shard_of_the_scale_AuraScript<HealProcSpellId, DamageProcSpellId>();
        }
};

enum SoulPreserver
{
    SPELL_SOUL_PRESERVER_DRUID       = 60512,
    SPELL_SOUL_PRESERVER_PALADIN     = 60513,
    SPELL_SOUL_PRESERVER_PRIEST      = 60514,
    SPELL_SOUL_PRESERVER_SHAMAN      = 60515,
};

class spell_item_soul_preserver : public SpellScriptLoader
{
public:
    spell_item_soul_preserver() : SpellScriptLoader("spell_item_soul_preserver") { }

    class spell_item_soul_preserver_AuraScript : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo(
            {
                SPELL_SOUL_PRESERVER_DRUID,
                SPELL_SOUL_PRESERVER_PALADIN,
                SPELL_SOUL_PRESERVER_PRIEST,
                SPELL_SOUL_PRESERVER_SHAMAN
            });
        }

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* caster = eventInfo.GetActor();

            switch (caster->getClass())
            {
                case CLASS_DRUID:
                    caster->CastSpell(caster, SPELL_SOUL_PRESERVER_DRUID, aurEff);
                    break;
                case CLASS_PALADIN:
                    caster->CastSpell(caster, SPELL_SOUL_PRESERVER_PALADIN, aurEff);
                    break;
                case CLASS_PRIEST:
                    caster->CastSpell(caster, SPELL_SOUL_PRESERVER_PRIEST, aurEff);
                    break;
                case CLASS_SHAMAN:
                    caster->CastSpell(caster, SPELL_SOUL_PRESERVER_SHAMAN, aurEff);
                    break;
                default:
                    break;
            }
        }

        void Register() override
        {
            OnEffectProc.Register(&spell_item_soul_preserver_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_item_soul_preserver_AuraScript();
    }
};

enum ExaltedSunwellNeck
{
    SPELL_LIGHTS_WRATH      = 45479, // Light's Wrath if Exalted by Aldor
    SPELL_ARCANE_BOLT       = 45429, // Arcane Bolt if Exalted by Scryers

    SPELL_LIGHTS_STRENGTH   = 45480, // Light's Strength if Exalted by Aldor
    SPELL_ARCANE_STRIKE     = 45428, // Arcane Strike if Exalted by Scryers

    SPELL_LIGHTS_WARD       = 45432, // Light's Ward if Exalted by Aldor
    SPELL_ARCANE_INSIGHT    = 45431, // Arcane Insight if Exalted by Scryers

    SPELL_LIGHTS_SALVATION  = 45478, // Light's Salvation if Exalted by Aldor
    SPELL_ARCANE_SURGE      = 45430, // Arcane Surge if Exalted by Scryers

    FACTION_ALDOR           = 932,
    FACTION_SCRYERS         = 934
};

// Item - 34678: Shattered Sun Pendant of Acumen
// 45481 - Sunwell Exalted Caster Neck

// Item - 34679: Shattered Sun Pendant of Might
// 45482 - Sunwell Exalted Melee Neck

// Item - 34680: Shattered Sun Pendant of Resolve
// 45483 - Sunwell Exalted Tank Neck

// Item - 34677: Shattered Sun Pendant of Restoration
// 45484 Sunwell Exalted Healer Neck
template <uint32 AldorSpellId, uint32 ScryersSpellId>
class spell_item_sunwell_neck : public SpellScriptLoader
{
    public:
        spell_item_sunwell_neck(char const* ScriptName) : SpellScriptLoader(ScriptName) { }

        template <uint32 Aldors, uint32 Scryers>
        class spell_item_sunwell_neck_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ Aldors, Scryers }) &&
                    sFactionStore.LookupEntry(FACTION_ALDOR) &&
                    sFactionStore.LookupEntry(FACTION_SCRYERS);
            }

            bool CheckProc(ProcEventInfo& eventInfo)
            {
                if (eventInfo.GetActor()->GetTypeId() != TYPEID_PLAYER)
                    return false;
                return true;
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();
                Player* player = eventInfo.GetActor()->ToPlayer();
                Unit* target = eventInfo.GetProcTarget();

                // Aggression checks are in the spell system... just cast and forget
                if (player->GetReputationRank(FACTION_ALDOR) == REP_EXALTED)
                    player->CastSpell(target, Aldors, aurEff);

                if (player->GetReputationRank(FACTION_SCRYERS) == REP_EXALTED)
                    player->CastSpell(target, Scryers, aurEff);
            }

            void Register() override
            {
                DoCheckProc.Register(&spell_item_sunwell_neck_AuraScript::CheckProc);
                OnEffectProc.Register(&spell_item_sunwell_neck_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_sunwell_neck_AuraScript<AldorSpellId, ScryersSpellId>();
        }
};

class spell_item_toy_train_set_pulse : public SpellScriptLoader
{
public:
    spell_item_toy_train_set_pulse() : SpellScriptLoader("spell_item_toy_train_set_pulse") { }

    class spell_item_toy_train_set_pulse_SpellScript : public SpellScript
    {
             void HandleDummy(SpellEffIndex /*index*/)
        {
            if (Player* target = GetHitUnit()->ToPlayer())
            {
                target->HandleEmoteCommand(EMOTE_ONESHOT_TRAIN);
                if (EmotesTextSoundEntry const* soundEntry = sDBCManager.FindTextSoundEmoteFor(TEXT_EMOTE_TRAIN, target->getRace(), target->getGender()))
                    target->PlayDistanceSound(soundEntry->ID);
            }
        }

        void HandleTargets(std::list<WorldObject*>& targetList)
        {
            targetList.remove_if([](WorldObject const* obj) { return obj->GetTypeId() != TYPEID_PLAYER; });
        }

        void Register() override
        {
            OnEffectHitTarget.Register(&spell_item_toy_train_set_pulse_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            OnObjectAreaTargetSelect.Register(&spell_item_toy_train_set_pulse_SpellScript::HandleTargets, EFFECT_ALL, TARGET_UNIT_SRC_AREA_ALLY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_item_toy_train_set_pulse_SpellScript();
    }
};

enum TauntFlag
{
    SPELL_TAUNT_FLAG    = 51657,

    EMOTE_PLANTS_FLAG   = 28008
};

// 51640 - Taunt Flag Targeting
class spell_item_taunt_flag_targeting : public SpellScriptLoader
{
    public:
        spell_item_taunt_flag_targeting() : SpellScriptLoader("spell_item_taunt_flag_targeting") { }

        class spell_item_taunt_flag_targeting_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_TAUNT_FLAG }) &&
                    sObjectMgr->GetBroadcastText(EMOTE_PLANTS_FLAG);
            }

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                targets.remove_if([](WorldObject* obj) -> bool
                {
                    return obj->GetTypeId() != TYPEID_PLAYER && obj->GetTypeId() != TYPEID_CORPSE;
                });

                if (targets.empty())
                {
                    FinishCast(SPELL_FAILED_NO_VALID_TARGETS);
                    return;
                }

                Trinity::Containers::RandomResize(targets, 1);
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                // we *really* want the unit implementation here
                // it sends a packet like seen on sniff
                GetCaster()->Unit::TextEmote(EMOTE_PLANTS_FLAG, GetHitUnit(), false);

                GetCaster()->CastSpell(GetHitUnit(), SPELL_TAUNT_FLAG, true);
            }

            void Register() override
            {
                OnObjectAreaTargetSelect.Register(&spell_item_taunt_flag_targeting_SpellScript::FilterTargets, EFFECT_0, TARGET_CORPSE_SRC_AREA_ENEMY);
                OnEffectHitTarget.Register(&spell_item_taunt_flag_targeting_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_taunt_flag_targeting_SpellScript();
        }
};


// 13180 - Gnomish Mind Control Cap
enum MindControlCap
{
    ROLL_CHANCE_DULLARD             = 32,
    ROLL_CHANCE_NO_BACKFIRE         = 95,
    SPELL_GNOMISH_MIND_CONTROL_CAP  = 13181,
    SPELL_DULLARD                   = 67809
};

class spell_item_mind_control_cap : public SpellScriptLoader
{
    public:
        spell_item_mind_control_cap() : SpellScriptLoader("spell_item_mind_control_cap") { }

        class spell_item_mind_control_cap_SpellScript : public SpellScript
        {
            bool Load() override
            {
                if (!GetCastItem())
                    return false;
                return true;
            }

            bool Validate(SpellInfo const* /*spell*/) override
            {
                return ValidateSpellInfo({ SPELL_GNOMISH_MIND_CONTROL_CAP, SPELL_DULLARD });
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                Unit* caster = GetCaster();
                if (Unit* target = GetHitUnit())
                {
                    if (roll_chance_i(ROLL_CHANCE_NO_BACKFIRE))
                        caster->CastSpell(target, roll_chance_i(ROLL_CHANCE_DULLARD) ? SPELL_DULLARD : SPELL_GNOMISH_MIND_CONTROL_CAP, GetCastItem());
                    else
                        target->CastSpell(caster, SPELL_GNOMISH_MIND_CONTROL_CAP, true); // backfire - 5% chance
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_mind_control_cap_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_mind_control_cap_SpellScript();
        }
};

// 8344 - Universal Remote (Gnomish Universal Remote)
enum UniversalRemote
{
    SPELL_CONTROL_MACHINE       = 8345,
    SPELL_MOBILITY_MALFUNCTION  = 8346,
    SPELL_TARGET_LOCK           = 8347
};

class spell_item_universal_remote : public SpellScriptLoader
{
    public:
        spell_item_universal_remote() : SpellScriptLoader("spell_item_universal_remote") { }

        class spell_item_universal_remote_SpellScript : public SpellScript
        {
            bool Load() override
            {
                if (!GetCastItem())
                    return false;
                return true;
            }

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_CONTROL_MACHINE, SPELL_MOBILITY_MALFUNCTION, SPELL_TARGET_LOCK });
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit* target = GetHitUnit())
                {
                    uint8 chance = urand(0, 99);
                    if (chance < 15)
                        GetCaster()->CastSpell(target, SPELL_TARGET_LOCK, GetCastItem());
                    else if (chance < 25)
                        GetCaster()->CastSpell(target, SPELL_MOBILITY_MALFUNCTION, GetCastItem());
                    else
                        GetCaster()->CastSpell(target, SPELL_CONTROL_MACHINE, GetCastItem());
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_item_universal_remote_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_universal_remote_SpellScript();
        }
};

enum ZandalarianCharms
{
    SPELL_UNSTABLE_POWER_AURA_STACK     = 24659,
    SPELL_RESTLESS_STRENGTH_AURA_STACK  = 24662
};

// Item - 19950: Zandalarian Hero Charm
// 24658 - Unstable Power

// Item - 19949: Zandalarian Hero Medallion
// 24661 - Restless Strength
class spell_item_zandalarian_charm : public SpellScriptLoader
{
    public:
        spell_item_zandalarian_charm(char const* ScriptName, uint32 SpellId) : SpellScriptLoader(ScriptName), _spellId(SpellId) { }

        class spell_item_zandalarian_charm_AuraScript : public AuraScript
        {

            friend class spell_item_zandalarian_charm;
            spell_item_zandalarian_charm_AuraScript(uint32 SpellId) : AuraScript(), _spellId(SpellId) { }
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ _spellId });
            }

            bool CheckProc(ProcEventInfo& eventInfo)
            {
                if (SpellInfo const* spellInfo = eventInfo.GetSpellInfo())
                    if (spellInfo->Id != m_scriptSpellId)
                        return true;

                return false;
            }

            void HandleStackDrop(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
            {
                PreventDefaultAction();
                GetTarget()->RemoveAuraFromStack(_spellId);
            }

            void Register() override
            {
                DoCheckProc.Register(&spell_item_zandalarian_charm_AuraScript::CheckProc);
                OnEffectProc.Register(&spell_item_zandalarian_charm_AuraScript::HandleStackDrop, EFFECT_0, SPELL_AURA_DUMMY);
            }

            uint32 _spellId;
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_zandalarian_charm_AuraScript(_spellId);
        }

    private:
        uint32 _spellId;
};

// 45051 - Mad Alchemist's Potion (34440)
class spell_item_mad_alchemists_potion : public SpellScriptLoader
{
public:
    spell_item_mad_alchemists_potion() : SpellScriptLoader("spell_item_mad_alchemists_potion") {}

    class mad_alchemists_potion_SpellScript : public SpellScript
    {
             void SecondaryEffect()
        {
            std::vector<uint32> availableElixirs =
            {
                // Battle Elixirs
                33720, // Onslaught Elixir (28102)
                54452, // Adept's Elixir (28103)
                33726, // Elixir of Mastery (28104)
                28490, // Elixir of Major Strength (22824)
                28491, // Elixir of Healing Power (22825)
                28493, // Elixir of Major Frost Power (22827)
                54494, // Elixir of Major Agility (22831)
                28501, // Elixir of Major Firepower (22833)
                28503,// Elixir of Major Shadow Power (22835)
                38954, // Fel Strength Elixir (31679)
                // Guardian Elixirs
                39625, // Elixir of Major Fortitude (32062)
                39626, // Earthen Elixir (32063)
                39627, // Elixir of Draenic Wisdom (32067)
                39628, // Elixir of Ironskin (32068)
                28502, // Elixir of Major Defense (22834)
                28514, // Elixir of Empowerment (22848)
                // Other
                28489, // Elixir of Camouflage (22823)
                28496  // Elixir of the Searching Eye (22830)
            };

            Unit* target = GetCaster();

            if (target->GetPowerType() == POWER_MANA)
                availableElixirs.push_back(28509); // Elixir of Major Mageblood (22840)

            uint32 chosenElixir = Trinity::Containers::SelectRandomContainerElement(availableElixirs);

            bool useElixir = true;

            SpellGroup chosenSpellGroup = SPELL_GROUP_NONE;
            if (sSpellMgr->IsSpellMemberOfSpellGroup(chosenElixir, SPELL_GROUP_ELIXIR_BATTLE))
                chosenSpellGroup = SPELL_GROUP_ELIXIR_BATTLE;
            if (sSpellMgr->IsSpellMemberOfSpellGroup(chosenElixir, SPELL_GROUP_ELIXIR_GUARDIAN))
                chosenSpellGroup = SPELL_GROUP_ELIXIR_GUARDIAN;
            // If another spell of the same group is already active the elixir should not be cast
            if (chosenSpellGroup != SPELL_GROUP_NONE)
            {
                Unit::AuraApplicationMap const& auraMap = target->GetAppliedAuras();
                for (auto itr = auraMap.begin(); itr != auraMap.end(); ++itr)
                {
                    uint32 spellId = itr->second->GetBase()->GetId();
                    if (sSpellMgr->IsSpellMemberOfSpellGroup(spellId, chosenSpellGroup) && spellId != chosenElixir)
                    {
                        useElixir = false;
                        break;
                    }
                }
            }

            if (useElixir)
                target->CastSpell(target, chosenElixir, GetCastItem());
        }

        void Register() override
        {
            AfterCast.Register(&mad_alchemists_potion_SpellScript::SecondaryEffect);
        }

    };

    SpellScript* GetSpellScript() const override
    {
        return new mad_alchemists_potion_SpellScript();
    }
};

// 53750 - Crazy Alchemist's Potion (40077)
class spell_item_crazy_alchemists_potion : public SpellScriptLoader
{
public:
    spell_item_crazy_alchemists_potion() : SpellScriptLoader("spell_item_crazy_alchemists_potion") {}

    class crazy_alchemists_potion_SpellScript : public SpellScript
    {
             void SecondaryEffect()
        {
            std::vector<uint32> availableElixirs =
            {
                43185, // Runic Healing Potion (33447)
                53750, // Crazy Alchemist's Potion (40077)
                53761, // Powerful Rejuvenation Potion (40087)
                53762, // Indestructible Potion (40093)
                53908, // Potion of Speed (40211)
                53909, // Potion of Wild Magic (40212)
                53910, // Mighty Arcane Protection Potion (40213)
                53911, // Mighty Fire Protection Potion (40214)
                53913, // Mighty Frost Protection Potion (40215)
                53914, // Mighty Nature Protection Potion (40216)
                53915  // Mighty Shadow Protection Potion (40217)
            };

            Unit* target = GetCaster();

            if (!target->IsInCombat())
                availableElixirs.push_back(53753); // Potion of Nightmares (40081)
            if (target->GetPowerType() == POWER_MANA)
                availableElixirs.push_back(43186); // Runic Mana Potion(33448)

            uint32 chosenElixir = Trinity::Containers::SelectRandomContainerElement(availableElixirs);

            target->CastSpell(target, chosenElixir, GetCastItem());
        }

        void Register() override
        {
            AfterCast.Register(&crazy_alchemists_potion_SpellScript::SecondaryEffect);
        }

    };

    SpellScript* GetSpellScript() const override
    {
        return new crazy_alchemists_potion_SpellScript();
    }
};

enum DeathChoiceSpells
{
    SPELL_DEATH_CHOICE_NORMAL_AURA      = 67702,
    SPELL_DEATH_CHOICE_NORMAL_AGILITY   = 67703,
    SPELL_DEATH_CHOICE_NORMAL_STRENGTH  = 67708,
    SPELL_DEATH_CHOICE_HEROIC_AURA      = 67771,
    SPELL_DEATH_CHOICE_HEROIC_AGILITY   = 67772,
    SPELL_DEATH_CHOICE_HEROIC_STRENGTH  = 67773
};

class spell_item_death_choice : public SpellScriptLoader
{
public:
    spell_item_death_choice() : SpellScriptLoader("spell_item_death_choice") { }

    class spell_item_death_choice_AuraScript : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo(
            {
                SPELL_DEATH_CHOICE_NORMAL_STRENGTH,
                SPELL_DEATH_CHOICE_NORMAL_AGILITY,
                SPELL_DEATH_CHOICE_HEROIC_STRENGTH,
                SPELL_DEATH_CHOICE_HEROIC_AGILITY
            });
        }

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* caster = eventInfo.GetActor();
            float str = caster->GetStat(STAT_STRENGTH);
            float agi = caster->GetStat(STAT_AGILITY);

            switch (aurEff->GetId())
            {
                case SPELL_DEATH_CHOICE_NORMAL_AURA:
                {
                    if (str > agi)
                        caster->CastSpell(caster, SPELL_DEATH_CHOICE_NORMAL_STRENGTH, aurEff);
                    else
                        caster->CastSpell(caster, SPELL_DEATH_CHOICE_NORMAL_AGILITY, aurEff);
                    break;
                }
                case SPELL_DEATH_CHOICE_HEROIC_AURA:
                {
                    if (str > agi)
                        caster->CastSpell(caster, SPELL_DEATH_CHOICE_HEROIC_STRENGTH, aurEff);
                    else
                        caster->CastSpell(caster, SPELL_DEATH_CHOICE_HEROIC_AGILITY, aurEff);
                    break;
                }
                default:
                    break;
            }
        }

        void Register() override
        {
            OnEffectProc.Register(&spell_item_death_choice_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_item_death_choice_AuraScript();
    }
};

enum TrinketStackSpells
{
    SPELL_LIGHTNING_CAPACITOR_AURA              = 37657,  // Lightning Capacitor
    SPELL_LIGHTNING_CAPACITOR_STACK             = 37658,
    SPELL_LIGHTNING_CAPACITOR_TRIGGER           = 37661,
    SPELL_THUNDER_CAPACITOR_AURA                = 54841,  // Thunder Capacitor
    SPELL_THUNDER_CAPACITOR_STACK               = 54842,
    SPELL_THUNDER_CAPACITOR_TRIGGER             = 54843,
    SPELL_TOC25_CASTER_TRINKET_NORMAL_AURA      = 67712,  // Item - Coliseum 25 Normal Caster Trinket
    SPELL_TOC25_CASTER_TRINKET_NORMAL_STACK     = 67713,
    SPELL_TOC25_CASTER_TRINKET_NORMAL_TRIGGER   = 67714,
    SPELL_TOC25_CASTER_TRINKET_HEROIC_AURA      = 67758,  // Item - Coliseum 25 Heroic Caster Trinket
    SPELL_TOC25_CASTER_TRINKET_HEROIC_STACK     = 67759,
    SPELL_TOC25_CASTER_TRINKET_HEROIC_TRIGGER   = 67760,
};

class spell_item_trinket_stack : public SpellScriptLoader
{
public:
    spell_item_trinket_stack(char const* scriptName, uint32 stackSpell, uint32 triggerSpell) : SpellScriptLoader(scriptName),
        _stackSpell(stackSpell), _triggerSpell(triggerSpell)
    {
    }

    class spell_item_trinket_stack_AuraScript : public AuraScript
    {
    public:
        spell_item_trinket_stack_AuraScript(uint32 stackSpell, uint32 triggerSpell) : _stackSpell(stackSpell), _triggerSpell(triggerSpell)
        {
        }

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ _stackSpell, _triggerSpell });
        }

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* caster = eventInfo.GetActor();

            caster->CastSpell(caster, _stackSpell, aurEff); // cast the stack

            Aura* dummy = caster->GetAura(_stackSpell); // retrieve aura

            //dont do anything if it's not the right amount of stacks;
            if (!dummy || dummy->GetStackAmount() < aurEff->GetAmount())
                return;

            // if right amount, remove the aura and cast real trigger
            caster->RemoveAurasDueToSpell(_stackSpell);
            if (Unit* target = eventInfo.GetActionTarget())
                caster->CastSpell(target, _triggerSpell, aurEff);
        }

        void Register() override
        {
            OnEffectProc.Register(&spell_item_trinket_stack_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        }

    private:
        uint32 _stackSpell;
        uint32 _triggerSpell;
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_item_trinket_stack_AuraScript(_stackSpell, _triggerSpell);
    }

private:
    uint32 _stackSpell;
    uint32 _triggerSpell;
};

// 57345 - Darkmoon Card: Greatness
enum DarkmoonCardSpells
{
    SPELL_DARKMOON_CARD_STRENGTH        = 60229,
    SPELL_DARKMOON_CARD_AGILITY         = 60233,
    SPELL_DARKMOON_CARD_INTELLECT       = 60234,
    SPELL_DARKMOON_CARD_SPIRIT          = 60235,
};

class spell_item_darkmoon_card_greatness : public SpellScriptLoader
{
public:
    spell_item_darkmoon_card_greatness() : SpellScriptLoader("spell_item_darkmoon_card_greatness") { }

    class spell_item_darkmoon_card_greatness_AuraScript : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo(
            {
                SPELL_DARKMOON_CARD_STRENGTH,
                SPELL_DARKMOON_CARD_AGILITY,
                SPELL_DARKMOON_CARD_INTELLECT,
                SPELL_DARKMOON_CARD_SPIRIT
            });
        }

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* caster = eventInfo.GetActor();
            float str = caster->GetStat(STAT_STRENGTH);
            float agi = caster->GetStat(STAT_AGILITY);
            float intl = caster->GetStat(STAT_INTELLECT);
            float spi = caster->GetStat(STAT_SPIRIT);
            float stat = 0.0f;

            uint32 spellTrigger = SPELL_DARKMOON_CARD_STRENGTH;

           if (str > stat)
           {
               spellTrigger = SPELL_DARKMOON_CARD_STRENGTH;
               stat = str;
           }

           if (agi > stat)
           {
               spellTrigger = SPELL_DARKMOON_CARD_AGILITY;
               stat = agi;
           }

           if (intl > stat)
           {
               spellTrigger = SPELL_DARKMOON_CARD_INTELLECT;
               stat = intl;
           }

           if (spi > stat)
           {
               spellTrigger = SPELL_DARKMOON_CARD_SPIRIT;
               stat = spi;
           }

           caster->CastSpell(caster, spellTrigger, aurEff);
        }

        void Register() override
        {
            OnEffectProc.Register(&spell_item_darkmoon_card_greatness_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_item_darkmoon_card_greatness_AuraScript();
    }
};

// 43820 - Amani Charm of the Witch Doctor
enum CharmWitchDoctor
{
    SPELL_CHARM_WITCH_DOCTOR_PROC = 43821
};

class spell_item_charm_witch_doctor : public SpellScriptLoader
{
public:
    spell_item_charm_witch_doctor() : SpellScriptLoader("spell_item_charm_witch_doctor") { }

    class spell_item_charm_witch_doctor_AuraScript : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ SPELL_CHARM_WITCH_DOCTOR_PROC });
        }

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* caster = eventInfo.GetActor();
            Unit* target = eventInfo.GetActionTarget();

            if (target)
            {
                int32 bp = CalculatePct(target->GetCreateHealth(),aurEff->GetSpellInfo()->Effects[1].CalcValue());
                caster->CastSpell(target, SPELL_CHARM_WITCH_DOCTOR_PROC, CastSpellExtraArgs(aurEff).AddSpellBP0(bp));
            }
        }

        void Register() override
        {
            OnEffectProc.Register(&spell_item_charm_witch_doctor_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_item_charm_witch_doctor_AuraScript();
    }
};

// 27522,40336 - Mana Drain
enum ManaDrainSpells
{
    SPELL_MANA_DRAIN_ENERGIZE = 29471,
    SPELL_MANA_DRAIN_LEECH    = 27526
};

class spell_item_mana_drain : public SpellScriptLoader
{
public:
    spell_item_mana_drain() : SpellScriptLoader("spell_item_mana_drain") { }

    class spell_item_mana_drain_AuraScript : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo(
            {
                SPELL_MANA_DRAIN_ENERGIZE,
                SPELL_MANA_DRAIN_LEECH
            });
        }

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* caster = eventInfo.GetActor();
            Unit* target = eventInfo.GetActionTarget();

            if (caster->IsAlive())
                caster->CastSpell(caster, SPELL_MANA_DRAIN_ENERGIZE, aurEff);

            if (target && target->IsAlive())
                caster->CastSpell(target, SPELL_MANA_DRAIN_LEECH, aurEff);
        }

        void Register() override
        {
            OnEffectProc.Register(&spell_item_mana_drain_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_item_mana_drain_AuraScript();
    }
};

enum EggShell
{
    SPELL_EGG_SHELL_NORMAL          = 91296,
    SPELL_EGG_SHELL_PERIODIC_NORMAL = 91306,
    SPELL_EGG_SHELL_CANCEL_NORMAL   = 91305,
    SPELL_EGG_SHELL_PERIODIC_HEROIC = 91311,
    SPELL_EGG_SHELL_CANCEL_HEROIC   = 91310
};

// 91296, 91308 - Egg Shell
class spell_item_egg_shell : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_EGG_SHELL_NORMAL,
                SPELL_EGG_SHELL_PERIODIC_NORMAL,
                SPELL_EGG_SHELL_CANCEL_NORMAL,
                SPELL_EGG_SHELL_PERIODIC_HEROIC,
                SPELL_EGG_SHELL_CANCEL_HEROIC
            });
    }

    void HandleEffectPeriodic(AuraEffect const* aurEff)
    {
        GetTarget()->CastSpell(GetTarget(), GetSpellInfo()->Id == SPELL_EGG_SHELL_NORMAL ? SPELL_EGG_SHELL_PERIODIC_NORMAL : SPELL_EGG_SHELL_PERIODIC_HEROIC, aurEff);
    }

    void HandleEffectRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        if (GetTargetApplication()->GetRemoveMode().HasFlag(AuraRemoveFlags::ByCancel))
            GetTarget()->CastSpell(GetTarget(), GetSpellInfo()->Id == SPELL_EGG_SHELL_NORMAL ? SPELL_EGG_SHELL_PERIODIC_NORMAL : SPELL_EGG_SHELL_PERIODIC_HEROIC, aurEff);
    }

    void Register() override
    {
        OnEffectPeriodic.Register(&spell_item_egg_shell::HandleEffectPeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
        AfterEffectRemove.Register(&spell_item_egg_shell::HandleEffectRemove, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_item_song_of_sorrow : public AuraScript
{
    bool CheckProc(ProcEventInfo& eventInfo)
    {
        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!damageInfo || !damageInfo->GetDamage())
            return false;

        int32 pct = GetSpellInfo()->Effects[EFFECT_0].BasePoints;

        if (!damageInfo->GetVictim()->HealthBelowPctDamaged(pct, damageInfo->GetDamage()))
            return false;

        uint32 spellId = GetSpellInfo()->Effects[EFFECT_0].TriggerSpell;
        return !GetTarget()->GetSpellHistory()->HasCooldown(spellId) && !GetTarget()->HasAura(spellId);
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_item_song_of_sorrow::CheckProc);
    }
};

class spell_item_crescendo_of_suffering : public AuraScript
{
    void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->GetSpellHistory()->AddCooldown(GetSpellInfo()->Id, 0, 10s);
    }

    void Register() override
    {
        AfterEffectRemove.Register(&spell_item_crescendo_of_suffering::HandleEffectRemove, EFFECT_0, SPELL_AURA_MOD_DAMAGE_DONE, AURA_EFFECT_HANDLE_REAL);
    }
};

enum ConsumeCharges
{
    SPELL_RAW_FURY                  = 91832
};

class spell_item_consume_charges : public SpellScriptLoader
{
    public:
        spell_item_consume_charges(char const* ScriptName, uint32 SpellId) : SpellScriptLoader(ScriptName), _spellId(SpellId) { }

        class spell_item_consume_charges_SpellScript : public SpellScript
        {
            friend class spell_item_consume_charges;
            spell_item_consume_charges_SpellScript(uint32 SpellId) : SpellScript(), _spellId(SpellId) { }
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ _spellId });
            }

            SpellCastResult CheckCast()
            {
                if (Aura* aura = GetCaster()->GetAura(_spellId, GetCaster()->GetGUID()))
                    if (aura->GetStackAmount() == aura->GetSpellInfo()->StackAmount)
                        return SPELL_CAST_OK;

                return SPELL_FAILED_CASTER_AURASTATE;
            }

            void HandleStackConsumption()
            {
                if (Unit* caster = GetCaster())
                    caster->RemoveAurasDueToSpell(_spellId, caster->GetGUID());
            }

            void Register() override
            {
                OnCheckCast.Register(&spell_item_consume_charges_SpellScript::CheckCast);
                AfterCast.Register(&spell_item_consume_charges_SpellScript::HandleStackConsumption);
            }

            uint32 _spellId;
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_consume_charges_SpellScript(_spellId);
        }

    private:
        uint32 _spellId;
};

enum HeartsJudgement
{
    SPELL_HEARTS_JUDGEMENT_HEROIC   = 92328,
    SPELL_HEARTS_REVELATION         = 91027,
    SPELL_HEARTS_REVELATION_HEROIC  = 92325
};

class spell_item_hearts_judgement : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_HEARTS_JUDGEMENT_HEROIC,
                SPELL_HEARTS_REVELATION,
                SPELL_HEARTS_REVELATION_HEROIC
            });
    }

    bool Load() override
    {
        _auraId = GetSpellInfo()->Id == SPELL_HEARTS_JUDGEMENT_HEROIC ? SPELL_HEARTS_REVELATION_HEROIC : SPELL_HEARTS_REVELATION;
        return true;
    }

    SpellCastResult CheckCast()
    {
        if (Aura* aura = GetCaster()->GetAura(_auraId, GetCaster()->GetGUID()))
            if (aura->GetStackAmount())
                return SPELL_CAST_OK;

        return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
    }

    void Register() override
    {
        OnCheckCast.Register(&spell_item_hearts_judgement::CheckCast);
    }
private:
    uint32 _auraId = 0;
};

class spell_item_hearts_judgement_AuraScript : public AuraScript
{
    bool Load() override
    {
        _auraId = GetSpellInfo()->Id == SPELL_HEARTS_JUDGEMENT_HEROIC ? SPELL_HEARTS_REVELATION_HEROIC : SPELL_HEARTS_REVELATION;
        return true;
    }

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        Unit* target = GetUnitOwner();
        uint8 stacks = 0;
        if (Aura* revelation = target->GetAura(_auraId, target->GetGUID()))
        {
            stacks = revelation->GetStackAmount();
            revelation->Remove();
        }

        amount *= stacks;
    }

    void Register() override
    {
        DoEffectCalcAmount.Register(&spell_item_hearts_judgement_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_MOD_RATING);
    }
private:
    uint32 _auraId;
};

enum ApparatusOfKhazgoroth
{
    SPELL_TITANIC_POWER                     = 96923,
    SPELL_BLESSING_OF_THE_SHAPER_HASTE      = 96927,
    SPELL_BLESSING_OF_THE_SHAPER_CRIT       = 96928,
    SPELL_BLESSING_OF_THE_SHAPER_MASTERY    = 96929
};

// 96934 - Blessing of Khaz'goroth
// 97127 - Blessing of Khaz'goroth
class spell_item_blessing_of_khazgoroth : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_TITANIC_POWER,
                SPELL_BLESSING_OF_THE_SHAPER_HASTE,
                SPELL_BLESSING_OF_THE_SHAPER_CRIT,
                SPELL_BLESSING_OF_THE_SHAPER_MASTERY
            });
    }

    void HandleDummyEffect(SpellEffIndex /*effIndex*/)
    {
        Player* player = GetHitPlayer();
        if (!player)
            return;

        uint8 stacks = 0;
        Aura* aura = player->GetAura(SPELL_TITANIC_POWER, player->GetGUID());
        if (!aura)
            return;

        stacks = aura->GetStackAmount();

        int32 basePoints = GetEffectValue() * stacks;
        if (!basePoints)
            return;

        uint32 spellId = 0;
        float haste = player->GetRatingBonusValue(CR_HASTE_MELEE);
        float crit  = player->GetRatingBonusValue(CR_CRIT_MELEE);
        float mastery = player->GetRatingBonusValue(CR_MASTERY);

        if (haste >= crit && haste >= mastery)
            spellId = SPELL_BLESSING_OF_THE_SHAPER_HASTE;
        else if (crit >= haste && crit >= mastery)
            spellId = SPELL_BLESSING_OF_THE_SHAPER_CRIT;
        else if (mastery >= haste && mastery >= crit)
            spellId = SPELL_BLESSING_OF_THE_SHAPER_MASTERY;

        if (spellId && basePoints)
        {
            player->CastSpell(player, spellId, CastSpellExtraArgs(true).AddSpellBP0(basePoints));
            aura->Remove();
        }
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_item_blessing_of_khazgoroth::HandleDummyEffect, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

enum ScalesOfLife
{
    SPELL_WEIGHT_OF_A_FEATHER   = 96881
};

// 96879 - Weight of a Feather
// 97117 - Weight of a Feather
class spell_item_weight_of_a_feather : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WEIGHT_OF_A_FEATHER });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        HealInfo* heal = eventInfo.GetHealInfo();
        return heal && heal->GetHeal() > heal->GetEffectiveHeal();
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* target = GetTarget();
        int32 heal = eventInfo.GetHealInfo()->GetHeal() - eventInfo.GetHealInfo()->GetEffectiveHeal();

        // Store overheal
        int32 basePoints = heal;
        if (AuraEffect const* effect = target->GetAuraEffect(SPELL_WEIGHT_OF_A_FEATHER, EFFECT_0, target->GetGUID()))
            basePoints += effect->GetAmount();

        basePoints = std::min<int32>(basePoints, aurEff->GetAmount());
        target->CastSpell(target, SPELL_WEIGHT_OF_A_FEATHER, CastSpellExtraArgs(aurEff).AddSpellBP0(basePoints));
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_item_weight_of_a_feather::CheckProc);
        OnEffectProc.Register(&spell_item_weight_of_a_feather::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 96880 - Tipping of the Scales
class spell_item_tipping_of_the_scales : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WEIGHT_OF_A_FEATHER });
    }

    SpellCastResult CheckCast()
    {
        if (GetCaster()->GetAuraEffect(SPELL_WEIGHT_OF_A_FEATHER, EFFECT_0, GetCaster()->GetGUID()))
            return SPELL_CAST_OK;

        return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
    }

    void HandleHealValue(SpellEffIndex /*effIndex*/)
    {
        if (AuraEffect const* effect = GetCaster()->GetAuraEffect(SPELL_WEIGHT_OF_A_FEATHER, EFFECT_0, GetCaster()->GetGUID()))
        {
            SetEffectValue(effect->GetAmount());
            effect->GetBase()->Remove();
        }
    }

    void Register() override
    {
        OnCheckCast.Register(&spell_item_tipping_of_the_scales::CheckCast);
        OnEffectLaunchTarget.Register(&spell_item_tipping_of_the_scales::HandleHealValue, EFFECT_0, SPELL_EFFECT_HEAL);
    }
};

enum JarOfAncientRemedies
{
    SPELL_INNER_EYE_N                   = 91320,
    SPELL_INNER_EYE_HC                  = 92329,
    SPELL_ITEM_PROC_STACKING_SPIRIT_N   = 91321,
    SPELL_ITEM_PROC_STACKING_SPIRIT_HC  = 92330

};

// 91322 Blind Spot
// 92331 Blind Spot
class spell_item_blind_spot : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_INNER_EYE_N,
                SPELL_INNER_EYE_HC,
                SPELL_ITEM_PROC_STACKING_SPIRIT_N,
                SPELL_ITEM_PROC_STACKING_SPIRIT_HC
            });
    }

    void HandleHitEffect(SpellEffIndex /*effIndex*/)
    {
        Unit* target = GetHitUnit();
        target->RemoveAurasDueToSpell(SPELL_INNER_EYE_N, target->GetGUID());
        target->RemoveAurasDueToSpell(SPELL_INNER_EYE_HC, target->GetGUID());

        uint32 spellId = target->HasAura(SPELL_ITEM_PROC_STACKING_SPIRIT_N, target->GetGUID()) ? SPELL_ITEM_PROC_STACKING_SPIRIT_N : SPELL_ITEM_PROC_STACKING_SPIRIT_HC;
        if (AuraEffect const* effect = target->GetAuraEffect(spellId, EFFECT_0, target->GetGUID()))
        {
            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
            if (SpellProcEntry const* procEntry = sSpellMgr->GetSpellProcEntry(spellId))
                effect->GetBase()->AddProcCooldown(procEntry, now + 30s);
        }
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_item_blind_spot::HandleHitEffect, EFFECT_0, SPELL_EFFECT_ENERGIZE);
    }
};

enum VariablePulseLightningCapacitor
{
    SPELL_LIGHTNING_BOLT                            = 96891,
    SPELL_ELECTRICAL_CHARGE                         = 96890,
    SPELL_VARIABLE_PULSE_LIGHTNING_CAPACITOR_NORMAL = 96887
};

// 96887 - Variable Pulse Lightning Capacitor
// 97119 - Variable Pulse Lightning Capacitor
class spell_item_variable_pulse_lightning_capacitor : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_LIGHTNING_BOLT,
                SPELL_ELECTRICAL_CHARGE,
                SPELL_VARIABLE_PULSE_LIGHTNING_CAPACITOR_NORMAL
            });
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Player* target = GetTarget()->ToPlayer();
        if (!target)
            return;
        ObjectGuid itemGUID = aurEff->GetBase()->GetCastItemGUID();
        if (!itemGUID)
            return;

        AuraEffect const* chargeEffect = target->GetAuraEffect(SPELL_ELECTRICAL_CHARGE, EFFECT_0);
        if (Item* castItem = target->GetItemByGuid(itemGUID))
        {
            if (!chargeEffect || roll_chance_i(60))
                target->CastSpell(target, SPELL_ELECTRICAL_CHARGE, CastSpellExtraArgs(castItem).SetTriggeringAura(aurEff));
            else
            {
                int32 bp = 0;
                if (GetSpellInfo()->Id == SPELL_VARIABLE_PULSE_LIGHTNING_CAPACITOR_NORMAL)
                    bp = sSpellMgr->AssertSpellInfo(SPELL_LIGHTNING_BOLT)->Effects[EFFECT_0].CalcValue();
                else
                    bp = GetSpellInfo()->Effects[EFFECT_0].CalcValue();

                bp *= chargeEffect->GetBase()->GetStackAmount();
                chargeEffect->GetBase()->Remove();
                target->CastSpell(eventInfo.GetProcTarget(), SPELL_LIGHTNING_BOLT, CastSpellExtraArgs(castItem).SetTriggeringAura(aurEff).AddSpellBP0(bp));
            }
        }
    }

    void Register() override
    {
        OnEffectProc.Register(&spell_item_variable_pulse_lightning_capacitor::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

enum AuraProcRemoveSpells
{
    SPELL_TALISMAN_OF_ASCENDANCE    = 28200,
    SPELL_JOM_GABBAR                = 29602,
    SPELL_BATTLE_TRANCE             = 45040,
    SPELL_WORLD_QUELLER_FOCUS       = 90900
};

// 28200 - Ascendance
class spell_item_talisman_of_ascendance : public AuraScript
{
    bool Validate(SpellInfo const* /*spell*/) override
    {
        return ValidateSpellInfo({ SPELL_TALISMAN_OF_ASCENDANCE });
    }

    void OnRemove(AuraEffect const* /*effect*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->RemoveAurasDueToSpell(GetSpellInfo()->Effects[EFFECT_0].TriggerSpell);
    }

    void Register() override
    {
        OnEffectRemove.Register(&spell_item_talisman_of_ascendance::OnRemove, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
    }
};

// 29602 - Jom Gabbar
class spell_item_jom_gabbar : public AuraScript
{
    bool Validate(SpellInfo const* /*spell*/) override
    {
        return ValidateSpellInfo({ SPELL_JOM_GABBAR });
    }

    void OnRemove(AuraEffect const* /*effect*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->RemoveAurasDueToSpell(GetSpellInfo()->Effects[EFFECT_0].TriggerSpell);
    }

    void Register() override
    {
        OnEffectRemove.Register(&spell_item_jom_gabbar::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
    }
};

// 45040 - Battle Trance
class spell_item_battle_trance : public AuraScript
{
    bool Validate(SpellInfo const* /*spell*/) override
    {
        return ValidateSpellInfo({ SPELL_BATTLE_TRANCE });
    }

    void OnRemove(AuraEffect const* /*effect*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->RemoveAurasDueToSpell(GetSpellInfo()->Effects[EFFECT_0].TriggerSpell);
    }

    void Register() override
    {
        OnEffectRemove.Register(&spell_item_battle_trance::OnRemove, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
    }
};

// 90900 - World-Queller Focus
class spell_item_world_queller_focus : public AuraScript
{
    bool Validate(SpellInfo const* /*spell*/) override
    {
        return ValidateSpellInfo({ SPELL_WORLD_QUELLER_FOCUS });
    }

    void OnRemove(AuraEffect const* /*effect*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->RemoveAurasDueToSpell(GetSpellInfo()->Effects[EFFECT_0].TriggerSpell);
    }

    void Register() override
    {
        OnEffectRemove.Register(&spell_item_world_queller_focus::OnRemove, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
    }
};

enum ChocolateCookie
{
    SPELL_BY_THE_TIME_YOURE_DONE_EATING_YOULL_FEEL_RIGHT_AS_RAIN = 99041 // Serverside spell
};

// 87649 - Satisfied
class spell_item_satisfied : public SpellScript
{
    bool Load() override
    {
        return GetCaster()->IsPlayer();
    }

    void HandleAchievementCriteria()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        if (Aura const* aura = GetHitAura())
        {
            if (aura->GetStackAmount() == 91)
            {
                Player* player = caster->ToPlayer();
                player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET, SPELL_BY_THE_TIME_YOURE_DONE_EATING_YOULL_FEEL_RIGHT_AS_RAIN, 1);
            }
        }
    }

    void Register() override
    {
        AfterHit.Register(&spell_item_satisfied::HandleAchievementCriteria);
    }
};

enum Herbouflage
{
    SPELL_NOURISHMENT_HEALTH    = 88753,
    SPELL_NOURISHMENT_MANA      = 88754
};

// 88715 - Herbouflage
class spell_item_herbouflage : public AuraScript
{
    bool Validate(SpellInfo const* /*spell*/) override
    {
        return ValidateSpellInfo({ SPELL_NOURISHMENT_HEALTH, SPELL_NOURISHMENT_MANA });
    }

    void AfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        for (uint32 spellId : { SPELL_NOURISHMENT_HEALTH, SPELL_NOURISHMENT_MANA })
            GetTarget()->RemoveAurasDueToSpell(spellId, GetTarget()->GetGUID());
    }

    void Register() override
    {
        AfterEffectRemove.Register(&spell_item_herbouflage::AfterRemove, EFFECT_0, SPELL_AURA_TRANSFORM, AURA_EFFECT_HANDLE_REAL);
    }
};

// 96966, 97136 - Blaze of Life
class spell_item_blaze_of_life : public SpellScript
{
    void FilterTargets(std::list<WorldObject*>& targets)
    {
        if (targets.size() < 2)
            return;

        targets.sort(Trinity::HealthPctOrderPred());
        targets.resize(1);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect.Register(&spell_item_blaze_of_life::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ALLY);
    }
};

static std::array<uint32 /*spellId*/, MAX_SPELL_SCHOOL - 1> const ResistanceSpellsBySchool =
{
    // SPELL_SCHOOL_NORMAL has no shield
    27536, // SPELL_SCHOOL_HOLY
    27533, // SPELL_SCHOOL_FIRE
    27538, // SPELL_SCHOOL_NATURE
    27534, // SPELL_SCHOOL_FROST
    27535, // SPELL_SCHOOL_SHADOW
    27540  // SPELL_SCHOOL_ARCANE
};

// 27539 - Obsidian Armor
class spell_item_obsidian_armor : public AuraScript
{
    bool Validate(SpellInfo const* /*spell*/) override
    {
        return ValidateSpellInfo(ResistanceSpellsBySchool);
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        // There are spells which have multiple schools (Frostfire Bolt for example). So we proc multiple shields
        for (uint8 i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
        {
            if ((eventInfo.GetSpellInfo()->GetSchoolMask() & (1 << i)) != 0)
                GetTarget()->CastSpell(GetTarget(), ResistanceSpellsBySchool[i - 1], aurEff);
        }
    }

    void Register() override
    {
        OnEffectProc.Register(&spell_item_obsidian_armor::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

static constexpr std::array<uint32, 3> const NoKaledProcSpellIdsLFR = { 109867, 109869, 109871 };
static constexpr std::array<uint32, 3> const NoKaledProcSpellIdsNormal = { 107787, 107789, 107785 };
static constexpr std::array<uint32, 3> const NoKaledProcSpellIdsHeroic = { 109868, 109870, 109872 };

// 109866, 107786, 109873 - Item - Dragon Soul - Proc - Agi Melee 1H Axe LFR
class spell_item_dragon_soul_proc_agi_melee_1h_axe : public AuraScript
{
public:
    spell_item_dragon_soul_proc_agi_melee_1h_axe(std::array<uint32, 3> const& procSpellIds) : AuraScript(), _procSpellIds(procSpellIds) { }

    bool Validate(SpellInfo const* /*spell*/) override
    {
        return ValidateSpellInfo(_procSpellIds);
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        uint32 spellId = Trinity::Containers::SelectRandomContainerElement(_procSpellIds);
        GetTarget()->CastSpell(eventInfo.GetProcTarget(), spellId, aurEff);

    }

    void Register() override
    {
        OnEffectProc.Register(&spell_item_dragon_soul_proc_agi_melee_1h_axe::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }

private:
    std::array<uint32, 3> _procSpellIds;
};
}

void AddSC_item_spell_scripts()
{
    using namespace Spells::Items;
    // 23074 Arcanite Dragonling
    new spell_item_trigger_spell("spell_item_arcanite_dragonling", SPELL_ARCANITE_DRAGONLING);
    // 23133 Gnomish Battle Chicken
    new spell_item_trigger_spell("spell_item_gnomish_battle_chicken", SPELL_BATTLE_CHICKEN);
    // 23076 Mechanical Dragonling
    new spell_item_trigger_spell("spell_item_mechanical_dragonling", SPELL_MECHANICAL_DRAGONLING);
    // 23075 Mithril Mechanical Dragonling
    new spell_item_trigger_spell("spell_item_mithril_mechanical_dragonling", SPELL_MITHRIL_MECHANICAL_DRAGONLING);

    new spell_item_aegis_of_preservation();
    new spell_item_arcane_shroud();
    new spell_item_alchemists_stone();
    new spell_item_anger_capacitor<8>("spell_item_tiny_abomination_in_a_jar");
    new spell_item_anger_capacitor<7>("spell_item_tiny_abomination_in_a_jar_hero");
    new spell_item_aura_of_madness();
    new spell_item_dementia();
    new spell_item_blessing_of_ancient_kings();
    RegisterSpellScript(spell_item_valanyr_hammer_of_ancient_kings);
    new spell_item_deadly_precision();
    new spell_item_deadly_precision_dummy();
    new spell_item_deathbringers_will<SPELL_STRENGTH_OF_THE_TAUNKA, SPELL_AGILITY_OF_THE_VRYKUL, SPELL_POWER_OF_THE_TAUNKA, SPELL_AIM_OF_THE_IRON_DWARVES, SPELL_SPEED_OF_THE_VRYKUL>("spell_item_deathbringers_will_normal");
    new spell_item_deathbringers_will<SPELL_STRENGTH_OF_THE_TAUNKA_HERO, SPELL_AGILITY_OF_THE_VRYKUL_HERO, SPELL_POWER_OF_THE_TAUNKA_HERO, SPELL_AIM_OF_THE_IRON_DWARVES_HERO, SPELL_SPEED_OF_THE_VRYKUL_HERO>("spell_item_deathbringers_will_heroic");
    new spell_item_decahedral_dwarven_dice();
    new spell_item_defibrillate("spell_item_goblin_jumper_cables", 67, SPELL_GOBLIN_JUMPER_CABLES_FAIL);
    new spell_item_defibrillate("spell_item_goblin_jumper_cables_xl", 50, SPELL_GOBLIN_JUMPER_CABLES_XL_FAIL);
    new spell_item_defibrillate("spell_item_gnomish_army_knife", 33);
    new spell_item_desperate_defense();
    new spell_item_deviate_fish();
    new spell_item_echoes_of_light();
    new spell_item_fate_rune_of_unsurpassed_vigor();
    new spell_item_flask_of_the_north();
    new spell_item_frozen_shadoweave();
    new spell_item_gnomish_death_ray();
    new spell_item_harm_prevention_belt();
    new spell_item_healing_touch_refund();
    new spell_item_heartpierce<SPELL_INVIGORATION_ENERGY, SPELL_INVIGORATION_MANA, SPELL_INVIGORATION_RAGE, SPELL_INVIGORATION_RP>("spell_item_heartpierce");
    new spell_item_heartpierce<SPELL_INVIGORATION_ENERGY_HERO, SPELL_INVIGORATION_MANA_HERO, SPELL_INVIGORATION_RAGE_HERO, SPELL_INVIGORATION_RP_HERO>("spell_item_heartpierce_hero");
    RegisterSpellScript(spell_item_herbouflage);
    new spell_item_crystal_spire_of_karabor();
    new spell_item_make_a_wish();
    new spell_item_mark_of_conquest();
    new spell_item_mingos_fortune_generator();
    new spell_item_necrotic_touch();
    new spell_item_net_o_matic();
    new spell_item_noggenfogger_elixir();
    new spell_item_pendant_of_the_violet_eye();
    new spell_item_persistent_shield();
    new spell_item_pet_healing();
    new spell_item_piccolo_of_the_flaming_fire();
    RegisterSpellScript(spell_item_power_circle);
    new spell_item_savory_deviate_delight();
    new spell_item_scroll_of_recall();
    new spell_item_unsated_craving();
    new spell_item_shadows_fate();
    new spell_item_shadowmourne();
    new spell_item_shadowmourne_soul_fragment();
    new spell_item_six_demon_bag();
    new spell_item_swift_hand_justice_dummy();
    new spell_item_totem_of_flowing_water();
    new spell_item_the_eye_of_diminution();
    new spell_item_underbelly_elixir();
    new spell_item_worn_troll_dice();
    new spell_item_red_rider_air_rifle();

    new spell_item_create_heart_candy();
    new spell_item_book_of_glyph_mastery();
    new spell_item_gift_of_the_harvester();
    new spell_item_map_of_the_geyser_fields();
    new spell_item_vanquished_clutches();

    new spell_item_ashbringer();
    new spell_magic_eater_food();
    new spell_item_shimmering_vessel();
    new spell_item_purify_helboar_meat();
    new spell_item_crystal_prison_dummy_dnd();
    new spell_item_reindeer_transformation();
    new spell_item_nigh_invulnerability();
    new spell_item_poultryizer();
    new spell_item_socrethars_stone();
    new spell_item_demon_broiled_surprise();
    new spell_item_complete_raptor_capture();
    new spell_item_impale_leviroth();
    new spell_item_brewfest_mount_transformation();
    new spell_item_nitro_boosts();
    new spell_item_nitro_boosts_backfire();
    new spell_item_teach_language();
    new spell_item_rocket_boots();
    new spell_item_pygmy_oil();
    new spell_item_unusual_compass();
    new spell_item_chicken_cover();
    new spell_item_muisek_vessel();
    new spell_item_greatmothers_soulcatcher();
    new spell_item_shard_of_the_scale<SPELL_PURIFIED_CAUTERIZING_HEAL, SPELL_PURIFIED_SEARING_FLAMES>("spell_item_purified_shard_of_the_scale");
    new spell_item_shard_of_the_scale<SPELL_SHINY_CAUTERIZING_HEAL, SPELL_SHINY_SEARING_FLAMES>("spell_item_shiny_shard_of_the_scale");
    new spell_item_soul_preserver();
    new spell_item_sunwell_neck<SPELL_LIGHTS_WRATH, SPELL_ARCANE_BOLT>("spell_item_sunwell_exalted_caster_neck");
    new spell_item_sunwell_neck<SPELL_LIGHTS_STRENGTH, SPELL_ARCANE_STRIKE>("spell_item_sunwell_exalted_melee_neck");
    new spell_item_sunwell_neck<SPELL_LIGHTS_WARD, SPELL_ARCANE_INSIGHT>("spell_item_sunwell_exalted_tank_neck");
    new spell_item_sunwell_neck<SPELL_LIGHTS_SALVATION, SPELL_ARCANE_SURGE>("spell_item_sunwell_exalted_healer_neck");
    new spell_item_toy_train_set_pulse();
    new spell_item_taunt_flag_targeting();

    new spell_item_death_choice();
    new spell_item_trinket_stack("spell_item_lightning_capacitor", SPELL_LIGHTNING_CAPACITOR_STACK, SPELL_LIGHTNING_CAPACITOR_TRIGGER);
    new spell_item_trinket_stack("spell_item_thunder_capacitor", SPELL_THUNDER_CAPACITOR_STACK, SPELL_THUNDER_CAPACITOR_TRIGGER);
    new spell_item_trinket_stack("spell_item_toc25_normal_caster_trinket", SPELL_TOC25_CASTER_TRINKET_NORMAL_STACK, SPELL_TOC25_CASTER_TRINKET_NORMAL_TRIGGER);
    new spell_item_trinket_stack("spell_item_toc25_heroic_caster_trinket", SPELL_TOC25_CASTER_TRINKET_HEROIC_STACK, SPELL_TOC25_CASTER_TRINKET_HEROIC_TRIGGER);
    new spell_item_darkmoon_card_greatness();
    new spell_item_charm_witch_doctor();
    new spell_item_mana_drain();

    new spell_item_mind_control_cap();
    new spell_item_universal_remote();
    new spell_item_zandalarian_charm("spell_item_unstable_power", SPELL_UNSTABLE_POWER_AURA_STACK);
    new spell_item_zandalarian_charm("spell_item_restless_strength", SPELL_RESTLESS_STRENGTH_AURA_STACK);

    new spell_item_mad_alchemists_potion();
    new spell_item_crazy_alchemists_potion();
    RegisterSpellScript(spell_item_egg_shell);
    RegisterSpellScript(spell_item_song_of_sorrow);
    RegisterSpellScript(spell_item_crescendo_of_suffering);
    new spell_item_consume_charges("spell_item_forged_fury", SPELL_RAW_FURY);
    RegisterSpellAndAuraScriptPair(spell_item_hearts_judgement, spell_item_hearts_judgement_AuraScript);
    RegisterSpellScript(spell_item_blessing_of_khazgoroth);
    RegisterSpellScript(spell_item_weight_of_a_feather);
    RegisterSpellScript(spell_item_tipping_of_the_scales);
    RegisterSpellScript(spell_item_blind_spot);
    RegisterSpellScript(spell_item_variable_pulse_lightning_capacitor);
    RegisterSpellScript(spell_item_talisman_of_ascendance);
    RegisterSpellScript(spell_item_battle_trance);
    RegisterSpellScript(spell_item_world_queller_focus);
    RegisterSpellScript(spell_item_jom_gabbar);
    RegisterSpellScript(spell_item_satisfied);
    RegisterSpellScript(spell_item_blaze_of_life);
    RegisterSpellScript(spell_item_obsidian_armor);
    RegisterSpellScriptWithArgs(spell_item_dragon_soul_proc_agi_melee_1h_axe, "spell_item_nokaled_the_elements_of_death_lfr", NoKaledProcSpellIdsLFR);
    RegisterSpellScriptWithArgs(spell_item_dragon_soul_proc_agi_melee_1h_axe, "spell_item_nokaled_the_elements_of_death_normal", NoKaledProcSpellIdsNormal);
    RegisterSpellScriptWithArgs(spell_item_dragon_soul_proc_agi_melee_1h_axe, "spell_item_nokaled_the_elements_of_death_heroic", NoKaledProcSpellIdsHeroic);
}
