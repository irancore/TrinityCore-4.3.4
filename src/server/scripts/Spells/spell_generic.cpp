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
 * Scripts for spells with SPELLFAMILY_GENERIC which cannot be included in AI script file
 * of creature using it or can't be bound to any player class.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_gen_"
 */

#include "ScriptMgr.h"
#include "Battleground.h"
#include "CellImpl.h"
#include "Containers.h"
#include "DBCStores.h"
#include "GameTime.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "InstanceScript.h"
#include "Item.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Pet.h"
#include "ReputationMgr.h"
#include "SkillDiscovery.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "Vehicle.h"
#include "CreatureAIImpl.h"

namespace Spells::Generic
{
class spell_gen_absorb0_hitlimit1 : public SpellScriptLoader
{
    public:
        spell_gen_absorb0_hitlimit1() : SpellScriptLoader("spell_gen_absorb0_hitlimit1") { }

        class spell_gen_absorb0_hitlimit1_AuraScript : public AuraScript
        {
        public:
            spell_gen_absorb0_hitlimit1_AuraScript()
            {
                limit = 0;
            }

        private:
            uint32 limit;

            bool Load() override
            {
                // Max absorb stored in 1 dummy effect
                limit = GetSpellInfo()->Effects[EFFECT_1].CalcValue();
                return true;
            }

            void Absorb(AuraEffect* /*aurEff*/, DamageInfo& /*dmgInfo*/, uint32& absorbAmount)
            {
                absorbAmount = std::min(limit, absorbAmount);
            }

            void Register() override
            {
                OnEffectAbsorb.Register(&spell_gen_absorb0_hitlimit1_AuraScript::Absorb, EFFECT_0);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_absorb0_hitlimit1_AuraScript();
        }
};

// 28764 - Adaptive Warding (Frostfire Regalia Set)
enum AdaptiveWarding
{
    SPELL_GEN_ADAPTIVE_WARDING_FIRE     = 28765,
    SPELL_GEN_ADAPTIVE_WARDING_NATURE   = 28768,
    SPELL_GEN_ADAPTIVE_WARDING_FROST    = 28766,
    SPELL_GEN_ADAPTIVE_WARDING_SHADOW   = 28769,
    SPELL_GEN_ADAPTIVE_WARDING_ARCANE   = 28770
};

class spell_gen_adaptive_warding : public SpellScriptLoader
{
    public:
        spell_gen_adaptive_warding() : SpellScriptLoader("spell_gen_adaptive_warding") { }

        class spell_gen_adaptive_warding_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_GEN_ADAPTIVE_WARDING_FIRE,
                    SPELL_GEN_ADAPTIVE_WARDING_NATURE,
                    SPELL_GEN_ADAPTIVE_WARDING_FROST,
                    SPELL_GEN_ADAPTIVE_WARDING_SHADOW,
                    SPELL_GEN_ADAPTIVE_WARDING_ARCANE
                });
            }

            bool CheckProc(ProcEventInfo& eventInfo)
            {
                DamageInfo* damageInfo = eventInfo.GetDamageInfo();
                if (!damageInfo || !damageInfo->GetSpellInfo())
                    return false;

                // find Mage Armor
                if (!GetTarget()->GetAuraEffect(SPELL_AURA_MOD_MANA_REGEN_INTERRUPT, SPELLFAMILY_MAGE, 0x10000000, 0x0, 0x0))
                    return false;

                switch (GetFirstSchoolInMask(eventInfo.GetSchoolMask()))
                {
                    case SPELL_SCHOOL_NORMAL:
                    case SPELL_SCHOOL_HOLY:
                        return false;
                    default:
                        break;
                }
                return true;
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();

                uint32 spellId = 0;
                switch (GetFirstSchoolInMask(eventInfo.GetSchoolMask()))
                {
                    case SPELL_SCHOOL_FIRE:
                        spellId = SPELL_GEN_ADAPTIVE_WARDING_FIRE;
                        break;
                    case SPELL_SCHOOL_NATURE:
                        spellId = SPELL_GEN_ADAPTIVE_WARDING_NATURE;
                        break;
                    case SPELL_SCHOOL_FROST:
                        spellId = SPELL_GEN_ADAPTIVE_WARDING_FROST;
                        break;
                    case SPELL_SCHOOL_SHADOW:
                        spellId = SPELL_GEN_ADAPTIVE_WARDING_SHADOW;
                        break;
                    case SPELL_SCHOOL_ARCANE:
                        spellId = SPELL_GEN_ADAPTIVE_WARDING_ARCANE;
                        break;
                    default:
                        return;
                }
                GetTarget()->CastSpell(GetTarget(), spellId, aurEff);
            }

            void Register() override
            {
                DoCheckProc.Register(&spell_gen_adaptive_warding_AuraScript::CheckProc);
                OnEffectProc.Register(&spell_gen_adaptive_warding_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_adaptive_warding_AuraScript();
        }
};

enum AlchemistStone
{
    ALCHEMIST_STONE_HEAL      = 21399,
    ALCHEMIST_STONE_MANA      = 21400,
};

// 17619 - Alchemist Stone
class spell_gen_alchemist_stone : public SpellScriptLoader
{
    public:
        spell_gen_alchemist_stone() : SpellScriptLoader("spell_gen_alchemist_stone") { }

        class spell_gen_alchemist_stone_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ ALCHEMIST_STONE_HEAL, ALCHEMIST_STONE_MANA });
            }

            bool CheckProc(ProcEventInfo& eventInfo)
            {
                return eventInfo.GetDamageInfo()->GetSpellInfo()->SpellFamilyName == SPELLFAMILY_POTION;
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();

                uint32 spellId = 0;
                int32 bp = int32(eventInfo.GetDamageInfo()->GetDamage() * 0.4f);

                if (eventInfo.GetDamageInfo()->GetSpellInfo()->HasEffect(SPELL_EFFECT_HEAL))
                    spellId = ALCHEMIST_STONE_HEAL;
                else if (eventInfo.GetDamageInfo()->GetSpellInfo()->HasEffect(SPELL_EFFECT_ENERGIZE))
                    spellId = ALCHEMIST_STONE_MANA;

                if (!spellId)
                    return;

                GetTarget()->CastSpell(GetTarget(), spellId, CastSpellExtraArgs(aurEff).AddSpellBP0(bp));
            }


            void Register() override
            {
                DoCheckProc.Register(&spell_gen_alchemist_stone_AuraScript::CheckProc);
                OnEffectProc.Register(&spell_gen_alchemist_stone_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_alchemist_stone_AuraScript();
        }
};

class spell_gen_allow_cast_from_item_only : public SpellScriptLoader
{
    public:
        spell_gen_allow_cast_from_item_only() : SpellScriptLoader("spell_gen_allow_cast_from_item_only") { }

        class spell_gen_allow_cast_from_item_only_SpellScript : public SpellScript
        {
            SpellCastResult CheckRequirement()
            {
                if (!GetCastItem())
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
                return SPELL_CAST_OK;
            }

            void Register() override
            {
                OnCheckCast.Register(&spell_gen_allow_cast_from_item_only_SpellScript::CheckRequirement);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_allow_cast_from_item_only_SpellScript();
        }
};

enum AnimalBloodPoolSpell
{
    SPELL_ANIMAL_BLOOD      = 46221,
    SPELL_SPAWN_BLOOD_POOL  = 63471
};

class spell_gen_animal_blood : public SpellScriptLoader
{
    public:
        spell_gen_animal_blood() : SpellScriptLoader("spell_gen_animal_blood") { }

        class spell_gen_animal_blood_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_SPAWN_BLOOD_POOL });
            }

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                // Remove all auras with spell id 46221, except the one currently being applied
                while (Aura* aur = GetUnitOwner()->GetOwnedAura(SPELL_ANIMAL_BLOOD, ObjectGuid::Empty, ObjectGuid::Empty, 0, GetAura()))
                    GetUnitOwner()->RemoveOwnedAura(aur);
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* owner = GetUnitOwner())
                    if (owner->IsInWater())
                        owner->CastSpell(owner, SPELL_SPAWN_BLOOD_POOL, true);
            }

            void Register() override
            {
                AfterEffectApply.Register(&spell_gen_animal_blood_AuraScript::OnApply, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
                AfterEffectRemove.Register(&spell_gen_animal_blood_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_animal_blood_AuraScript();
        }
};

// 41337 Aura of Anger
class spell_gen_aura_of_anger : public SpellScriptLoader
{
    public:
        spell_gen_aura_of_anger() : SpellScriptLoader("spell_gen_aura_of_anger") { }

        class spell_gen_aura_of_anger_AuraScript : public AuraScript
        {
            void HandleEffectPeriodicUpdate(AuraEffect* aurEff)
            {
                if (AuraEffect* aurEff1 = aurEff->GetBase()->GetEffect(EFFECT_1))
                    aurEff1->ChangeAmount(aurEff1->GetAmount() + 5);
                aurEff->SetAmount(100 * aurEff->GetTickNumber());
            }

            void Register() override
            {
                OnEffectUpdatePeriodic.Register(&spell_gen_aura_of_anger_AuraScript::HandleEffectPeriodicUpdate, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_aura_of_anger_AuraScript();
        }
};

enum ServiceUniform
{
    // Spells
    SPELL_SERVICE_UNIFORM       = 71450,

    // Models
    MODEL_GOBLIN_MALE           = 31002,
    MODEL_GOBLIN_FEMALE         = 31003
};

class spell_gen_aura_service_uniform : public SpellScriptLoader
{
    public:
        spell_gen_aura_service_uniform() : SpellScriptLoader("spell_gen_aura_service_uniform") { }

        class spell_gen_aura_service_uniform_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_SERVICE_UNIFORM });
            }

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                // Apply model goblin
                Unit* target = GetTarget();
                if (target->GetTypeId() == TYPEID_PLAYER)
                {
                    if (target->getGender() == GENDER_MALE)
                        target->SetDisplayId(MODEL_GOBLIN_MALE);
                    else
                        target->SetDisplayId(MODEL_GOBLIN_FEMALE);
                }
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                if (target->GetTypeId() == TYPEID_PLAYER)
                    target->RestoreDisplayId();
            }

            void Register() override
            {
                AfterEffectApply.Register(&spell_gen_aura_service_uniform_AuraScript::OnApply, EFFECT_0, SPELL_AURA_TRANSFORM, AURA_EFFECT_HANDLE_REAL);
                AfterEffectRemove.Register(&spell_gen_aura_service_uniform_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_TRANSFORM, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_aura_service_uniform_AuraScript();
        }
};

class spell_gen_av_drekthar_presence : public SpellScriptLoader
{
    public:
        spell_gen_av_drekthar_presence() : SpellScriptLoader("spell_gen_av_drekthar_presence") { }

        class spell_gen_av_drekthar_presence_AuraScript : public AuraScript
        {
            bool CheckAreaTarget(Unit* target)
            {
                switch (target->GetEntry())
                {
                    // alliance
                    case 14762: // Dun Baldar North Marshal
                    case 14763: // Dun Baldar South Marshal
                    case 14764: // Icewing Marshal
                    case 14765: // Stonehearth Marshal
                    case 11948: // Vandar Stormspike
                    // horde
                    case 14772: // East Frostwolf Warmaster
                    case 14776: // Tower Point Warmaster
                    case 14773: // Iceblood Warmaster
                    case 14777: // West Frostwolf Warmaster
                    case 11946: // Drek'thar
                        return true;
                    default:
                        return false;
                }
            }

            void Register() override
            {
                DoCheckAreaTarget.Register(&spell_gen_av_drekthar_presence_AuraScript::CheckAreaTarget);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_av_drekthar_presence_AuraScript();
        }
};

enum GenericBandage
{
    SPELL_RECENTLY_BANDAGED     = 11196
};

class spell_gen_bandage : public SpellScriptLoader
{
    public:
        spell_gen_bandage() : SpellScriptLoader("spell_gen_bandage") { }

        class spell_gen_bandage_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_RECENTLY_BANDAGED });
            }

            SpellCastResult CheckCast()
            {
                if (Unit* target = GetExplTargetUnit())
                {
                    if (target->HasAura(SPELL_RECENTLY_BANDAGED))
                        return SPELL_FAILED_TARGET_AURASTATE;
                }
                return SPELL_CAST_OK;
            }

            void HandleScript()
            {
                if (Unit* target = GetHitUnit())
                    GetCaster()->CastSpell(target, SPELL_RECENTLY_BANDAGED, true);
            }

            void Register() override
            {
                OnCheckCast.Register(&spell_gen_bandage_SpellScript::CheckCast);
                AfterHit.Register(&spell_gen_bandage_SpellScript::HandleScript);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_bandage_SpellScript();
        }
};

// Blood Reserve - 64568
enum BloodReserve
{
    SPELL_GEN_BLOOD_RESERVE_AURA = 64568,
    SPELL_GEN_BLOOD_RESERVE_HEAL = 64569
};

class spell_gen_blood_reserve : public SpellScriptLoader
{
    public:
        spell_gen_blood_reserve() : SpellScriptLoader("spell_gen_blood_reserve") { }

        class spell_gen_blood_reserve_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_GEN_BLOOD_RESERVE_HEAL });
            }

            bool CheckProc(ProcEventInfo& eventInfo)
            {
                if (DamageInfo* dmgInfo = eventInfo.GetDamageInfo())
                    if (Unit* caster = eventInfo.GetActionTarget())
                        if (caster->HealthBelowPctDamaged(35, dmgInfo->GetDamage()))
                            return true;

                return false;
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();

                Unit* caster = eventInfo.GetActionTarget();
                caster->CastSpell(caster, SPELL_GEN_BLOOD_RESERVE_HEAL, CastSpellExtraArgs(aurEff).AddSpellBP0(aurEff->GetAmount()));
                caster->RemoveAura(SPELL_GEN_BLOOD_RESERVE_AURA);
            }

            void Register() override
            {
                DoCheckProc.Register(&spell_gen_blood_reserve_AuraScript::CheckProc);
                OnEffectProc.Register(&spell_gen_blood_reserve_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_blood_reserve_AuraScript();
        }
};

// Blade Warding - 64440
enum BladeWarding
{
    SPELL_GEN_BLADE_WARDING_TRIGGERED = 64442
};

class spell_gen_blade_warding : public SpellScriptLoader
{
    public:
        spell_gen_blade_warding() : SpellScriptLoader("spell_gen_blade_warding") { }

        class spell_gen_blade_warding_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_GEN_BLADE_WARDING_TRIGGERED });
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();

                Unit* caster = eventInfo.GetActionTarget();
                SpellInfo const* spellInfo = sSpellMgr->AssertSpellInfo(SPELL_GEN_BLADE_WARDING_TRIGGERED);

                uint8 stacks = GetStackAmount();
                int32 bp = 0;

                for (uint8 i = 0; i < stacks; ++i)
                    bp += spellInfo->Effects[EFFECT_0].CalcValue(caster);

                caster->CastSpell(eventInfo.GetActor(), SPELL_GEN_BLADE_WARDING_TRIGGERED, CastSpellExtraArgs(aurEff).AddSpellBP0(bp));
            }

            void Register() override
            {
                OnEffectProc.Register(&spell_gen_blade_warding_AuraScript::HandleProc, EFFECT_1, SPELL_AURA_PROC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_blade_warding_AuraScript();
        }
};

enum Bonked
{
    SPELL_BONKED            = 62991,
    SPELL_FOAM_SWORD_DEFEAT = 62994,
    SPELL_ON_GUARD          = 62972
};

class spell_gen_bonked : public SpellScriptLoader
{
    public:
        spell_gen_bonked() : SpellScriptLoader("spell_gen_bonked") { }

        class spell_gen_bonked_SpellScript : public SpellScript
        {
            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (Player* target = GetHitPlayer())
                {
                    Aura const* aura = GetHitAura();
                    if (!(aura && aura->GetStackAmount() == 3))
                        return;

                    target->CastSpell(target, SPELL_FOAM_SWORD_DEFEAT, true);
                    target->RemoveAurasDueToSpell(SPELL_BONKED);

                    if (Aura const* auraOnGuard = target->GetAura(SPELL_ON_GUARD))
                    {
                        if (Item* item = target->GetItemByGuid(auraOnGuard->GetCastItemGUID()))
                            target->DestroyItemCount(item->GetEntry(), 1, true);
                    }
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_gen_bonked_SpellScript::HandleScript, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_bonked_SpellScript();
        }
};

/* DOCUMENTATION: Break-Shield spells
    Break-Shield spells can be classified in three groups:

        - Spells on vehicle bar used by players:
            + EFFECT_0: SCRIPT_EFFECT
            + EFFECT_1: NONE
            + EFFECT_2: NONE
        - Spells cast by players triggered by script:
            + EFFECT_0: SCHOOL_DAMAGE
            + EFFECT_1: SCRIPT_EFFECT
            + EFFECT_2: FORCE_CAST
        - Spells cast by NPCs on players:
            + EFFECT_0: SCHOOL_DAMAGE
            + EFFECT_1: SCRIPT_EFFECT
            + EFFECT_2: NONE

    In the following script we handle the SCRIPT_EFFECT for effIndex EFFECT_0 and EFFECT_1.
        - When handling EFFECT_0 we're in the "Spells on vehicle bar used by players" case
          and we'll trigger "Spells cast by players triggered by script"
        - When handling EFFECT_1 we're in the "Spells cast by players triggered by script"
          or "Spells cast by NPCs on players" so we'll search for the first defend layer and drop it.
*/

enum BreakShieldSpells
{
    SPELL_BREAK_SHIELD_DAMAGE_2K                 = 62626,
    SPELL_BREAK_SHIELD_DAMAGE_10K                = 64590,

    SPELL_BREAK_SHIELD_TRIGGER_FACTION_MOUNTS    = 62575, // Also on ToC5 mounts
    SPELL_BREAK_SHIELD_TRIGGER_CAMPAING_WARHORSE = 64595,
    SPELL_BREAK_SHIELD_TRIGGER_UNK               = 66480
};

class spell_gen_break_shield: public SpellScriptLoader
{
    public:
        spell_gen_break_shield(char const* name) : SpellScriptLoader(name) { }

        class spell_gen_break_shield_SpellScript : public SpellScript
        {
            void HandleScriptEffect(SpellEffIndex effIndex)
            {
                Unit* target = GetHitUnit();

                switch (effIndex)
                {
                    case EFFECT_0: // On spells wich trigger the damaging spell (and also the visual)
                    {
                        uint32 spellId;

                        switch (GetSpellInfo()->Id)
                        {
                            case SPELL_BREAK_SHIELD_TRIGGER_UNK:
                            case SPELL_BREAK_SHIELD_TRIGGER_CAMPAING_WARHORSE:
                                spellId = SPELL_BREAK_SHIELD_DAMAGE_10K;
                                break;
                            case SPELL_BREAK_SHIELD_TRIGGER_FACTION_MOUNTS:
                                spellId = SPELL_BREAK_SHIELD_DAMAGE_2K;
                                break;
                            default:
                                return;
                        }

                        if (Unit* rider = GetCaster()->GetCharmer())
                            rider->CastSpell(target, spellId, false);
                        else
                            GetCaster()->CastSpell(target, spellId, false);
                        break;
                    }
                    case EFFECT_1: // On damaging spells, for removing a defend layer
                    {
                        Unit::AuraApplicationMap const& auras = target->GetAppliedAuras();
                        for (Unit::AuraApplicationMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                        {
                            if (Aura* aura = itr->second->GetBase())
                            {
                                SpellInfo const* auraInfo = aura->GetSpellInfo();
                                if (auraInfo && auraInfo->SpellIconID == 2007 && aura->HasEffectType(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN))
                                {
                                    aura->ModStackAmount(-1, AuraRemoveFlags::ByEnemySpell);
                                    // Remove dummys from rider (Necessary for updating visual shields)
                                    if (Unit* rider = target->GetCharmer())
                                        if (Aura* defend = rider->GetAura(aura->GetId()))
                                            defend->ModStackAmount(-1, AuraRemoveFlags::ByEnemySpell);
                                    break;
                                }
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_gen_break_shield_SpellScript::HandleScriptEffect, EFFECT_FIRST_FOUND, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_break_shield_SpellScript();
        }
};

// 46394 Brutallus Burn
class spell_gen_burn_brutallus : public SpellScriptLoader
{
    public:
        spell_gen_burn_brutallus() : SpellScriptLoader("spell_gen_burn_brutallus") { }

        class spell_gen_burn_brutallus_AuraScript : public AuraScript
        {
            void HandleEffectPeriodicUpdate(AuraEffect* aurEff)
            {
                if (aurEff->GetTickNumber() % 11 == 0)
                    aurEff->SetAmount(aurEff->GetAmount() * 2);
            }

            void Register() override
            {
                OnEffectUpdatePeriodic.Register(&spell_gen_burn_brutallus_AuraScript::HandleEffectPeriodicUpdate, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_burn_brutallus_AuraScript();
        }
};

// 48750 - Burning Depths Necrolyte Image
class spell_gen_burning_depths_necrolyte_image : public SpellScriptLoader
{
    public:
        spell_gen_burning_depths_necrolyte_image() : SpellScriptLoader("spell_gen_burning_depths_necrolyte_image") { }

        class spell_gen_burning_depths_necrolyte_image_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* spellInfo) override
            {
                return ValidateSpellInfo({ uint32(spellInfo->Effects[EFFECT_2].CalcValue()) });
            }

            void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                    caster->CastSpell(GetTarget(), uint32(GetSpellInfo()->Effects[EFFECT_2].CalcValue()));
            }

            void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                GetTarget()->RemoveAurasDueToSpell(uint32(GetSpellInfo()->Effects[EFFECT_2].CalcValue()), GetCasterGUID());
            }

            void Register() override
            {
                AfterEffectApply.Register(&spell_gen_burning_depths_necrolyte_image_AuraScript::HandleApply, EFFECT_0, SPELL_AURA_TRANSFORM, AURA_EFFECT_HANDLE_REAL);
                AfterEffectRemove.Register(&spell_gen_burning_depths_necrolyte_image_AuraScript::HandleRemove, EFFECT_0, SPELL_AURA_TRANSFORM, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_burning_depths_necrolyte_image_AuraScript();
        }
};

enum CannibalizeSpells
{
    SPELL_CANNIBALIZE_TRIGGERED = 20578
};

class spell_gen_cannibalize : public SpellScriptLoader
{
    public:
        spell_gen_cannibalize() : SpellScriptLoader("spell_gen_cannibalize") { }

        class spell_gen_cannibalize_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_CANNIBALIZE_TRIGGERED });
            }

            SpellCastResult CheckIfCorpseNear()
            {
                Unit* caster = GetCaster();
                float max_range = GetSpellInfo()->GetMaxRange(false);
                WorldObject* result = nullptr;
                // search for nearby enemy corpse in range
                Trinity::AnyDeadUnitSpellTargetInRangeCheck check(caster, max_range, GetSpellInfo(), TARGET_CHECK_ENEMY);
                Trinity::WorldObjectSearcher<Trinity::AnyDeadUnitSpellTargetInRangeCheck> searcher(caster, result, check);
                Cell::VisitWorldObjects(caster, searcher, max_range);
                if (!result)
                    Cell::VisitGridObjects(caster, searcher, max_range);
                if (!result)
                    return SPELL_FAILED_NO_EDIBLE_CORPSES;
                return SPELL_CAST_OK;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                GetCaster()->CastSpell(GetCaster(), SPELL_CANNIBALIZE_TRIGGERED, false);
            }

            void Register() override
            {
                OnEffectHit.Register(&spell_gen_cannibalize_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
                OnCheckCast.Register(&spell_gen_cannibalize_SpellScript::CheckIfCorpseNear);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_cannibalize_SpellScript();
        }
};

enum ChaosBlast
{
    SPELL_CHAOS_BLAST   = 37675
};

class spell_gen_chaos_blast : public SpellScriptLoader
{
    public:
        spell_gen_chaos_blast() : SpellScriptLoader("spell_gen_chaos_blast") { }

        class spell_gen_chaos_blast_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_CHAOS_BLAST });
            }
            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                int32 basepoints0 = 100;
                Unit* caster = GetCaster();
                if (Unit* target = GetHitUnit())
                    caster->CastSpell(target, SPELL_CHAOS_BLAST, CastSpellExtraArgs(true).AddSpellBP0(basepoints0));
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_gen_chaos_blast_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_chaos_blast_SpellScript();
        }
};

enum Clone
{
    SPELL_CLONE_ME                              = 45204,
    SPELL_NIGHTMARE_FIGMENT_MIRROR_IMAGE        = 57528
};

// 5138 - Drain Mana
// 8129 - Mana Burn
class spell_gen_clear_fear_poly : public SpellScript
{
    void HandleAfterHit()
    {
        if (Unit* unitTarget = GetHitUnit())
            unitTarget->RemoveAurasWithMechanic((1 << MECHANIC_FEAR) | (1 << MECHANIC_POLYMORPH));
    }

    void Register() override
    {
        AfterHit.Register(&spell_gen_clear_fear_poly::HandleAfterHit);
    }
};

class spell_gen_clone : public SpellScriptLoader
{
    public:
        spell_gen_clone() : SpellScriptLoader("spell_gen_clone") { }

        class spell_gen_clone_SpellScript : public SpellScript
        {
            void HandleScriptEffect(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                GetHitUnit()->CastSpell(GetCaster(), uint32(GetEffectValue()), true);
            }

            void Register() override
            {
                switch (m_scriptSpellId)
                {
                    case SPELL_CLONE_ME:
                        OnEffectHitTarget.Register(&spell_gen_clone_SpellScript::HandleScriptEffect, EFFECT_2, SPELL_EFFECT_SCRIPT_EFFECT);
                        break;
                    case SPELL_NIGHTMARE_FIGMENT_MIRROR_IMAGE:
                        OnEffectHitTarget.Register(&spell_gen_clone_SpellScript::HandleScriptEffect, EFFECT_1, SPELL_EFFECT_DUMMY);
                        OnEffectHitTarget.Register(&spell_gen_clone_SpellScript::HandleScriptEffect, EFFECT_2, SPELL_EFFECT_DUMMY);
                        break;
                    default:
                        OnEffectHitTarget.Register(&spell_gen_clone_SpellScript::HandleScriptEffect, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
                        OnEffectHitTarget.Register(&spell_gen_clone_SpellScript::HandleScriptEffect, EFFECT_2, SPELL_EFFECT_SCRIPT_EFFECT);
                        break;
                }
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_clone_SpellScript();
        }
};

enum CloneWeaponSpells
{
    SPELL_COPY_WEAPON_AURA       = 41054,
    SPELL_COPY_WEAPON_2_AURA     = 63418,
    SPELL_COPY_WEAPON_3_AURA     = 69893,

    SPELL_COPY_OFFHAND_AURA      = 45205,
    SPELL_COPY_OFFHAND_2_AURA    = 69896,

    SPELL_COPY_RANGED_AURA       = 57594
};

class spell_gen_clone_weapon : public SpellScriptLoader
{
    public:
        spell_gen_clone_weapon() : SpellScriptLoader("spell_gen_clone_weapon") { }

        class spell_gen_clone_weapon_SpellScript : public SpellScript
        {
            void HandleScriptEffect(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                GetHitUnit()->CastSpell(GetCaster(), uint32(GetEffectValue()), true);
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_gen_clone_weapon_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_clone_weapon_SpellScript();
        }
};

class spell_gen_clone_weapon_aura : public SpellScriptLoader
{
    public:
        spell_gen_clone_weapon_aura() : SpellScriptLoader("spell_gen_clone_weapon_aura") { }

        class spell_gen_clone_weapon_auraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_COPY_WEAPON_AURA,
                    SPELL_COPY_WEAPON_2_AURA,
                    SPELL_COPY_WEAPON_3_AURA,
                    SPELL_COPY_OFFHAND_AURA,
                    SPELL_COPY_OFFHAND_2_AURA,
                    SPELL_COPY_RANGED_AURA
                });
            }

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetTarget();
                if (!caster)
                    return;

                switch (GetSpellInfo()->Id)
                {
                    case SPELL_COPY_WEAPON_AURA:
                    case SPELL_COPY_WEAPON_2_AURA:
                    case SPELL_COPY_WEAPON_3_AURA:
                    {
                        prevItem = target->GetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID);

                        if (Player* player = caster->ToPlayer())
                        {
                            if (Item* mainItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND))
                                target->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, mainItem->GetEntry());
                        }
                        else
                            target->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, caster->GetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID));
                        break;
                    }
                    case SPELL_COPY_OFFHAND_AURA:
                    case SPELL_COPY_OFFHAND_2_AURA:
                    {
                        prevItem = target->GetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID) + 1;

                        if (Player* player = caster->ToPlayer())
                        {
                            if (Item* offItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND))
                                target->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, offItem->GetEntry());
                        }
                        else
                            target->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, caster->GetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1));
                        break;
                    }
                    case SPELL_COPY_RANGED_AURA:
                    {
                        prevItem = target->GetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID) + 2;

                        if (Player* player = caster->ToPlayer())
                        {
                            if (Item* rangedItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED))
                                target->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 2, rangedItem->GetEntry());
                        }
                        else
                            target->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 2, caster->GetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 2));
                        break;
                    }
                    default:
                        break;
                }
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();

                switch (GetSpellInfo()->Id)
                {
                    case SPELL_COPY_WEAPON_AURA:
                    case SPELL_COPY_WEAPON_2_AURA:
                    case SPELL_COPY_WEAPON_3_AURA:
                        target->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, prevItem);
                        break;
                    case SPELL_COPY_OFFHAND_AURA:
                    case SPELL_COPY_OFFHAND_2_AURA:
                        target->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, prevItem);
                        break;
                    case SPELL_COPY_RANGED_AURA:
                        target->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 2, prevItem);
                        break;
                    default:
                        break;
                }
            }

            void Register() override
            {
                OnEffectApply.Register(&spell_gen_clone_weapon_auraScript::OnApply, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
                OnEffectRemove.Register(&spell_gen_clone_weapon_auraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }

            uint32 prevItem = 0;
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_clone_weapon_auraScript();
        }
};

class spell_gen_count_pct_from_max_hp : public SpellScriptLoader
{
    public:
        spell_gen_count_pct_from_max_hp(char const* name, int32 damagePct = 0) : SpellScriptLoader(name), _damagePct(damagePct) { }

        class spell_gen_count_pct_from_max_hp_SpellScript : public SpellScript
        {
        public:
            spell_gen_count_pct_from_max_hp_SpellScript(int32 damagePct) : SpellScript(), _damagePct(damagePct) { }

            void RecalculateDamage()
            {
                if (!_damagePct)
                    _damagePct = GetHitDamage();

                SetHitDamage(GetHitUnit()->CountPctFromMaxHealth(_damagePct));
            }

            void Register() override
            {
                OnHit.Register(&spell_gen_count_pct_from_max_hp_SpellScript::RecalculateDamage);
            }

        private:
            int32 _damagePct;
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_count_pct_from_max_hp_SpellScript(_damagePct);
        }

    private:
        int32 _damagePct;
};

// 63845 - Create Lance
enum CreateLanceSpells
{
    SPELL_CREATE_LANCE_ALLIANCE = 63914,
    SPELL_CREATE_LANCE_HORDE    = 63919
};

class spell_gen_create_lance : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_CREATE_LANCE_ALLIANCE,
                SPELL_CREATE_LANCE_HORDE
            });
    }

    void HandleScript(SpellEffIndex effIndex)
    {
        PreventHitDefaultEffect(effIndex);

        GameObject* caster = GetGObjCaster();
        if (!caster)
            return;

        if (Player* target = GetHitPlayer())
        {
            if (target->GetTeam() == ALLIANCE)
                caster->CastSpell(target, SPELL_CREATE_LANCE_ALLIANCE, true);
            else
                caster->CastSpell(target, SPELL_CREATE_LANCE_HORDE, true);
        }
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_create_lance::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

enum DalaranDisguiseSpells
{
    SPELL_SUNREAVER_DISGUISE_TRIGGER       = 69672,
    SPELL_SUNREAVER_DISGUISE_FEMALE        = 70973,
    SPELL_SUNREAVER_DISGUISE_MALE          = 70974,

    SPELL_SILVER_COVENANT_DISGUISE_TRIGGER = 69673,
    SPELL_SILVER_COVENANT_DISGUISE_FEMALE  = 70971,
    SPELL_SILVER_COVENANT_DISGUISE_MALE    = 70972
};

class spell_gen_dalaran_disguise : public SpellScriptLoader
{
    public:
        spell_gen_dalaran_disguise(char const* name) : SpellScriptLoader(name) { }

        class spell_gen_dalaran_disguise_SpellScript : public SpellScript
        {
                        bool Validate(SpellInfo const* spellInfo) override
            {
                switch (spellInfo->Id)
                {
                    case SPELL_SUNREAVER_DISGUISE_TRIGGER:
                        return ValidateSpellInfo(
                        {
                            SPELL_SUNREAVER_DISGUISE_FEMALE,
                            SPELL_SUNREAVER_DISGUISE_MALE
                        });
                    case SPELL_SILVER_COVENANT_DISGUISE_TRIGGER:
                        return ValidateSpellInfo(
                        {
                            SPELL_SILVER_COVENANT_DISGUISE_FEMALE,
                            SPELL_SILVER_COVENANT_DISGUISE_MALE
                        });
                    default:
                        break;
                }

                return false;
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (Player* player = GetHitPlayer())
                {
                    uint8 gender = player->getGender();

                    uint32 spellId = GetSpellInfo()->Id;

                    switch (spellId)
                    {
                        case SPELL_SUNREAVER_DISGUISE_TRIGGER:
                            spellId = gender ? SPELL_SUNREAVER_DISGUISE_FEMALE : SPELL_SUNREAVER_DISGUISE_MALE;
                            break;
                        case SPELL_SILVER_COVENANT_DISGUISE_TRIGGER:
                            spellId = gender ? SPELL_SILVER_COVENANT_DISGUISE_FEMALE : SPELL_SILVER_COVENANT_DISGUISE_MALE;
                            break;
                        default:
                            break;
                    }

                    GetCaster()->CastSpell(player, spellId, true);
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_gen_dalaran_disguise_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_dalaran_disguise_SpellScript();
        }
};

class spell_gen_decay_over_time : public SpellScriptLoader
{
    public:
        spell_gen_decay_over_time(char const* name) : SpellScriptLoader(name) { }

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_decay_over_time_SpellScript();
        }

    private:
        class spell_gen_decay_over_time_SpellScript : public SpellScript
        {
            void ModAuraStack()
            {
                if (Aura* aur = GetHitAura())
                    aur->SetStackAmount(static_cast<uint8>(GetSpellInfo()->StackAmount));
            }

            void Register() override
            {
                AfterHit.Register(&spell_gen_decay_over_time_SpellScript::ModAuraStack);
            }
        };

    protected:
        class spell_gen_decay_over_time_AuraScript : public AuraScript
        {
            protected:
                bool CheckProc(ProcEventInfo& eventInfo)
                {
                    return (eventInfo.GetSpellInfo() == GetSpellInfo());
                }

                void Decay(ProcEventInfo& /*eventInfo*/)
                {
                    PreventDefaultAction();
                    ModStackAmount(-1);
                }

                void Register() override
                {
                    DoCheckProc.Register(&spell_gen_decay_over_time_AuraScript::CheckProc);
                    OnProc.Register(&spell_gen_decay_over_time_AuraScript::Decay);
                }

                ~spell_gen_decay_over_time_AuraScript() = default;
        };

        ~spell_gen_decay_over_time() = default;
};

enum FungalDecay
{
    // found in sniffs, there is no duration entry we can possibly use
    AURA_DURATION = 12600
};

// 32065 - Fungal Decay
class spell_gen_decay_over_time_fungal_decay : public spell_gen_decay_over_time
{
    public:
        spell_gen_decay_over_time_fungal_decay() : spell_gen_decay_over_time("spell_gen_decay_over_time_fungal_decay") { }

        class spell_gen_decay_over_time_fungal_decay_AuraScript : public spell_gen_decay_over_time_AuraScript
        {
            void ModDuration(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                // only on actual reapply, not on stack decay
                if (GetDuration() == GetMaxDuration())
                {
                    SetMaxDuration(AURA_DURATION);
                    SetDuration(AURA_DURATION);
                }
            }

            void Register() override
            {
                spell_gen_decay_over_time_AuraScript::Register();
                OnEffectApply.Register(&spell_gen_decay_over_time_fungal_decay_AuraScript::ModDuration, EFFECT_0, SPELL_AURA_MOD_DECREASE_SPEED, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_decay_over_time_fungal_decay_AuraScript();
        }
};

// 36659 - Tail Sting
class spell_gen_decay_over_time_tail_sting : public spell_gen_decay_over_time
{
    public:
        spell_gen_decay_over_time_tail_sting() : spell_gen_decay_over_time("spell_gen_decay_over_time_tail_sting") { }

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_decay_over_time_AuraScript();
        }
};

enum DefendVisuals
{
    SPELL_VISUAL_SHIELD_1 = 63130,
    SPELL_VISUAL_SHIELD_2 = 63131,
    SPELL_VISUAL_SHIELD_3 = 63132
};

class spell_gen_defend : public SpellScriptLoader
{
    public:
        spell_gen_defend() : SpellScriptLoader("spell_gen_defend") { }

        class spell_gen_defend_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_VISUAL_SHIELD_1,
                    SPELL_VISUAL_SHIELD_2,
                    SPELL_VISUAL_SHIELD_3
                });
            }

            void RefreshVisualShields(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (GetCaster())
                {
                    Unit* target = GetTarget();

                    for (uint8 i = 0; i < GetSpellInfo()->StackAmount; ++i)
                        target->RemoveAurasDueToSpell(SPELL_VISUAL_SHIELD_1 + i);

                    target->CastSpell(target, SPELL_VISUAL_SHIELD_1 + GetAura()->GetStackAmount() - 1, aurEff);
                }
                else
                    GetTarget()->RemoveAurasDueToSpell(GetId());
            }

            void RemoveVisualShields(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                for (uint8 i = 0; i < GetSpellInfo()->StackAmount; ++i)
                    GetTarget()->RemoveAurasDueToSpell(SPELL_VISUAL_SHIELD_1 + i);
            }

            void RemoveDummyFromDriver(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                    if (TempSummon* vehicle = caster->ToTempSummon())
                        if (Unit* rider = vehicle->GetSummoner())
                            rider->RemoveAurasDueToSpell(GetId());
            }

            void Register() override
            {
                SpellInfo const* spell = sSpellMgr->AssertSpellInfo(m_scriptSpellId);

                // Defend spells cast by NPCs (add visuals)
                if (spell->Effects[EFFECT_0].ApplyAuraName == SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN)
                {
                    AfterEffectApply.Register(&spell_gen_defend_AuraScript::RefreshVisualShields, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
                    OnEffectRemove.Register(&spell_gen_defend_AuraScript::RemoveVisualShields, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN, AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK);
                }

                // Remove Defend spell from player when he dismounts
                if (spell->Effects[EFFECT_2].ApplyAuraName == SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN)
                    OnEffectRemove.Register(&spell_gen_defend_AuraScript::RemoveDummyFromDriver, EFFECT_2, SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN, AURA_EFFECT_HANDLE_REAL);

                // Defend spells cast by players (add/remove visuals)
                if (spell->Effects[EFFECT_1].ApplyAuraName == SPELL_AURA_DUMMY)
                {
                    AfterEffectApply.Register(&spell_gen_defend_AuraScript::RefreshVisualShields, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
                    OnEffectRemove.Register(&spell_gen_defend_AuraScript::RemoveVisualShields, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK);
                }
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_defend_AuraScript();
        }
};

class spell_gen_despawn_self : public SpellScriptLoader
{
    public:
        spell_gen_despawn_self() : SpellScriptLoader("spell_gen_despawn_self") { }

        class spell_gen_despawn_self_SpellScript : public SpellScript
        {
            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_UNIT;
            }

            void HandleDummy(SpellEffIndex effIndex)
            {
                if (GetSpellInfo()->Effects[effIndex].Effect == SPELL_EFFECT_DUMMY || GetSpellInfo()->Effects[effIndex].Effect == SPELL_EFFECT_SCRIPT_EFFECT)
                    GetCaster()->ToCreature()->DespawnOrUnsummon();
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_gen_despawn_self_SpellScript::HandleDummy, EFFECT_ALL, SPELL_EFFECT_ANY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_despawn_self_SpellScript();
        }
};

enum DivineStormSpell
{
    SPELL_DIVINE_STORM      = 53385,
};

// 70769 Divine Storm!
class spell_gen_divine_storm_cd_reset : public SpellScriptLoader
{
    public:
        spell_gen_divine_storm_cd_reset() : SpellScriptLoader("spell_gen_divine_storm_cd_reset") { }

        class spell_gen_divine_storm_cd_reset_SpellScript : public SpellScript
        {
            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_DIVINE_STORM });
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                Player* caster = GetCaster()->ToPlayer();
                if (caster->GetSpellHistory()->HasCooldown(SPELL_DIVINE_STORM))
                    caster->GetSpellHistory()->ResetCooldown(SPELL_DIVINE_STORM, true);
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_gen_divine_storm_cd_reset_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_divine_storm_cd_reset_SpellScript();
        }
};

class spell_gen_ds_flush_knockback : public SpellScriptLoader
{
    public:
        spell_gen_ds_flush_knockback() : SpellScriptLoader("spell_gen_ds_flush_knockback") { }

        class spell_gen_ds_flush_knockback_SpellScript : public SpellScript
        {
            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                // Here the target is the water spout and determines the position where the player is knocked from
                if (Unit* target = GetHitUnit())
                {
                    if (Player* player = GetCaster()->ToPlayer())
                    {
                        float horizontalSpeed = 20.0f + (40.0f - GetCaster()->GetDistance(target));
                        float verticalSpeed = 8.0f;
                        // This method relies on the Dalaran Sewer map disposition and Water Spout position
                        // What we do is knock the player from a position exactly behind him and at the end of the pipe
                        player->KnockbackFrom(target->GetPositionX(), player->GetPositionY(), horizontalSpeed, verticalSpeed);
                    }
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_gen_ds_flush_knockback_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_ds_flush_knockback_SpellScript();
        }
};

class spell_gen_dungeon_credit : public SpellScriptLoader
{
    public:
        spell_gen_dungeon_credit() : SpellScriptLoader("spell_gen_dungeon_credit") { }

        class spell_gen_dungeon_credit_SpellScript : public SpellScript
        {
            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_UNIT;
            }

            void CreditEncounter()
            {
                // This hook is executed for every target, make sure we only credit instance once
                if (_handled)
                    return;

                _handled = true;
                Unit* caster = GetCaster();
                if (InstanceScript* instance = caster->GetInstanceScript())
                    instance->UpdateEncounterStateForSpellCast(GetSpellInfo()->Id, caster);
            }

            void Register() override
            {
                AfterHit.Register(&spell_gen_dungeon_credit_SpellScript::CreditEncounter);
            }

            bool _handled = false;
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_dungeon_credit_SpellScript();
        }
};

enum EluneCandle
{
    // Creatures
    NPC_OMEN                       = 15467,

    // Spells
    SPELL_ELUNE_CANDLE_OMEN_HEAD   = 26622,
    SPELL_ELUNE_CANDLE_OMEN_CHEST  = 26624,
    SPELL_ELUNE_CANDLE_OMEN_HAND_R = 26625,
    SPELL_ELUNE_CANDLE_OMEN_HAND_L = 26649,
    SPELL_ELUNE_CANDLE_NORMAL      = 26636
};

class spell_gen_elune_candle : public SpellScriptLoader
{
    public:
        spell_gen_elune_candle() : SpellScriptLoader("spell_gen_elune_candle") { }

        class spell_gen_elune_candle_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_ELUNE_CANDLE_OMEN_HEAD,
                    SPELL_ELUNE_CANDLE_OMEN_CHEST,
                    SPELL_ELUNE_CANDLE_OMEN_HAND_R,
                    SPELL_ELUNE_CANDLE_OMEN_HAND_L,
                    SPELL_ELUNE_CANDLE_NORMAL
                });
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                uint32 spellId = 0;

                if (GetHitUnit()->GetEntry() == NPC_OMEN)
                {
                    switch (urand(0, 3))
                    {
                        case 0:
                            spellId = SPELL_ELUNE_CANDLE_OMEN_HEAD;
                            break;
                        case 1:
                            spellId = SPELL_ELUNE_CANDLE_OMEN_CHEST;
                            break;
                        case 2:
                            spellId = SPELL_ELUNE_CANDLE_OMEN_HAND_R;
                            break;
                        case 3:
                            spellId = SPELL_ELUNE_CANDLE_OMEN_HAND_L;
                            break;
                    }
                }
                else
                    spellId = SPELL_ELUNE_CANDLE_NORMAL;

                GetCaster()->CastSpell(GetHitUnit(), spellId, true);
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_gen_elune_candle_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_elune_candle_SpellScript();
        }
};

// 50051 - Ethereal Pet Aura
enum EtherealPet
{
    NPC_ETHEREAL_SOUL_TRADER        = 27914,

    SAY_STEAL_ESSENCE               = 1,
    SAY_CREATE_TOKEN                = 2,

    SPELL_PROC_TRIGGER_ON_KILL_AURA = 50051,
    SPELL_ETHEREAL_PET_AURA         = 50055,
    SPELL_CREATE_TOKEN              = 50063,
    SPELL_STEAL_ESSENCE_VISUAL      = 50101
};

// 50051 - Ethereal Pet Aura
class spell_ethereal_pet_aura : public AuraScript
{
    bool CheckProc(ProcEventInfo& eventInfo)
    {
        uint32 levelDiff = std::abs(GetTarget()->getLevel() - eventInfo.GetProcTarget()->getLevel());
        return levelDiff <= 9;
    }

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        std::list<Creature*> minionList;
        GetUnitOwner()->GetAllMinionsByEntry(minionList, NPC_ETHEREAL_SOUL_TRADER);
        for (Creature* minion : minionList)
        {
            if (minion->IsAIEnabled())
            {
                minion->AI()->Talk(SAY_STEAL_ESSENCE);
                minion->CastSpell(eventInfo.GetProcTarget(), SPELL_STEAL_ESSENCE_VISUAL);
            }
        }
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_ethereal_pet_aura::CheckProc);
        OnEffectProc.Register(&spell_ethereal_pet_aura::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// 50052 - Ethereal Pet onSummon
class spell_ethereal_pet_onsummon : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PROC_TRIGGER_ON_KILL_AURA });
    }

    void HandleScriptEffect(SpellEffIndex /*effIndex*/)
    {
        Unit* target = GetHitUnit();
        target->CastSpell(target, SPELL_PROC_TRIGGER_ON_KILL_AURA, true);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_ethereal_pet_onsummon::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

// 50055 - Ethereal Pet Aura Remove
class spell_ethereal_pet_aura_remove : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ETHEREAL_PET_AURA });
    }

    void HandleScriptEffect(SpellEffIndex /*effIndex*/)
    {
        GetHitUnit()->RemoveAurasDueToSpell(SPELL_ETHEREAL_PET_AURA);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_ethereal_pet_aura_remove::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

// 50101 - Steal Essence Visual
class spell_steal_essence_visual : public AuraScript
{
    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Unit* caster = GetCaster())
        {
            caster->CastSpell(caster, SPELL_CREATE_TOKEN, true);
            if (Creature* soulTrader = caster->ToCreature())
                soulTrader->AI()->Talk(SAY_CREATE_TOKEN);
        }
    }

    void Register() override
    {
        AfterEffectRemove.Register(&spell_steal_essence_visual::HandleRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

/*
There are only 3 possible flags Feign Death auras can apply: UNIT_DYNFLAG_DEAD, UNIT_FLAG2_FEIGN_DEATH
and UNIT_FLAG_PREVENT_EMOTES_FROM_CHAT_TEXT. Some auras can apply only 2 flags

spell_gen_feign_death_all_flags applies all 3 flags
spell_gen_feign_death_all_flags_uninteractible applies all 3 flags and additionally sets UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_UNINTERACTIBLE
spell_gen_feign_death_all_flags_no_uninteractible applies all 3 flags and additionally sets UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC
spell_gen_feign_death_no_dyn_flag applies no UNIT_DYNFLAG_DEAD (does not make the creature appear dead)
spell_gen_feign_death_no_prevent_emotes applies no UNIT_FLAG_PREVENT_EMOTES_FROM_CHAT_TEXT

REACT_PASSIVE should be handled directly in scripts since not all creatures should be passive. Otherwise
creature will be not able to aggro or execute MoveInLineOfSight events. Removing may cause more issues
than already exists
*/

class spell_gen_feign_death_all_flags : public AuraScript
{
    void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        target->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);
        target->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);
        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREVENT_EMOTES_FROM_CHAT_TEXT);

        if (Creature* creature = target->ToCreature())
            creature->SetReactState(REACT_PASSIVE);
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        target->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);
        target->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);
        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREVENT_EMOTES_FROM_CHAT_TEXT);

        if (Creature* creature = target->ToCreature())
            creature->InitializeReactState();
    }

    void Register() override
    {
        OnEffectApply.Register(&spell_gen_feign_death_all_flags::HandleEffectApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        OnEffectRemove.Register(&spell_gen_feign_death_all_flags::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_gen_feign_death_all_flags_uninteractible : public AuraScript
{
    void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        target->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);
        target->SetImmuneToAll(true);
        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREVENT_EMOTES_FROM_CHAT_TEXT | UNIT_FLAG_NOT_SELECTABLE);

        if (Creature* creature = target->ToCreature())
            creature->SetReactState(REACT_PASSIVE);
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        target->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);
        target->SetImmuneToAll(false);
        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREVENT_EMOTES_FROM_CHAT_TEXT| UNIT_FLAG_NOT_SELECTABLE);

        if (Creature* creature = target->ToCreature())
            creature->InitializeReactState();
    }

    void Register() override
    {
        OnEffectApply.Register(&spell_gen_feign_death_all_flags_uninteractible::HandleEffectApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        OnEffectRemove.Register(&spell_gen_feign_death_all_flags_uninteractible::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 96733 - Permanent Feign Death (Stun)
class spell_gen_feign_death_all_flags_no_uninteractible : public AuraScript
{
    void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG2_FEIGN_DEATH);
        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREVENT_EMOTES_FROM_CHAT_TEXT);
        target->SetImmuneToAll(true);

        if (Creature* creature = target->ToCreature())
            creature->SetReactState(REACT_PASSIVE);
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG2_FEIGN_DEATH);
        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREVENT_EMOTES_FROM_CHAT_TEXT);
        target->SetImmuneToAll(false);

        if (Creature* creature = target->ToCreature())
            creature->InitializeReactState();
    }

    void Register() override
    {
        OnEffectApply.Register(&spell_gen_feign_death_all_flags_no_uninteractible::HandleEffectApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        OnEffectRemove.Register(&spell_gen_feign_death_all_flags_no_uninteractible::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 35357 - Spawn Feign Death
// 51329 - Feign Death
class spell_gen_feign_death_no_dyn_flag : public AuraScript
{
    void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        target->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);
        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREVENT_EMOTES_FROM_CHAT_TEXT);

        if (Creature* creature = target->ToCreature())
            creature->SetReactState(REACT_PASSIVE);
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        target->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);
        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREVENT_EMOTES_FROM_CHAT_TEXT);

        if (Creature* creature = target->ToCreature())
            creature->InitializeReactState();
    }

    void Register() override
    {
        OnEffectApply.Register(&spell_gen_feign_death_no_dyn_flag::HandleEffectApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        OnEffectRemove.Register(&spell_gen_feign_death_no_dyn_flag::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 58951 - Permanent Feign Death
class spell_gen_feign_death_no_prevent_emotes : public AuraScript
{
    void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        target->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);
        target->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);

        if (Creature* creature = target->ToCreature())
            creature->SetReactState(REACT_PASSIVE);
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        target->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);
        target->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);

        if (Creature* creature = target->ToCreature())
            creature->InitializeReactState();
    }

    void Register() override
    {
        OnEffectApply.Register(&spell_gen_feign_death_no_prevent_emotes::HandleEffectApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        OnEffectRemove.Register(&spell_gen_feign_death_no_prevent_emotes::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

enum TransporterBackfires
{
    SPELL_TRANSPORTER_MALFUNCTION_POLYMORPH     = 23444,
    SPELL_TRANSPORTER_EVIL_TWIN                 = 23445,
    SPELL_TRANSPORTER_MALFUNCTION_MISS          = 36902
};

class spell_gen_gadgetzan_transporter_backfire : public SpellScriptLoader
{
    public:
        spell_gen_gadgetzan_transporter_backfire() : SpellScriptLoader("spell_gen_gadgetzan_transporter_backfire") { }

        class spell_gen_gadgetzan_transporter_backfire_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_TRANSPORTER_MALFUNCTION_POLYMORPH,
                    SPELL_TRANSPORTER_EVIL_TWIN,
                    SPELL_TRANSPORTER_MALFUNCTION_MISS
                });
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                Unit* caster = GetCaster();
                int32 r = irand(0, 119);
                if (r < 20)                           // Transporter Malfunction - 1/6 polymorph
                    caster->CastSpell(caster, SPELL_TRANSPORTER_MALFUNCTION_POLYMORPH, true);
                else if (r < 100)                     // Evil Twin               - 4/6 evil twin
                    caster->CastSpell(caster, SPELL_TRANSPORTER_EVIL_TWIN, true);
                else                                    // Transporter Malfunction - 1/6 miss the target
                    caster->CastSpell(caster, SPELL_TRANSPORTER_MALFUNCTION_MISS, true);
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_gen_gadgetzan_transporter_backfire_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_gadgetzan_transporter_backfire_SpellScript();
        }
};


class spell_gen_gift_of_naaru : public SpellScriptLoader
{
    public:
        spell_gen_gift_of_naaru() : SpellScriptLoader("spell_gen_gift_of_naaru") { }

        class spell_gen_gift_of_naaru_AuraScript : public AuraScript
        {
            void CalculateAmount(AuraEffect const* aurEff, int32& amount, bool& /*canBeRecalculated*/)
            {
                Unit* target = GetOwner()->ToUnit();
                if (!target)
                    return;

                uint64 health = target->GetMaxHealth();
                int32 healthPct = CalculatePct(health, GetSpellInfo()->Effects[EFFECT_1].BasePoints);

                if (healthPct)
                    amount = std::floor(healthPct / aurEff->GetTotalTicks());
            }

            void Register() override
            {
                DoEffectCalcAmount.Register(&spell_gen_gift_of_naaru_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_PERIODIC_HEAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_gift_of_naaru_AuraScript();
        }
};

enum GnomishTransporter
{
    SPELL_TRANSPORTER_SUCCESS                   = 23441,
    SPELL_TRANSPORTER_FAILURE                   = 23446
};

class spell_gen_gnomish_transporter : public SpellScriptLoader
{
    public:
        spell_gen_gnomish_transporter() : SpellScriptLoader("spell_gen_gnomish_transporter") { }

        class spell_gen_gnomish_transporter_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_TRANSPORTER_SUCCESS,
                    SPELL_TRANSPORTER_FAILURE
                });
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                GetCaster()->CastSpell(GetCaster(), roll_chance_i(50) ? SPELL_TRANSPORTER_SUCCESS : SPELL_TRANSPORTER_FAILURE, true);
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_gen_gnomish_transporter_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_gnomish_transporter_SpellScript();
        }
};

enum Interrupt
{
    SPELL_GEN_INTERRUPT = 32747
};

// 32748 - Deadly Throw Interrupt
// 44835 - Maim Interrupt
class spell_gen_interrupt : public SpellScriptLoader
{
    public:
        spell_gen_interrupt() : SpellScriptLoader("spell_gen_interrupt") { }

        class spell_gen_interrupt_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_GEN_INTERRUPT });
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();
                GetTarget()->CastSpell(eventInfo.GetProcTarget(), SPELL_GEN_INTERRUPT, aurEff);
            }

            void Register() override
            {
                OnEffectProc.Register(&spell_gen_interrupt_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_interrupt_AuraScript();
        }
};

class spell_gen_increase_stats_buff : public SpellScript
{
    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        if (GetHitUnit()->IsInRaidWith(GetCaster()))
            GetCaster()->CastSpell(GetCaster(), GetEffectValue() + 1, true); // raid buff
        else
            GetCaster()->CastSpell(GetHitUnit(), GetEffectValue(), true); // single-target buff
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_increase_stats_buff::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

enum GenericLifebloom
{
    SPELL_HEXLORD_MALACRASS_LIFEBLOOM_FINAL_HEAL        = 43422,
    SPELL_TUR_RAGEPAW_LIFEBLOOM_FINAL_HEAL              = 52552,
    SPELL_CENARION_SCOUT_LIFEBLOOM_FINAL_HEAL           = 53692,
    SPELL_TWISTED_VISAGE_LIFEBLOOM_FINAL_HEAL           = 57763,
    SPELL_FACTION_CHAMPIONS_DRU_LIFEBLOOM_FINAL_HEAL    = 66094
};

class spell_gen_lifebloom : public AuraScript
{
public:
    spell_gen_lifebloom(uint32 spellId) : AuraScript(), _spellId(spellId) { }

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ _spellId });
    }

    void AfterRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        // Final heal only on duration end
        if (!GetTargetApplication()->GetRemoveMode().HasAllFlags(AuraRemoveFlags::Expired | AuraRemoveFlags::ByEnemySpell))
            return;

        // final heal
        GetTarget()->CastSpell(GetTarget(), _spellId, CastSpellExtraArgs(aurEff).SetOriginalCaster(GetCasterGUID()));
    }

    void Register() override
    {
        AfterEffectRemove.Register(&spell_gen_lifebloom::AfterRemove, EFFECT_0, SPELL_AURA_PERIODIC_HEAL, AURA_EFFECT_HANDLE_REAL);
    }

private:
    uint32 _spellId;
};

/* DOCUMENTATION: Charge spells
    Charge spells can be classified in four groups:

        - Spells on vehicle bar used by players:
            + EFFECT_0: SCRIPT_EFFECT
            + EFFECT_1: TRIGGER_SPELL
            + EFFECT_2: NONE
        - Spells cast by player's mounts triggered by script:
            + EFFECT_0: CHARGE
            + EFFECT_1: TRIGGER_SPELL
            + EFFECT_2: APPLY_AURA
        - Spells cast by players on the target triggered by script:
            + EFFECT_0: SCHOOL_DAMAGE
            + EFFECT_1: SCRIPT_EFFECT
            + EFFECT_2: NONE
        - Spells cast by NPCs on players:
            + EFFECT_0: SCHOOL_DAMAGE
            + EFFECT_1: CHARGE
            + EFFECT_2: SCRIPT_EFFECT

    In the following script we handle the SCRIPT_EFFECT and CHARGE
        - When handling SCRIPT_EFFECT:
            + EFFECT_0: Corresponds to "Spells on vehicle bar used by players" and we make player's mount cast
              the charge effect on the current target ("Spells cast by player's mounts triggered by script").
            + EFFECT_1 and EFFECT_2: Triggered when "Spells cast by player's mounts triggered by script" hits target,
              corresponding to "Spells cast by players on the target triggered by script" and "Spells cast by
              NPCs on players" and we check Defend layers and drop a charge of the first found.
        - When handling CHARGE:
            + Only launched for "Spells cast by player's mounts triggered by script", makes the player cast the
              damaging spell on target with a small chance of failing it.
*/

enum ChargeSpells
{
    SPELL_CHARGE_DAMAGE_8K5             = 62874,
    SPELL_CHARGE_DAMAGE_20K             = 68498,
    SPELL_CHARGE_DAMAGE_45K             = 64591,

    SPELL_CHARGE_CHARGING_EFFECT_8K5    = 63661,
    SPELL_CHARGE_CHARGING_EFFECT_20K_1  = 68284,
    SPELL_CHARGE_CHARGING_EFFECT_20K_2  = 68501,
    SPELL_CHARGE_CHARGING_EFFECT_45K_1  = 62563,
    SPELL_CHARGE_CHARGING_EFFECT_45K_2  = 66481,

    SPELL_CHARGE_TRIGGER_FACTION_MOUNTS = 62960,
    SPELL_CHARGE_TRIGGER_TRIAL_CHAMPION = 68282,

    SPELL_CHARGE_MISS_EFFECT            = 62977,
};

class spell_gen_mounted_charge: public SpellScriptLoader
{
    public:
        spell_gen_mounted_charge() : SpellScriptLoader("spell_gen_mounted_charge") { }

        class spell_gen_mounted_charge_SpellScript : public SpellScript
        {
            void HandleScriptEffect(SpellEffIndex effIndex)
            {
                Unit* target = GetHitUnit();

                switch (effIndex)
                {
                    case EFFECT_0: // On spells wich trigger the damaging spell (and also the visual)
                    {
                        uint32 spellId;

                        switch (GetSpellInfo()->Id)
                        {
                            case SPELL_CHARGE_TRIGGER_TRIAL_CHAMPION:
                                spellId = SPELL_CHARGE_CHARGING_EFFECT_20K_1;
                                break;
                            case SPELL_CHARGE_TRIGGER_FACTION_MOUNTS:
                                spellId = SPELL_CHARGE_CHARGING_EFFECT_8K5;
                                break;
                            default:
                                return;
                        }

                        // If target isn't a training dummy there's a chance of failing the charge
                        if (!target->IsCharmedOwnedByPlayerOrPlayer() && roll_chance_f(12.5f))
                            spellId = SPELL_CHARGE_MISS_EFFECT;

                        if (Unit* vehicle = GetCaster()->GetVehicleBase())
                            vehicle->CastSpell(target, spellId, false);
                        else
                            GetCaster()->CastSpell(target, spellId, false);
                        break;
                    }
                    case EFFECT_1: // On damaging spells, for removing a defend layer
                    case EFFECT_2:
                    {
                        Unit::AuraApplicationMap const& auras = target->GetAppliedAuras();
                        for (Unit::AuraApplicationMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                        {
                            if (Aura* aura = itr->second->GetBase())
                            {
                                SpellInfo const* auraInfo = aura->GetSpellInfo();
                                if (auraInfo && auraInfo->SpellIconID == 2007 && aura->HasEffectType(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN))
                                {
                                    aura->ModStackAmount(-1, AuraRemoveFlags::ByEnemySpell);
                                    // Remove dummys from rider (Necessary for updating visual shields)
                                    if (Unit* rider = target->GetCharmer())
                                        if (Aura* defend = rider->GetAura(aura->GetId()))
                                            defend->ModStackAmount(-1, AuraRemoveFlags::ByEnemySpell);
                                    break;
                                }
                            }
                        }
                        break;
                    }
                }
            }

            void HandleChargeEffect(SpellEffIndex /*effIndex*/)
            {
                uint32 spellId;

                switch (GetSpellInfo()->Id)
                {
                    case SPELL_CHARGE_CHARGING_EFFECT_8K5:
                        spellId = SPELL_CHARGE_DAMAGE_8K5;
                        break;
                    case SPELL_CHARGE_CHARGING_EFFECT_20K_1:
                    case SPELL_CHARGE_CHARGING_EFFECT_20K_2:
                        spellId = SPELL_CHARGE_DAMAGE_20K;
                        break;
                    case SPELL_CHARGE_CHARGING_EFFECT_45K_1:
                    case SPELL_CHARGE_CHARGING_EFFECT_45K_2:
                        spellId = SPELL_CHARGE_DAMAGE_45K;
                        break;
                    default:
                        return;
                }

                if (Unit* rider = GetCaster()->GetCharmer())
                    rider->CastSpell(GetHitUnit(), spellId, false);
                else
                    GetCaster()->CastSpell(GetHitUnit(), spellId, false);
            }

            void Register() override
            {
                SpellInfo const* spell = sSpellMgr->AssertSpellInfo(m_scriptSpellId);

                if (spell->HasEffect(SPELL_EFFECT_SCRIPT_EFFECT))
                    OnEffectHitTarget.Register(&spell_gen_mounted_charge_SpellScript::HandleScriptEffect, EFFECT_FIRST_FOUND, SPELL_EFFECT_SCRIPT_EFFECT);

                if (spell->Effects[EFFECT_0].Effect == SPELL_EFFECT_CHARGE)
                    OnEffectHitTarget.Register(&spell_gen_mounted_charge_SpellScript::HandleChargeEffect, EFFECT_0, SPELL_EFFECT_CHARGE);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_mounted_charge_SpellScript();
        }
};

enum MossCoveredFeet
{
    SPELL_FALL_DOWN = 6869
};

// 6870 Moss Covered Feet
// 31399 Moss Covered Feet
class spell_gen_moss_covered_feet : public SpellScriptLoader
{
    public:
        spell_gen_moss_covered_feet() : SpellScriptLoader("spell_gen_moss_covered_feet") { }

        class spell_gen_moss_covered_feet_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_FALL_DOWN });
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();
                eventInfo.GetActionTarget()->CastSpell(nullptr, SPELL_FALL_DOWN, aurEff);
            }

            void Register() override
            {
                OnEffectProc.Register(&spell_gen_moss_covered_feet_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_moss_covered_feet_AuraScript();
        }
};

enum Netherbloom : uint32
{
    SPELL_NETHERBLOOM_POLLEN_1      = 28703
};

// 28702 - Netherbloom
class spell_gen_netherbloom : public SpellScriptLoader
{
    public:
        spell_gen_netherbloom() : SpellScriptLoader("spell_gen_netherbloom") { }

        class spell_gen_netherbloom_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                for (uint8 i = 0; i < 5; ++i)
                    if (!ValidateSpellInfo({ SPELL_NETHERBLOOM_POLLEN_1 + i }))
                        return false;

                return true;
            }

            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);

                if (Unit* target = GetHitUnit())
                {
                    // 25% chance of casting a random buff
                    if (roll_chance_i(75))
                        return;

                    // triggered spells are 28703 to 28707
                    // Note: some sources say, that there was the possibility of
                    //       receiving a debuff. However, this seems to be removed by a patch.

                    // don't overwrite an existing aura
                    for (uint8 i = 0; i < 5; ++i)
                        if (target->HasAura(SPELL_NETHERBLOOM_POLLEN_1 + i))
                            return;

                    target->CastSpell(target, SPELL_NETHERBLOOM_POLLEN_1 + urand(0, 4), true);
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_gen_netherbloom_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_netherbloom_SpellScript();
        }
};

enum NightmareVine
{
    SPELL_NIGHTMARE_POLLEN      = 28721
};

// 28720 - Nightmare Vine
class spell_gen_nightmare_vine : public SpellScriptLoader
{
    public:
        spell_gen_nightmare_vine() : SpellScriptLoader("spell_gen_nightmare_vine") { }

        class spell_gen_nightmare_vine_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_NIGHTMARE_POLLEN });
            }

            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);

                if (Unit* target = GetHitUnit())
                {
                    // 25% chance of casting Nightmare Pollen
                    if (roll_chance_i(25))
                        target->CastSpell(target, SPELL_NIGHTMARE_POLLEN, true);
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_gen_nightmare_vine_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_nightmare_vine_SpellScript();
        }
};

enum ObsidianArmor
{
    SPELL_GEN_OBSIDIAN_ARMOR_HOLY       = 27536,
    SPELL_GEN_OBSIDIAN_ARMOR_FIRE       = 27533,
    SPELL_GEN_OBSIDIAN_ARMOR_NATURE     = 27538,
    SPELL_GEN_OBSIDIAN_ARMOR_FROST      = 27534,
    SPELL_GEN_OBSIDIAN_ARMOR_SHADOW     = 27535,
    SPELL_GEN_OBSIDIAN_ARMOR_ARCANE     = 27540
};

// 27539 - Obsidian Armor
class spell_gen_obsidian_armor : public SpellScriptLoader
{
    public:
        spell_gen_obsidian_armor() : SpellScriptLoader("spell_gen_obsidian_armor") { }

        class spell_gen_obsidian_armor_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_GEN_OBSIDIAN_ARMOR_HOLY,
                    SPELL_GEN_OBSIDIAN_ARMOR_FIRE,
                    SPELL_GEN_OBSIDIAN_ARMOR_NATURE,
                    SPELL_GEN_OBSIDIAN_ARMOR_FROST,
                    SPELL_GEN_OBSIDIAN_ARMOR_SHADOW,
                    SPELL_GEN_OBSIDIAN_ARMOR_ARCANE
                });
            }

            bool CheckProc(ProcEventInfo& eventInfo)
            {
                DamageInfo* damageInfo = eventInfo.GetDamageInfo();
                if (!damageInfo || !damageInfo->GetSpellInfo())
                    return false;

                if (GetFirstSchoolInMask(eventInfo.GetSchoolMask()) == SPELL_SCHOOL_NORMAL)
                    return false;

                return true;
            }

            void OnProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();

                uint32 spellId = 0;
                switch (GetFirstSchoolInMask(eventInfo.GetSchoolMask()))
                {
                    case SPELL_SCHOOL_HOLY:
                        spellId = SPELL_GEN_OBSIDIAN_ARMOR_HOLY;
                        break;
                    case SPELL_SCHOOL_FIRE:
                        spellId = SPELL_GEN_OBSIDIAN_ARMOR_FIRE;
                        break;
                    case SPELL_SCHOOL_NATURE:
                        spellId = SPELL_GEN_OBSIDIAN_ARMOR_NATURE;
                        break;
                    case SPELL_SCHOOL_FROST:
                        spellId = SPELL_GEN_OBSIDIAN_ARMOR_FROST;
                        break;
                    case SPELL_SCHOOL_SHADOW:
                        spellId = SPELL_GEN_OBSIDIAN_ARMOR_SHADOW;
                        break;
                    case SPELL_SCHOOL_ARCANE:
                        spellId = SPELL_GEN_OBSIDIAN_ARMOR_ARCANE;
                        break;
                    default:
                        return;
                }
                GetTarget()->CastSpell(GetTarget(), spellId, aurEff);
            }

            void Register() override
            {
                DoCheckProc.Register(&spell_gen_obsidian_armor_AuraScript::CheckProc);
                OnEffectProc.Register(&spell_gen_obsidian_armor_AuraScript::OnProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_obsidian_armor_AuraScript();
        }
};

class spell_gen_oracle_wolvar_reputation : public SpellScriptLoader
{
    public:
        spell_gen_oracle_wolvar_reputation() : SpellScriptLoader("spell_gen_oracle_wolvar_reputation") { }

        class spell_gen_oracle_wolvar_reputation_SpellScript : public SpellScript
        {
            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            void HandleDummy(SpellEffIndex effIndex)
            {
                Player* player = GetCaster()->ToPlayer();
                uint32 factionId = GetSpellInfo()->Effects[effIndex].CalcValue();
                int32  repChange = GetSpellInfo()->Effects[EFFECT_1].CalcValue();

                FactionEntry const* factionEntry = sFactionStore.LookupEntry(factionId);
                if (!factionEntry)
                    return;

                // Set rep to baserep + basepoints (expecting spillover for oposite faction -> become hated)
                // Not when player already has equal or higher rep with this faction
                if (player->GetReputationMgr().GetReputation(factionEntry) < repChange)
                    player->GetReputationMgr().SetReputation(factionEntry, repChange);

                // EFFECT_INDEX_2 most likely update at war state, we already handle this in SetReputation
            }

            void Register() override
            {
                OnEffectHit.Register(&spell_gen_oracle_wolvar_reputation_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_oracle_wolvar_reputation_SpellScript();
        }
};

enum OrcDisguiseSpells
{
    SPELL_ORC_DISGUISE_TRIGGER       = 45759,
    SPELL_ORC_DISGUISE_MALE          = 45760,
    SPELL_ORC_DISGUISE_FEMALE        = 45762
};

class spell_gen_orc_disguise : public SpellScriptLoader
{
    public:
        spell_gen_orc_disguise() : SpellScriptLoader("spell_gen_orc_disguise") { }

        class spell_gen_orc_disguise_SpellScript : public SpellScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_ORC_DISGUISE_TRIGGER,
                    SPELL_ORC_DISGUISE_MALE,
                    SPELL_ORC_DISGUISE_FEMALE
                });
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (Player* target = GetHitPlayer())
                {
                    uint8 gender = target->getGender();
                    if (!gender)
                        caster->CastSpell(target, SPELL_ORC_DISGUISE_MALE, true);
                    else
                        caster->CastSpell(target, SPELL_ORC_DISGUISE_FEMALE, true);
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_gen_orc_disguise_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_orc_disguise_SpellScript();
        }
};

enum ParalyticPoison
{
    SPELL_PARALYSIS = 35202
};

// 35201 - Paralytic Poison
class spell_gen_paralytic_poison : public SpellScriptLoader
{
    public:
        spell_gen_paralytic_poison() : SpellScriptLoader("spell_gen_paralytic_poison") { }

        class spell_gen_paralytic_poison_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_PARALYSIS });
            }

            void HandleStun(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (!GetTargetApplication()->GetRemoveMode().HasFlag(AuraRemoveFlags::Expired))
                    return;

                GetTarget()->CastSpell(nullptr, SPELL_PARALYSIS, aurEff);
            }

            void Register() override
            {
                AfterEffectRemove.Register(&spell_gen_paralytic_poison_AuraScript::HandleStun, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_paralytic_poison_AuraScript();
        }
};

class spell_gen_prevent_emotes : public AuraScript
{
    void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREVENT_EMOTES_FROM_CHAT_TEXT);
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREVENT_EMOTES_FROM_CHAT_TEXT);
    }

    void Register() override
    {
        OnEffectApply.Register(&spell_gen_prevent_emotes::HandleEffectApply, SpellEffIndex(EFFECT_FIRST_FOUND), SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
        OnEffectRemove.Register(&spell_gen_prevent_emotes::OnRemove, SpellEffIndex(EFFECT_FIRST_FOUND), SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_gen_proc_below_pct_damaged : public AuraScript
{
    bool CheckProc(ProcEventInfo& eventInfo)
    {
        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!damageInfo || !damageInfo->GetDamage())
            return false;

        int32 pct = GetSpellInfo()->Effects[EFFECT_0].CalcValue();

        if (eventInfo.GetActionTarget()->HealthBelowPctDamaged(pct, damageInfo->GetDamage()))
            return true;

        return false;
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_gen_proc_below_pct_damaged::CheckProc);
    }
};

class spell_gen_proc_charge_drop_only : public SpellScriptLoader
{
    public:
        spell_gen_proc_charge_drop_only() : SpellScriptLoader("spell_gen_proc_charge_drop_only") { }

        class spell_gen_proc_charge_drop_only_AuraScript : public AuraScript
        {
            void HandleChargeDrop(ProcEventInfo& /*eventInfo*/)
            {
                PreventDefaultAction();
            }

            void Register() override
            {
                OnProc.Register(&spell_gen_proc_charge_drop_only_AuraScript::HandleChargeDrop);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_proc_charge_drop_only_AuraScript();
        }
};

enum ParachuteSpells
{
    SPELL_PARACHUTE         = 45472,
    SPELL_PARACHUTE_BUFF    = 44795,
};

// 45472 Parachute
class spell_gen_parachute : public SpellScriptLoader
{
    public:
        spell_gen_parachute() : SpellScriptLoader("spell_gen_parachute") { }

        class spell_gen_parachute_AuraScript : public AuraScript
        {
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo(
                {
                    SPELL_PARACHUTE,
                    SPELL_PARACHUTE_BUFF
                });
            }

            void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
            {
                if (Player* target = GetTarget()->ToPlayer())
                    if (target->IsFalling())
                    {
                        target->RemoveAurasDueToSpell(SPELL_PARACHUTE);
                        target->CastSpell(target, SPELL_PARACHUTE_BUFF, true);
                    }
            }

            void Register() override
            {
                OnEffectPeriodic.Register(&spell_gen_parachute_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_parachute_AuraScript();
        }
};

enum PetSummoned
{
    NPC_DOOMGUARD       = 11859,
    NPC_INFERNAL        = 89,
    NPC_IMP             = 416
};

class spell_gen_pet_summoned : public SpellScriptLoader
{
    public:
        spell_gen_pet_summoned() : SpellScriptLoader("spell_gen_pet_summoned") { }

        class spell_gen_pet_summoned_SpellScript : public SpellScript
        {
            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                Player* player = GetCaster()->ToPlayer();
                if (player->GetLastPetNumber())
                {
                    PetType newPetType = (player->getClass() == CLASS_HUNTER) ? HUNTER_PET : SUMMON_PET;
                    Pet* newPet = new Pet(player, newPetType);
                    if (newPet->LoadPetData(player, 0, player->GetLastPetNumber(), true))
                    {
                        // revive the pet if it is dead
                        if (newPet->getDeathState() == DEAD)
                            newPet->setDeathState(ALIVE);

                        newPet->SetFullHealth();
                        newPet->SetPower(newPet->GetPowerType(), newPet->GetMaxPower(newPet->GetPowerType()));

                        switch (newPet->GetEntry())
                        {
                            case NPC_DOOMGUARD:
                            case NPC_INFERNAL:
                                newPet->SetEntry(NPC_IMP);
                                break;
                            default:
                                break;
                        }
                    }
                    else
                        delete newPet;
                }
            }

            void Register() override
            {
                OnEffectHitTarget.Register(&spell_gen_pet_summoned_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_pet_summoned_SpellScript();
        }
};

class spell_gen_profession_research : public SpellScript
{
    bool Load() override
    {
        return GetCaster()->GetTypeId() == TYPEID_PLAYER;
    }

    SpellCastResult CheckRequirement()
    {
        if (HasDiscoveredAllSpells(GetSpellInfo()->Id, GetCaster()->ToPlayer()))
        {
            SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_NOTHING_TO_DISCOVER);
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

        caster->UpdateCraftSkill(GetSpellInfo());
    }

    void Register() override
    {
        OnCheckCast.Register(&spell_gen_profession_research::CheckRequirement);
        OnEffectHitTarget.Register(&spell_gen_profession_research::HandleScript, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

class spell_gen_remove_flight_auras : public SpellScript
{
    void HandleScript(SpellEffIndex /*effIndex*/)
    {
        if (Unit* target = GetHitUnit())
        {
            target->RemoveAurasByType(SPELL_AURA_FLY);
            target->RemoveAurasByType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED);
        }
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_remove_flight_auras::HandleScript, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

enum Replenishment
{
    SPELL_REPLENISHMENT             = 57669,
    SPELL_INFINITE_REPLENISHMENT    = 61782
};

class ReplenishmentCheck
{
public:
    bool operator()(WorldObject* obj) const
    {
        if (Unit* target = obj->ToUnit())
            return target->GetPowerType() != POWER_MANA;

        return true;
    }
};

class spell_gen_replenishment : public SpellScript
{
    void RemoveInvalidTargets(std::list<WorldObject*>& targets)
    {
        // In arenas Replenishment may only affect the caster
        if (Player* caster = GetCaster()->ToPlayer())
        {
            if (caster->InArena())
            {
                targets.clear();
                targets.push_back(caster);
                return;
            }
        }

        targets.remove_if(ReplenishmentCheck());

        uint8 const maxTargets = 10;

        if (targets.size() > maxTargets)
        {
            targets.sort(Trinity::PowerPctOrderPred(POWER_MANA));
            targets.resize(maxTargets);
        }
    }

    void Register() override
    {
        OnObjectAreaTargetSelect.Register(&spell_gen_replenishment::RemoveInvalidTargets, EFFECT_ALL, TARGET_UNIT_CASTER_AREA_RAID);
    }
};

class spell_gen_replenishment_AuraScript : public AuraScript
{
    bool Load() override
    {
        return GetUnitOwner()->GetPowerType() == POWER_MANA;
    }

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        switch (GetSpellInfo()->Id)
        {
            case SPELL_REPLENISHMENT:
            case SPELL_INFINITE_REPLENISHMENT:
                amount = CalculatePct(GetUnitOwner()->GetMaxPower(POWER_MANA), 0.1f);
                break;
            default:
                break;
        }
    }

    void Register() override
    {
        DoEffectCalcAmount.Register(&spell_gen_replenishment_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_PERIODIC_ENERGIZE);
    }
};

enum RunningWildMountIds
{
    RUNNING_WILD_MODEL_MALE     = 29422,
    RUNNING_WILD_MODEL_FEMALE   = 29423,
    SPELL_ALTERED_FORM          = 97709
};

class spell_gen_running_wild_AuraScript : public AuraScript
{
    bool Validate(SpellInfo const* /*spell*/) override
    {
        if (!sCreatureDisplayInfoStore.LookupEntry(RUNNING_WILD_MODEL_MALE))
            return false;
        if (!sCreatureDisplayInfoStore.LookupEntry(RUNNING_WILD_MODEL_FEMALE))
            return false;
        return true;
    }

    void HandleMount(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        PreventDefaultAction();

        target->Mount(target->getGender() == GENDER_FEMALE ? RUNNING_WILD_MODEL_FEMALE : RUNNING_WILD_MODEL_MALE, 0, 0);

        // cast speed aura
        if (MountCapabilityEntry const* mountCapability = sMountCapabilityStore.LookupEntry(aurEff->GetAmount()))
            target->CastSpell(target, mountCapability->ModSpellAuraID, TRIGGERED_FULL_MASK);
    }

    void Register() override
    {
        OnEffectApply.Register(&spell_gen_running_wild_AuraScript::HandleMount, EFFECT_1, SPELL_AURA_MOUNTED, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_gen_running_wild : public SpellScript
{
    bool Validate(SpellInfo const* /*spell*/) override
    {
        return ValidateSpellInfo({ SPELL_ALTERED_FORM });
    }

    bool Load() override
    {
        // Definitely not a good thing, but currently the only way to do something at cast start
        // Should be replaced as soon as possible with a new hook: BeforeCastStart
        GetCaster()->CastSpell(GetCaster(), SPELL_ALTERED_FORM, TRIGGERED_FULL_MASK);
        return false;
    }

    void Register() override
    {
    }
};

class spell_gen_two_forms : public SpellScript
{
    SpellCastResult CheckCast()
    {
        if (GetCaster()->IsInCombat())
        {
            SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_CANT_TRANSFORM);
            return SPELL_FAILED_CUSTOM_ERROR;
        }

        // Player cannot transform to human form if he is forced to be worgen for some reason (Darkflight)
        Unit::AuraEffectList const& alteredFormAuras = GetCaster()->GetAuraEffectsByType(SPELL_AURA_WORGEN_ALTERED_FORM);
        if (std::distance(alteredFormAuras.begin(), alteredFormAuras.end()) > 1)
        {
            SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_CANT_TRANSFORM);
            return SPELL_FAILED_CUSTOM_ERROR;
        }

        return SPELL_CAST_OK;
    }

    void HandleTransform(SpellEffIndex effIndex)
    {
        Unit* target = GetHitUnit();
        PreventHitDefaultEffect(effIndex);
        if (target->HasAuraType(SPELL_AURA_WORGEN_ALTERED_FORM))
            target->RemoveAurasByType(SPELL_AURA_WORGEN_ALTERED_FORM);
        else    // Basepoints 1 for this aura control whether to trigger transform transition animation or not.
            target->CastSpell(target, SPELL_ALTERED_FORM, CastSpellExtraArgs(true).AddSpellBP0(1));
    }

    void Register() override
    {
        OnCheckCast.Register(&spell_gen_two_forms::CheckCast);
        OnEffectHitTarget.Register(&spell_gen_two_forms::HandleTransform, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

class spell_gen_darkflight : public SpellScript
{
    void TriggerTransform()
    {
        GetCaster()->CastSpell(GetCaster(), SPELL_ALTERED_FORM, TRIGGERED_FULL_MASK);
    }

    void Register() override
    {
        AfterCast.Register(&spell_gen_darkflight::TriggerTransform);
    }
};

enum AlteredForm
{
    SPELL_ALTERED_FORM_PROC_AURA = 97681 // Serverside spell
};

class spell_gen_enable_worgen_altered_form : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ALTERED_FORM_PROC_AURA });
    }

    void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->CastSpell(nullptr, SPELL_ALTERED_FORM_PROC_AURA, true);
    }

    void Register() override
    {
        AfterEffectApply.Register(&spell_gen_enable_worgen_altered_form::HandleApply, EFFECT_0, SPELL_AURA_ENABLE_ALTERED_FORM, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
    }
};

enum SeaforiumSpells
{
    SPELL_PLANT_CHARGES_CREDIT_ACHIEVEMENT  = 60937
};

class spell_gen_seaforium_blast : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_PLANT_CHARGES_CREDIT_ACHIEVEMENT });
    }

    bool Load() override
    {
        return GetGObjCaster()->GetOwnerGUID().IsPlayer();
    }

    void AchievementCredit(SpellEffIndex /*effIndex*/)
    {
        if (Unit* owner = GetGObjCaster()->GetOwner())
            if (GameObject* go = GetHitGObj())
                if (go->GetGOInfo()->type == GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING)
                    owner->CastSpell(nullptr, SPELL_PLANT_CHARGES_CREDIT_ACHIEVEMENT, true);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_seaforium_blast::AchievementCredit, EFFECT_1, SPELL_EFFECT_GAMEOBJECT_DAMAGE);
    }
};

enum SpectatorCheerTrigger
{
    EMOTE_ONE_SHOT_CHEER        = 4,
    EMOTE_ONE_SHOT_EXCLAMATION  = 5,
    EMOTE_ONE_SHOT_APPLAUD      = 21
};

uint8 const EmoteArray[3] = { EMOTE_ONE_SHOT_CHEER, EMOTE_ONE_SHOT_EXCLAMATION, EMOTE_ONE_SHOT_APPLAUD };

class spell_gen_spectator_cheer_trigger : public SpellScript
{
    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        GetCaster()->HandleEmoteCommand(EmoteArray[urand(0, 2)]);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_spectator_cheer_trigger::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

class spell_gen_spirit_healer_res : public SpellScript
{
    bool Load() override
    {
        return GetOriginalCaster() && GetOriginalCaster()->GetTypeId() == TYPEID_PLAYER;
    }

    void HandleDummy(SpellEffIndex /* effIndex */)
    {
        Player* originalCaster = GetOriginalCaster()->ToPlayer();
        if (Unit* target = GetHitUnit())
        {
            WorldPacket data(SMSG_SPIRIT_HEALER_CONFIRM, 8);
            data << uint64(target->GetGUID());
            originalCaster->SendDirectMessage(&data);
        }
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_spirit_healer_res::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

enum SummonElemental
{
    SPELL_SUMMON_FIRE_ELEMENTAL  = 8985,
    SPELL_SUMMON_EARTH_ELEMENTAL = 19704
};

class spell_gen_summon_elemental : public AuraScript
{
public:
    spell_gen_summon_elemental(uint32 spellId) : AuraScript(), _spellId(spellId) { }

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ _spellId });
    }

    void AfterApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (GetCaster())
            if (Unit* owner = GetCaster()->GetOwner())
                owner->CastSpell(owner, _spellId, true);
    }

    void AfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (GetCaster())
            if (Unit* owner = GetCaster()->GetOwner())
                if (owner->GetTypeId() == TYPEID_PLAYER) /// @todo this check is maybe wrong
                    owner->ToPlayer()->RemovePet(nullptr, PET_SAVE_DISMISS, true);
    }

    void Register() override
    {
         AfterEffectApply.Register(&spell_gen_summon_elemental::AfterApply, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
         AfterEffectRemove.Register(&spell_gen_summon_elemental::AfterRemove, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }

private:
    uint32 _spellId;
};

enum TournamentMountsSpells
{
    SPELL_LANCE_EQUIPPED     = 62853
};

class spell_gen_summon_tournament_mount : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_LANCE_EQUIPPED });
    }

    SpellCastResult CheckIfLanceEquiped()
    {
        if (GetCaster()->IsInDisallowedMountForm())
            GetCaster()->RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT);

        if (!GetCaster()->HasAura(SPELL_LANCE_EQUIPPED))
        {
            SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_MUST_HAVE_LANCE_EQUIPPED);
            return SPELL_FAILED_CUSTOM_ERROR;
        }

        return SPELL_CAST_OK;
    }

    void Register() override
    {
        OnCheckCast.Register(&spell_gen_summon_tournament_mount::CheckIfLanceEquiped);
    }
};

// 41213, 43416, 69222, 73076 - Throw Shield
class spell_gen_throw_shield : public SpellScript
{
    void HandleScriptEffect(SpellEffIndex effIndex)
    {
        PreventHitDefaultEffect(effIndex);
        GetCaster()->CastSpell(GetHitUnit(), uint32(GetEffectValue()), true);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_throw_shield::HandleScriptEffect, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

enum MountedDuelSpells
{
    SPELL_ON_TOURNAMENT_MOUNT = 63034,
    SPELL_MOUNTED_DUEL        = 62875
};

class spell_gen_tournament_duel : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
        {
            SPELL_ON_TOURNAMENT_MOUNT,
            SPELL_MOUNTED_DUEL
        });
    }

    void HandleScriptEffect(SpellEffIndex /*effIndex*/)
    {
        if (Unit* rider = GetCaster()->GetCharmer())
        {
            if (Player* playerTarget = GetHitPlayer())
            {
                if (playerTarget->HasAura(SPELL_ON_TOURNAMENT_MOUNT) && playerTarget->GetVehicleBase())
                    rider->CastSpell(playerTarget, SPELL_MOUNTED_DUEL, true);
            }
            else if (Unit* unitTarget = GetHitUnit())
            {
                if (unitTarget->GetCharmer() && unitTarget->GetCharmer()->GetTypeId() == TYPEID_PLAYER && unitTarget->GetCharmer()->HasAura(SPELL_ON_TOURNAMENT_MOUNT))
                    rider->CastSpell(unitTarget->GetCharmer(), SPELL_MOUNTED_DUEL, true);
            }
        }
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_tournament_duel::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

class spell_gen_tournament_pennant : public AuraScript
{
    bool Load() override
    {
        return GetCaster() && GetCaster()->GetTypeId() == TYPEID_PLAYER;
    }

    void HandleApplyEffect(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Unit* caster = GetCaster())
            if (!caster->GetVehicleBase())
                caster->RemoveAurasDueToSpell(GetId());
    }

    void Register() override
    {
        OnEffectApply.Register(&spell_gen_tournament_pennant::HandleApplyEffect, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
    }
};

enum FriendOrFowl
{
    SPELL_TURKEY_VENGEANCE      = 25285
};

class spell_gen_turkey_marker : public AuraScript
{
    void OnApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        // store stack apply times, so we can pop them while they expire
        _applyTimes.push_back(GameTime::GetGameTimeMS());
        Unit* target = GetTarget();

        // on stack 15 cast the achievement crediting spell
        if (GetStackAmount() >= 15)
            target->CastSpell(target, SPELL_TURKEY_VENGEANCE, CastSpellExtraArgs(aurEff).SetOriginalCaster(GetCasterGUID()));
    }

    void OnPeriodic(AuraEffect const* /*aurEff*/)
    {
        if (_applyTimes.empty())
            return;

        // pop stack if it expired for us
        if (_applyTimes.front() + GetMaxDuration() < GameTime::GetGameTimeMS())
            ModStackAmount(-1, AuraRemoveFlags::Expired);
    }

    void Register() override
    {
        AfterEffectApply.Register(&spell_gen_turkey_marker::OnApply, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
        OnEffectPeriodic.Register(&spell_gen_turkey_marker::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }

    std::list<uint32> _applyTimes;
};

enum FoamSword
{
    ITEM_FOAM_SWORD_GREEN   = 45061,
    ITEM_FOAM_SWORD_PINK    = 45176,
    ITEM_FOAM_SWORD_BLUE    = 45177,
    ITEM_FOAM_SWORD_RED     = 45178,
    ITEM_FOAM_SWORD_YELLOW  = 45179
};

class spell_gen_upper_deck_create_foam_sword : public SpellScript
{
    void HandleScript(SpellEffIndex effIndex)
    {
        if (Player* player = GetHitPlayer())
        {
            static uint32 const itemId[5] = { ITEM_FOAM_SWORD_GREEN, ITEM_FOAM_SWORD_PINK, ITEM_FOAM_SWORD_BLUE, ITEM_FOAM_SWORD_RED, ITEM_FOAM_SWORD_YELLOW };
            // player can only have one of these items
            for (uint8 i = 0; i < 5; ++i)
            {
                if (player->HasItemCount(itemId[i], 1, true))
                    return;
            }

            CreateItem(effIndex, itemId[urand(0, 4)]);
        }
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_upper_deck_create_foam_sword::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

enum VampiricTouch
{
    SPELL_VAMPIRIC_TOUCH_HEAL   = 52724
};

// 52723 - Vampiric Touch
// 60501 - Vampiric Touch
class spell_gen_vampiric_touch : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_VAMPIRIC_TOUCH_HEAL });
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!damageInfo || !damageInfo->GetDamage())
            return;

        Unit* caster = eventInfo.GetActor();
        int32 bp = damageInfo->GetDamage() / 2;
        caster->CastSpell(caster, SPELL_VAMPIRIC_TOUCH_HEAL, CastSpellExtraArgs(aurEff).AddSpellBP0(bp));
    }

    void Register() override
    {
        OnEffectProc.Register(&spell_gen_vampiric_touch::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_gen_vehicle_scaling_trigger : public SpellScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo({ uint32(spellInfo->Effects[EFFECT_0].BasePoints) });
    }

    void HandleEffect(SpellEffIndex effIndex)
    {
        if (Unit* caster = GetCaster())
            GetHitUnit()->CastSpell(caster, GetSpellInfo()->Effects[effIndex].BasePoints, true);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_vehicle_scaling_trigger::HandleEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

enum VehicleScaling
{
    SPELL_GEAR_SCALING_1        = 66668,
    SPELL_GEAR_SCALING_CATA_N   = 91401,
    SPELL_GEAR_SCALING_CATA_HC  = 91405

};

class spell_gen_vehicle_scaling : public AuraScript
{
    bool Load() override
    {
        return GetCaster() && GetCaster()->GetTypeId() == TYPEID_PLAYER;
    }

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        Unit* caster = GetCaster();
        float factor = 0.0f;
        uint16 baseItemLevel = 0;

        /// @todo Reserach coeffs for different vehicles
        switch (GetId())
        {
            case SPELL_GEAR_SCALING_1:
                factor = 1.0f;
                baseItemLevel = 205;
                break;
            case SPELL_GEAR_SCALING_CATA_N:
                factor = 1.0f;
                baseItemLevel = 305;
                break;
            case SPELL_GEAR_SCALING_CATA_HC:
                factor = 1.0f;
                baseItemLevel = 329;
                break;
            default:
                factor = 1.0f;
                baseItemLevel = 170;
                break;
        }

        float avgILvl = caster->ToPlayer()->GetAverageItemLevel();
        if (avgILvl < baseItemLevel)
            return;                     /// @todo Research possibility of scaling down

        amount = uint16((avgILvl - baseItemLevel) * factor);
    }

    void Register() override
    {
        switch (m_scriptSpellId)
        {
            case SPELL_GEAR_SCALING_CATA_N:
            case SPELL_GEAR_SCALING_CATA_HC:
                DoEffectCalcAmount.Register(&spell_gen_vehicle_scaling::CalculateAmount, EFFECT_0, SPELL_AURA_MOD_HEALING_DONE_PERCENT);
                break;
            default:
                DoEffectCalcAmount.Register(&spell_gen_vehicle_scaling::CalculateAmount, EFFECT_0, SPELL_AURA_MOD_HEALING_PCT);
                break;
        }

        DoEffectCalcAmount.Register(&spell_gen_vehicle_scaling::CalculateAmount, EFFECT_1, SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
        DoEffectCalcAmount.Register(&spell_gen_vehicle_scaling::CalculateAmount, EFFECT_2, SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT);
    }
};

enum VendorBarkTrigger
{
    NPC_AMPHITHEATER_VENDOR     = 30098,
    SAY_AMPHITHEATER_VENDOR     = 0
};

class spell_gen_vendor_bark_trigger : public SpellScript
{
    void HandleDummy(SpellEffIndex /* effIndex */)
    {
        if (Creature* vendor = GetCaster()->ToCreature())
            if (vendor->GetEntry() == NPC_AMPHITHEATER_VENDOR)
                vendor->AI()->Talk(SAY_AMPHITHEATER_VENDOR);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_vendor_bark_trigger::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

class spell_gen_wg_water : public SpellScript
{
    SpellCastResult CheckCast()
    {
        if (!GetSpellInfo()->CheckTargetCreatureType(GetCaster()))
            return SPELL_FAILED_DONT_REPORT;
        return SPELL_CAST_OK;
    }

    void Register() override
    {
        OnCheckCast.Register(&spell_gen_wg_water::CheckCast);
    }
};

enum WhisperGulchYoggSaronWhisper
{
    SPELL_YOGG_SARON_WHISPER_DUMMY  = 29072
};

class spell_gen_whisper_gulch_yogg_saron_whisper : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_YOGG_SARON_WHISPER_DUMMY });
    }

    void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
    {
        PreventDefaultAction();
        GetTarget()->CastSpell((Unit*)nullptr, SPELL_YOGG_SARON_WHISPER_DUMMY, true);
    }

    void Register() override
    {
        OnEffectPeriodic.Register(&spell_gen_whisper_gulch_yogg_saron_whisper::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

class spell_gen_eject_all_passengers : public SpellScript
{
    void RemoveVehicleAuras()
    {
        if (Vehicle* vehicle = GetHitUnit()->GetVehicleKit())
            vehicle->RemoveAllPassengers();
    }

    void Register() override
    {
        AfterHit.Register(&spell_gen_eject_all_passengers::RemoveVehicleAuras);
    }
};

enum EjectPassenger
{
    SPELL_GEN_EJECT_PASSENGER_1 = 77946,
    SPELL_GEN_EJECT_PASSENGER_3 = 95204
};

class spell_gen_eject_passenger : public SpellScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        if (spellInfo->Effects[EFFECT_0].CalcValue() < 1)
        {
            switch (spellInfo->Id)
            {
                case SPELL_GEN_EJECT_PASSENGER_1:
                case SPELL_GEN_EJECT_PASSENGER_3:
                    return ValidateSpellInfo({ spellInfo->Id });
                default:
                    return false;
            }
            return false;
        }
        return true;
    }

    void EjectPassenger(SpellEffIndex /*effIndex*/)
    {
        if (Vehicle* vehicle = GetHitUnit()->GetVehicleKit())
        {
            uint8 seatId = 0;
            switch (GetSpellInfo()->Id)
            {
                case SPELL_GEN_EJECT_PASSENGER_1:
                    seatId = 0;
                    break;
                case SPELL_GEN_EJECT_PASSENGER_3:
                    seatId = 2;
                    break;
                default:
                    seatId = GetEffectValue() - 1;
                    break;
            }

            if (Unit* passenger = vehicle->GetPassenger(seatId))
                passenger->ExitVehicle();
        }
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_eject_passenger::EjectPassenger, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

enum GMFreeze
{
    SPELL_GM_FREEZE = 9454
};

class spell_gen_gm_freeze : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_GM_FREEZE });
    }

    void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        // Do what was done before to the target in HandleFreezeCommand
        if (Player* player = GetTarget()->ToPlayer())
        {
            // stop combat + make player unattackable + duel stop + stop some spells
            player->SetFaction(FACTION_FRIENDLY);
            player->CombatStop();
            if (player->IsNonMeleeSpellCast(true))
                player->InterruptNonMeleeSpells(true);
            player->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

            // if player class = hunter || warlock remove pet if alive
            if ((player->getClass() == CLASS_HUNTER) || (player->getClass() == CLASS_WARLOCK))
            {
                if (Pet* pet = player->GetPet())
                {
                    pet->SavePetToDB(PET_SAVE_CURRENT_STATE);
                    // not let dismiss dead pet
                    if (pet->IsAlive())
                        player->RemovePet(pet, PET_SAVE_DISMISS);
                }
            }
        }
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        // Do what was done before to the target in HandleUnfreezeCommand
        if (Player* player = GetTarget()->ToPlayer())
        {
            // Reset player faction + allow combat + allow duels
            player->SetFactionForRace(player->getRace());
            player->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            // save player
            player->SaveToDB();
        }
    }

    void Register() override
    {
        OnEffectApply.Register(&spell_gen_gm_freeze::OnApply, EFFECT_0, SPELL_AURA_MOD_STUN, AURA_EFFECT_HANDLE_REAL);
        OnEffectRemove.Register(&spell_gen_gm_freeze::OnRemove, EFFECT_0, SPELL_AURA_MOD_STUN, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_gen_stand : public SpellScript
{
    void HandleScript(SpellEffIndex /*eff*/)
    {
        Creature* target = GetHitCreature();
        if (!target)
            return;

        target->SetStandState(UNIT_STAND_STATE_STAND);
        target->HandleEmoteCommand(EMOTE_STATE_NONE);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_stand::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

enum RequiredMixologySpells
{
    SPELL_MIXOLOGY                      = 53042,
    // Flasks
    SPELL_FLASK_OF_THE_FROST_WYRM       = 53755,
    SPELL_FLASK_OF_STONEBLOOD           = 53758,
    SPELL_FLASK_OF_ENDLESS_RAGE         = 53760,
    SPELL_FLASK_OF_PURE_MOJO            = 54212,
    SPELL_LESSER_FLASK_OF_RESISTANCE    = 62380,
    SPELL_LESSER_FLASK_OF_TOUGHNESS     = 53752,
    SPELL_FLASK_OF_BLINDING_LIGHT       = 28521,
    SPELL_FLASK_OF_CHROMATIC_WONDER     = 42735,
    SPELL_FLASK_OF_FORTIFICATION        = 28518,
    SPELL_FLASK_OF_MIGHTY_RESTORATION   = 28519,
    SPELL_FLASK_OF_PURE_DEATH           = 28540,
    SPELL_FLASK_OF_RELENTLESS_ASSAULT   = 28520,
    SPELL_FLASK_OF_CHROMATIC_RESISTANCE = 17629,
    SPELL_FLASK_OF_DISTILLED_WISDOM     = 17627,
    SPELL_FLASK_OF_SUPREME_POWER        = 17628,
    SPELL_FLASK_OF_THE_TITANS           = 17626,
    // Elixirs
    SPELL_ELIXIR_OF_MIGHTY_AGILITY      = 28497,
    SPELL_ELIXIR_OF_ACCURACY            = 60340,
    SPELL_ELIXIR_OF_DEADLY_STRIKES      = 60341,
    SPELL_ELIXIR_OF_MIGHTY_DEFENSE      = 60343,
    SPELL_ELIXIR_OF_EXPERTISE           = 60344,
    SPELL_ELIXIR_OF_ARMOR_PIERCING      = 60345,
    SPELL_ELIXIR_OF_LIGHTNING_SPEED     = 60346,
    SPELL_ELIXIR_OF_MIGHTY_FORTITUDE    = 53751,
    SPELL_ELIXIR_OF_MIGHTY_MAGEBLOOD    = 53764,
    SPELL_ELIXIR_OF_MIGHTY_STRENGTH     = 53748,
    SPELL_ELIXIR_OF_MIGHTY_TOUGHTS      = 60347,
    SPELL_ELIXIR_OF_PROTECTION          = 53763,
    SPELL_ELIXIR_OF_SPIRIT              = 53747,
    SPELL_GURUS_ELIXIR                  = 53749,
    SPELL_SHADOWPOWER_ELIXIR            = 33721,
    SPELL_WRATH_ELIXIR                  = 53746,
    SPELL_ELIXIR_OF_EMPOWERMENT         = 28514,
    SPELL_ELIXIR_OF_MAJOR_MAGEBLOOD     = 28509,
    SPELL_ELIXIR_OF_MAJOR_SHADOW_POWER  = 28503,
    SPELL_ELIXIR_OF_MAJOR_DEFENSE       = 28502,
    SPELL_FEL_STRENGTH_ELIXIR           = 38954,
    SPELL_ELIXIR_OF_IRONSKIN            = 39628,
    SPELL_ELIXIR_OF_MAJOR_AGILITY       = 54494,
    SPELL_ELIXIR_OF_DRAENIC_WISDOM      = 39627,
    SPELL_ELIXIR_OF_MAJOR_FIREPOWER     = 28501,
    SPELL_ELIXIR_OF_MAJOR_FROST_POWER   = 28493,
    SPELL_EARTHEN_ELIXIR                = 39626,
    SPELL_ELIXIR_OF_MASTERY             = 33726,
    SPELL_ELIXIR_OF_HEALING_POWER       = 28491,
    SPELL_ELIXIR_OF_MAJOR_FORTITUDE     = 39625,
    SPELL_ELIXIR_OF_MAJOR_STRENGTH      = 28490,
    SPELL_ADEPTS_ELIXIR                 = 54452,
    SPELL_ONSLAUGHT_ELIXIR              = 33720,
    SPELL_MIGHTY_TROLLS_BLOOD_ELIXIR    = 24361,
    SPELL_GREATER_ARCANE_ELIXIR         = 17539,
    SPELL_ELIXIR_OF_THE_MONGOOSE        = 17538,
    SPELL_ELIXIR_OF_BRUTE_FORCE         = 17537,
    SPELL_ELIXIR_OF_SAGES               = 17535,
    SPELL_ELIXIR_OF_SUPERIOR_DEFENSE    = 11348,
    SPELL_ELIXIR_OF_DEMONSLAYING        = 11406,
    SPELL_ELIXIR_OF_GREATER_FIREPOWER   = 26276,
    SPELL_ELIXIR_OF_SHADOW_POWER        = 11474,
    SPELL_MAGEBLOOD_ELIXIR              = 24363,
    SPELL_ELIXIR_OF_GIANTS              = 11405,
    SPELL_ELIXIR_OF_GREATER_AGILITY     = 11334,
    SPELL_ARCANE_ELIXIR                 = 11390,
    SPELL_ELIXIR_OF_GREATER_INTELLECT   = 11396,
    SPELL_ELIXIR_OF_GREATER_DEFENSE     = 11349,
    SPELL_ELIXIR_OF_FROST_POWER         = 21920,
    SPELL_ELIXIR_OF_AGILITY             = 11328,
    SPELL_MAJOR_TROLLS_BLLOOD_ELIXIR    =  3223,
    SPELL_ELIXIR_OF_FORTITUDE           =  3593,
    SPELL_ELIXIR_OF_OGRES_STRENGTH      =  3164,
    SPELL_ELIXIR_OF_FIREPOWER           =  7844,
    SPELL_ELIXIR_OF_LESSER_AGILITY      =  3160,
    SPELL_ELIXIR_OF_DEFENSE             =  3220,
    SPELL_STRONG_TROLLS_BLOOD_ELIXIR    =  3222,
    SPELL_ELIXIR_OF_MINOR_ACCURACY      = 63729,
    SPELL_ELIXIR_OF_WISDOM              =  3166,
    SPELL_ELIXIR_OF_GIANTH_GROWTH       =  8212,
    SPELL_ELIXIR_OF_MINOR_AGILITY       =  2374,
    SPELL_ELIXIR_OF_MINOR_FORTITUDE     =  2378,
    SPELL_WEAK_TROLLS_BLOOD_ELIXIR      =  3219,
    SPELL_ELIXIR_OF_LIONS_STRENGTH      =  2367,
    SPELL_ELIXIR_OF_MINOR_DEFENSE       =   673
};

class spell_gen_mixology_bonus : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MIXOLOGY });
    }

    bool Load() override
    {
        return GetCaster() && GetCaster()->GetTypeId() == TYPEID_PLAYER;
    }

    void SetBonusValueForEffect(SpellEffIndex effIndex, int32 value, AuraEffect const* aurEff)
    {
        if (aurEff->GetEffIndex() == uint32(effIndex))
            bonus = value;
    }

    void CalculateAmount(AuraEffect const* aurEff, int32& amount, bool& /*canBeRecalculated*/)
        {
            if (GetCaster()->HasAura(SPELL_MIXOLOGY) && GetCaster()->HasSpell(GetSpellInfo()->Effects[EFFECT_0].TriggerSpell))
            {
                switch (GetId())
                {
                    case SPELL_WEAK_TROLLS_BLOOD_ELIXIR:
                    case SPELL_MAGEBLOOD_ELIXIR:
                        bonus = amount;
                        break;
                    case SPELL_ELIXIR_OF_FROST_POWER:
                    case SPELL_LESSER_FLASK_OF_TOUGHNESS:
                    case SPELL_LESSER_FLASK_OF_RESISTANCE:
                        bonus = CalculatePct(amount, 80);
                        break;
                    case SPELL_ELIXIR_OF_MINOR_DEFENSE:
                    case SPELL_ELIXIR_OF_LIONS_STRENGTH:
                    case SPELL_ELIXIR_OF_MINOR_AGILITY:
                    case SPELL_MAJOR_TROLLS_BLLOOD_ELIXIR:
                    case SPELL_ELIXIR_OF_SHADOW_POWER:
                    case SPELL_ELIXIR_OF_BRUTE_FORCE:
                    case SPELL_MIGHTY_TROLLS_BLOOD_ELIXIR:
                    case SPELL_ELIXIR_OF_GREATER_FIREPOWER:
                    case SPELL_ONSLAUGHT_ELIXIR:
                    case SPELL_EARTHEN_ELIXIR:
                    case SPELL_ELIXIR_OF_MAJOR_AGILITY:
                    case SPELL_FLASK_OF_THE_TITANS:
                    case SPELL_FLASK_OF_RELENTLESS_ASSAULT:
                    case SPELL_FLASK_OF_STONEBLOOD:
                    case SPELL_ELIXIR_OF_MINOR_ACCURACY:
                        bonus = CalculatePct(amount, 50);
                        break;
                    case SPELL_ELIXIR_OF_PROTECTION:
                        bonus = 280;
                        break;
                    case SPELL_ELIXIR_OF_MAJOR_DEFENSE:
                        bonus = 200;
                        break;
                    case SPELL_ELIXIR_OF_GREATER_DEFENSE:
                    case SPELL_ELIXIR_OF_SUPERIOR_DEFENSE:
                        bonus = 140;
                        break;
                    case SPELL_ELIXIR_OF_FORTITUDE:
                        bonus = 100;
                        break;
                    case SPELL_FLASK_OF_ENDLESS_RAGE:
                        bonus = 82;
                        break;
                    case SPELL_ELIXIR_OF_DEFENSE:
                        bonus = 70;
                        break;
                    case SPELL_ELIXIR_OF_DEMONSLAYING:
                        bonus = 50;
                        break;
                    case SPELL_FLASK_OF_THE_FROST_WYRM:
                        bonus = 47;
                        break;
                    case SPELL_WRATH_ELIXIR:
                        bonus = 32;
                        break;
                    case SPELL_ELIXIR_OF_MAJOR_FROST_POWER:
                    case SPELL_ELIXIR_OF_MAJOR_FIREPOWER:
                    case SPELL_ELIXIR_OF_MAJOR_SHADOW_POWER:
                        bonus = 29;
                        break;
                    case SPELL_ELIXIR_OF_MIGHTY_TOUGHTS:
                        bonus = 27;
                        break;
                    case SPELL_FLASK_OF_SUPREME_POWER:
                    case SPELL_FLASK_OF_BLINDING_LIGHT:
                    case SPELL_FLASK_OF_PURE_DEATH:
                    case SPELL_SHADOWPOWER_ELIXIR:
                        bonus = 23;
                        break;
                    case SPELL_ELIXIR_OF_MIGHTY_AGILITY:
                    case SPELL_FLASK_OF_DISTILLED_WISDOM:
                    case SPELL_ELIXIR_OF_SPIRIT:
                    case SPELL_ELIXIR_OF_MIGHTY_STRENGTH:
                    case SPELL_FLASK_OF_PURE_MOJO:
                    case SPELL_ELIXIR_OF_ACCURACY:
                    case SPELL_ELIXIR_OF_DEADLY_STRIKES:
                    case SPELL_ELIXIR_OF_MIGHTY_DEFENSE:
                    case SPELL_ELIXIR_OF_EXPERTISE:
                    case SPELL_ELIXIR_OF_ARMOR_PIERCING:
                    case SPELL_ELIXIR_OF_LIGHTNING_SPEED:
                        bonus = 20;
                        break;
                    case SPELL_FLASK_OF_CHROMATIC_RESISTANCE:
                        bonus = 17;
                        break;
                    case SPELL_ELIXIR_OF_MINOR_FORTITUDE:
                    case SPELL_ELIXIR_OF_MAJOR_STRENGTH:
                        bonus = 15;
                        break;
                    case SPELL_FLASK_OF_MIGHTY_RESTORATION:
                        bonus = 13;
                        break;
                    case SPELL_ARCANE_ELIXIR:
                        bonus = 12;
                        break;
                    case SPELL_ELIXIR_OF_GREATER_AGILITY:
                    case SPELL_ELIXIR_OF_GIANTS:
                        bonus = 11;
                        break;
                    case SPELL_ELIXIR_OF_AGILITY:
                    case SPELL_ELIXIR_OF_GREATER_INTELLECT:
                    case SPELL_ELIXIR_OF_SAGES:
                    case SPELL_ELIXIR_OF_IRONSKIN:
                    case SPELL_ELIXIR_OF_MIGHTY_MAGEBLOOD:
                        bonus = 10;
                        break;
                    case SPELL_ELIXIR_OF_HEALING_POWER:
                        bonus = 9;
                        break;
                    case SPELL_ELIXIR_OF_DRAENIC_WISDOM:
                    case SPELL_GURUS_ELIXIR:
                        bonus = 8;
                        break;
                    case SPELL_ELIXIR_OF_FIREPOWER:
                    case SPELL_ELIXIR_OF_MAJOR_MAGEBLOOD:
                    case SPELL_ELIXIR_OF_MASTERY:
                        bonus = 6;
                        break;
                    case SPELL_ELIXIR_OF_LESSER_AGILITY:
                    case SPELL_ELIXIR_OF_OGRES_STRENGTH:
                    case SPELL_ELIXIR_OF_WISDOM:
                    case SPELL_ELIXIR_OF_THE_MONGOOSE:
                        bonus = 5;
                        break;
                    case SPELL_STRONG_TROLLS_BLOOD_ELIXIR:
                    case SPELL_FLASK_OF_CHROMATIC_WONDER:
                        bonus = 4;
                        break;
                    case SPELL_ELIXIR_OF_EMPOWERMENT:
                        bonus = -10;
                        break;
                    case SPELL_ADEPTS_ELIXIR:
                        SetBonusValueForEffect(EFFECT_0, 13, aurEff);
                        SetBonusValueForEffect(EFFECT_1, 13, aurEff);
                        SetBonusValueForEffect(EFFECT_2, 8, aurEff);
                        break;
                    case SPELL_ELIXIR_OF_MIGHTY_FORTITUDE:
                        SetBonusValueForEffect(EFFECT_0, 160, aurEff);
                        break;
                    case SPELL_ELIXIR_OF_MAJOR_FORTITUDE:
                        SetBonusValueForEffect(EFFECT_0, 116, aurEff);
                        SetBonusValueForEffect(EFFECT_1, 6, aurEff);
                        break;
                    case SPELL_FEL_STRENGTH_ELIXIR:
                        SetBonusValueForEffect(EFFECT_0, 40, aurEff);
                        SetBonusValueForEffect(EFFECT_1, 40, aurEff);
                        break;
                    case SPELL_FLASK_OF_FORTIFICATION:
                        SetBonusValueForEffect(EFFECT_0, 210, aurEff);
                        SetBonusValueForEffect(EFFECT_1, 5, aurEff);
                        break;
                    case SPELL_GREATER_ARCANE_ELIXIR:
                        SetBonusValueForEffect(EFFECT_0, 19, aurEff);
                        SetBonusValueForEffect(EFFECT_1, 19, aurEff);
                        SetBonusValueForEffect(EFFECT_2, 5, aurEff);
                        break;
                    case SPELL_ELIXIR_OF_GIANTH_GROWTH:
                        SetBonusValueForEffect(EFFECT_0, 5, aurEff);
                        break;
                    default:
                        TC_LOG_ERROR("spells", "SpellId %u couldn't be processed in spell_gen_mixology_bonus", GetId());
                        break;
                }
                amount += bonus;
            }
        }

    int32 bonus = 0;

    void Register() override
    {
        DoEffectCalcAmount.Register(&spell_gen_mixology_bonus::CalculateAmount, EFFECT_ALL, SPELL_AURA_ANY);
    }
};

enum LandmineKnockbackAchievement
{
    SPELL_LANDMINE_KNOCKBACK_ACHIEVEMENT = 57064
};

class spell_gen_landmine_knockback_achievement : public SpellScript
{
    void HandleScript(SpellEffIndex /*effIndex*/)
    {
        if (Player* target = GetHitPlayer())
        {
            Aura const* aura = GetHitAura();
            if (!aura || aura->GetStackAmount() < 10)
                return;

            target->CastSpell(target, SPELL_LANDMINE_KNOCKBACK_ACHIEVEMENT, true);
        }
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_landmine_knockback_achievement::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

// 34098 - ClearAllDebuffs
class spell_gen_clear_debuffs : public SpellScript
{
    void HandleScript(SpellEffIndex /*effIndex*/)
    {
        if (Unit* target = GetHitUnit())
        {
            target->RemoveOwnedAuras([](Aura const* aura)
            {
                SpellInfo const* spellInfo = aura->GetSpellInfo();
                return !spellInfo->IsPositive() && !spellInfo->IsPassive();
            });
        }
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_clear_debuffs::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

enum PonySpells
{
    ACHIEV_PONY_UP              = 3736,
    MOUNT_PONY                  = 29736
};

class spell_gen_pony_mount_check : public AuraScript
{
    void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;
        Player* owner = caster->GetOwner()->ToPlayer();
        if (!owner || !owner->HasAchieved(ACHIEV_PONY_UP))
            return;

        if (owner->IsMounted())
        {
            caster->Mount(MOUNT_PONY);
            caster->SetSpeedRate(MOVE_RUN, owner->GetSpeedRate(MOVE_RUN));
        }
        else if (caster->IsMounted())
        {
            caster->Dismount();
            caster->SetSpeedRate(MOVE_RUN, owner->GetSpeedRate(MOVE_RUN));
        }
    }

    void Register() override
    {
        OnEffectPeriodic.Register(&spell_gen_pony_mount_check::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

class spell_gen_shroud_of_death : public AuraScript
{
    void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        PreventDefaultAction();
        GetUnitOwner()->m_serverSideVisibility.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_GHOST);
        GetUnitOwner()->m_serverSideVisibilityDetect.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_GHOST);
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        PreventDefaultAction();
        GetUnitOwner()->m_serverSideVisibility.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_ALIVE);
        GetUnitOwner()->m_serverSideVisibilityDetect.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_ALIVE);
    }

    void Register() override
    {
        OnEffectApply.Register(&spell_gen_shroud_of_death::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        OnEffectRemove.Register(&spell_gen_shroud_of_death::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

enum ArmorSpecializationSpells
{
    // Warrior
    SPELL_ARMOR_SPEC_WAR_ARMS           = 86110,
    SPELL_ARMOR_SPEC_WAR_FURY           = 86101,
    SPELL_ARMOR_SPEC_WAR_PROTECTION     = 86535,

    // Paladin
    SPELL_ARMOR_SPEC_PAL_HOLY           = 86103,
    SPELL_ARMOR_SPEC_PAL_PROTECTOION    = 86102,
    SPELL_ARMOR_SPEC_PAL_RETRIBUTION    = 86539,

    // Hunter
    SPELL_ARMOR_SPEC_HUN                = 86538,

    // Rogue
    SPELL_ARMOR_SPEC_ROG                = 86092,

    // Death Knight
    SPELL_ARMOR_SPEC_DK_BLOOD           = 86537,
    SPELL_ARMOR_SPEC_DK_FROST           = 86536,
    SPELL_ARMOR_SPEC_DK_UNHOLY          = 86113,

    // Shaman
    SPELL_ARMOR_SPEC_SHA_ELEMENTAL      = 86100,
    SPELL_ARMOR_SPEC_SHA_ENHANCEMENT    = 86099,
    SPELL_ARMOR_SPEC_SHA_RESTORATION    = 86108,

    // Druid
    SPELL_ARMOR_SPEC_DRU_BALANCE        = 86093,
    SPELL_ARMOR_SPEC_DRU_FREAL_BEAR     = 86096,
    SPELL_ARMOR_SPEC_DRU_FERAL          = 86097,
    SPELL_ARMOR_SPEC_DRU_RESTORATION    = 86104
};

class spell_gen_armor_specialization : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_ARMOR_SPEC_WAR_ARMS,
                SPELL_ARMOR_SPEC_WAR_FURY,
                SPELL_ARMOR_SPEC_WAR_PROTECTION,
                SPELL_ARMOR_SPEC_PAL_HOLY,
                SPELL_ARMOR_SPEC_PAL_PROTECTOION,
                SPELL_ARMOR_SPEC_PAL_RETRIBUTION,
                SPELL_ARMOR_SPEC_HUN,
                SPELL_ARMOR_SPEC_ROG,
                SPELL_ARMOR_SPEC_DK_BLOOD,
                SPELL_ARMOR_SPEC_DK_FROST,
                SPELL_ARMOR_SPEC_DK_UNHOLY,
                SPELL_ARMOR_SPEC_SHA_ELEMENTAL,
                SPELL_ARMOR_SPEC_SHA_ENHANCEMENT,
                SPELL_ARMOR_SPEC_SHA_RESTORATION,
                SPELL_ARMOR_SPEC_DRU_BALANCE,
                SPELL_ARMOR_SPEC_DRU_FREAL_BEAR,
                SPELL_ARMOR_SPEC_DRU_FERAL,
                SPELL_ARMOR_SPEC_DRU_RESTORATION
            });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Player* player = GetHitPlayer();
        if (!player)
            return;

        uint32 spellId = 0;

        switch (player->GetPrimaryTalentTree(player->GetActiveSpec()))
        {
            case TALENT_TREE_WARRIOR_ARMS:
                spellId = SPELL_ARMOR_SPEC_WAR_ARMS;
                break;
            case TALENT_TREE_WARRIOR_FURY:
                spellId = SPELL_ARMOR_SPEC_WAR_FURY;
                break;
            case TALENT_TREE_WARRIOR_PROTECTION:
                spellId = SPELL_ARMOR_SPEC_WAR_PROTECTION;
                break;
            case TALENT_TREE_PALADIN_HOLY:
                spellId = SPELL_ARMOR_SPEC_PAL_HOLY;
                break;
            case TALENT_TREE_PALADIN_PROTECTION:
                spellId = SPELL_ARMOR_SPEC_PAL_PROTECTOION;
                break;
            case TALENT_TREE_PALADIN_RETRIBUTION:
                spellId = SPELL_ARMOR_SPEC_PAL_RETRIBUTION;
                break;
            case TALENT_TREE_HUNTER_BEAST_MASTERY:
            case TALENT_TREE_HUNTER_MARKSMANSHIP:
            case TALENT_TREE_HUNTER_SURVIVAL:
                spellId = SPELL_ARMOR_SPEC_HUN;
                break;
            case TALENT_TREE_ROGUE_ASSASSINATION:
            case TALENT_TREE_ROGUE_COMBAT:
            case TALENT_TREE_ROGUE_SUBTLETY:
                spellId = SPELL_ARMOR_SPEC_ROG;
                break;
            case TALENT_TREE_DEATH_KNIGHT_BLOOD:
                spellId = SPELL_ARMOR_SPEC_DK_BLOOD;
                break;
            case TALENT_TREE_DEATH_KNIGHT_FROST:
                spellId = SPELL_ARMOR_SPEC_DK_FROST;
                break;
            case TALENT_TREE_DEATH_KNIGHT_UNHOLY:
                spellId = SPELL_ARMOR_SPEC_DK_UNHOLY;
                break;
            case TALENT_TREE_SHAMAN_ELEMENTAL:
                spellId = SPELL_ARMOR_SPEC_SHA_ELEMENTAL;
                break;
            case TALENT_TREE_SHAMAN_ENHANCEMENT:
                spellId = SPELL_ARMOR_SPEC_SHA_ENHANCEMENT;
                break;
            case TALENT_TREE_SHAMAN_RESTORATION:
                spellId = SPELL_ARMOR_SPEC_SHA_RESTORATION;
                break;
            case TALENT_TREE_DRUID_BALANCE:
                spellId = SPELL_ARMOR_SPEC_DRU_BALANCE;
                break;
            case TALENT_TREE_DRUID_FERAL_COMBAT:
            {
                if (player->GetShapeshiftForm() == FORM_BEAR)
                    spellId = SPELL_ARMOR_SPEC_DRU_FREAL_BEAR;
                else
                    spellId = SPELL_ARMOR_SPEC_DRU_FERAL;
                break;
            }
            case TALENT_TREE_DRUID_RESTORATION:
                spellId = SPELL_ARMOR_SPEC_DRU_RESTORATION;
                break;
            default:
                return;
        }

        if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId))
            if (player->HasAllItemsToFitToSpellRequirements(spellInfo))
                player->CastSpell(player, spellId, true);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_armor_specialization::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

enum PvPTrinket
{
    SPELL_PVP_TRINKET_ALLIANCE  = 97403,
    SPELL_PVP_TRINKET_HORDE     = 97404,
    SPELL_PVP_TRINKET_NEUTRAL   = 97979,
    SPELL_EVERY_MAN_FOR_HIMSELF = 59752
};

class spell_gen_pvp_trinket : public SpellScript
{
    void HandlePvPTrinketVisual()
    {
        if (Unit* caster = GetCaster())
        {
            if (caster->GetTypeId() == TYPEID_PLAYER && GetSpellInfo()->Id != SPELL_EVERY_MAN_FOR_HIMSELF)
                caster->CastSpell(caster, caster->ToPlayer()->GetTeam() == ALLIANCE ? SPELL_PVP_TRINKET_ALLIANCE : SPELL_PVP_TRINKET_HORDE, true);
            else
                caster->CastSpell(caster, SPELL_PVP_TRINKET_NEUTRAL, true);
        }
    }

    void Register() override
    {
        AfterHit.Register(&spell_gen_pvp_trinket::HandlePvPTrinketVisual);
    }
};

enum Blink
{
    SPELL_BLINK_TARGET = 28401
};

class spell_gen_blink : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_BLINK_TARGET });
    }

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        if (targets.empty())
            return;

        Trinity::Containers::RandomResize(targets, 1);
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster || !caster->IsCreature())
            return;

        Creature* creature = caster->ToCreature();
        if (Unit* target = creature->GetThreatManager().GetCurrentVictim())
        {
            creature->GetThreatManager().ResetThreat(target);
            if (creature->IsAIEnabled())
            {
                creature->CastSpell(target, SPELL_BLINK_TARGET, true);
                creature->AI()->AttackStart(target);
            }
        }
    }

    void Register() override
    {
        OnObjectAreaTargetSelect.Register(&spell_gen_blink::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        OnEffectHitTarget.Register(&spell_gen_blink::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

class spell_gen_toxic_blow_dart : public SpellScript
{
    void FilterTargets(std::list<WorldObject*>& targets)
    {
        if (targets.empty())
            return;

        Trinity::Containers::RandomResize(targets, 1);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect.Register(&spell_gen_toxic_blow_dart::FilterTargets, EFFECT_ALL, TARGET_UNIT_SRC_AREA_ENEMY);
    }
};

enum ProjectileGoods
{
    SPELL_PROJECTILE_GOODS_1    = 84136,
    SPELL_PROJECTILE_GOODS_2    = 84138,
    SPELL_PROJECTILE_GOODS_3    = 84141,
    SPELL_PROJECTILE_GOODS_4    = 84142
};

class spell_gen_projectile_goods : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_PROJECTILE_GOODS_1,
                SPELL_PROJECTILE_GOODS_2,
                SPELL_PROJECTILE_GOODS_3,
                SPELL_PROJECTILE_GOODS_4,
            });
    }

    void HandleScriptEffect(SpellEffIndex /*effIndex*/)
    {
        uint32 spellId = 0;
        switch (RAND(0, 3))
        {
            case 0:
                spellId = SPELL_PROJECTILE_GOODS_1;
                break;
            case 1:
                spellId = SPELL_PROJECTILE_GOODS_2;
                break;
            case 2:
                spellId = SPELL_PROJECTILE_GOODS_3;
                break;
            case 3:
                spellId = SPELL_PROJECTILE_GOODS_4;
                break;
        }
        GetCaster()->CastSpell(GetHitUnit(), spellId, true);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_projectile_goods::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

enum Vengeance
{
    SPELL_VENGEANCE_TRIGGERED = 76691
};

// 93098 - 93099 - 84839 - 84840 - Vengeance
class spell_gen_vengeance : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_VENGEANCE_TRIGGERED });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        return eventInfo.GetDamageInfo() && eventInfo.GetDamageInfo()->GetAttacker()
            && eventInfo.GetDamageInfo()->GetAttacker()->GetTypeId() != TYPEID_PLAYER;
    }

    void HandleEffectProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        Unit* caster = GetTarget();

        if (!caster->GetAura(SPELL_VENGEANCE_TRIGGERED, caster->GetGUID()))
        {
            uint32 healthCap = CalculatePct(caster->GetCreateHealth(), 10) + caster->GetStat(STAT_STAMINA);
            uint32 damageBonus = CalculatePct(eventInfo.GetDamageInfo()->GetDamage(), 33);
            int32 bp = std::min<int32>(damageBonus, healthCap);
            caster->CastSpell(caster, SPELL_VENGEANCE_TRIGGERED, CastSpellExtraArgs(true).AddSpellBP0(bp).AddSpellMod(SPELLVALUE_BASE_POINT1, bp).AddSpellMod(SPELLVALUE_BASE_POINT2, bp));
        }
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_gen_vengeance::CheckProc);
        OnEffectProc.Register(&spell_gen_vengeance::HandleEffectProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

struct DamageTimeInfo
{
    uint32 TimeStamp;
    uint32 Damage;
};

typedef std::vector<DamageTimeInfo> DamageInfoContainer;

// 76691 - Vengeance
class spell_gen_vengeance_triggered : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_VENGEANCE_TRIGGERED });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        return eventInfo.GetDamageInfo() && eventInfo.GetDamageInfo()->GetAttacker()
            && eventInfo.GetDamageInfo()->GetAttacker()->GetTypeId() != TYPEID_PLAYER;
    }

    void HandleEffectProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        if (uint32 damage = eventInfo.GetDamageInfo()->GetDamage())
        {
            _damageInfo.push_back({ GameTime::GetGameTimeMS(), damage });
        }
    }

    void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
    {
        Unit* target = GetTarget();
        // Get the total damage of the last two seconds and clean older damage data
        int32 damageLastTwoSeconds = 0;
        for (DamageInfoContainer::const_iterator itr = _damageInfo.begin(); itr != _damageInfo.end();)
        {
            if ((GameTime::GetGameTimeMS() - (*itr).TimeStamp) > 2 * IN_MILLISECONDS)
                itr = _damageInfo.erase(itr);
            else
            {
                damageLastTwoSeconds += (*itr).Damage;
                itr++;
            }
        }

        uint32 healthCap = CalculatePct(target->GetCreateHealth(), 10) + target->GetStat(STAT_STAMINA);
        damageLastTwoSeconds = std::min<int32>(healthCap, CalculatePct(damageLastTwoSeconds, 33));
        if (damageLastTwoSeconds)
            target->CastSpell(target, SPELL_VENGEANCE_TRIGGERED, CastSpellExtraArgs(true).AddSpellBP0(damageLastTwoSeconds).AddSpellMod(SPELLVALUE_BASE_POINT1, damageLastTwoSeconds).AddSpellMod(SPELLVALUE_BASE_POINT2, damageLastTwoSeconds));
        else
            Remove();
    }

    void Register() override
    {
        DoCheckProc.Register(&spell_gen_vengeance_triggered::CheckProc);
        OnEffectProc.Register(&spell_gen_vengeance_triggered::HandleEffectProc, EFFECT_0, SPELL_AURA_MOD_ATTACK_POWER);
        OnEffectPeriodic.Register(&spell_gen_vengeance_triggered::HandleEffectPeriodic, EFFECT_2, SPELL_AURA_PERIODIC_DUMMY);
    }
private:
    DamageInfoContainer _damageInfo;
};

enum GilneasPrison
{
    SPELL_SUMMON_RAVENOUS_WORGEN_1 = 66836,
    SPELL_SUMMON_RAVENOUS_WORGEN_2 = 66925,

    NPC_WORGEN_RUNT                = 35456,
};

Position const WorgenRuntHousePos[] =
{
    // House Roof
    { -1729.345f, 1526.495f, 55.47962f, 6.188943f },
    { -1709.63f, 1527.464f, 56.86086f, 3.258752f },
    { -1717.75f, 1513.727f, 55.47941f, 4.704845f },
    { -1724.719f, 1526.731f, 55.66177f, 6.138319f },
    { -1713.974f, 1526.625f, 56.21981f, 3.306195f },
    { -1718.104f, 1524.071f, 55.81641f, 4.709816f },
    { -1718.262f, 1518.557f, 55.55954f, 4.726997f },
    // Cathdral Roof
    { -1618.054f, 1489.644f, 68.45153f, 3.593639f },
    { -1625.62f, 1487.033f, 71.27762f, 3.531424f },
    { -1638.569f, 1489.736f, 68.55273f, 4.548815f },
    { -1630.399f, 1481.66f, 71.41516f, 3.484555f },
    { -1622.424f, 1483.882f, 67.67381f, 3.404875f },
    { -1634.344f, 1491.3f, 70.10101f, 4.6248f },
    { -1631.979f, 1491.585f, 71.11481f, 4.032866f },
    { -1627.273f, 1499.689f, 68.89395f, 4.251452f },
    { -1622.665f, 1489.818f, 71.03797f, 3.776179f },
};

class spell_gen_gilneas_prison_periodic_dummy : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_SUMMON_RAVENOUS_WORGEN_1, // House roof
                SPELL_SUMMON_RAVENOUS_WORGEN_2, // Cathedral roof
            });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        if (Unit* caster = GetCaster())
        {
            switch (RAND(0, 1))
            {
                case 0:
                    caster->CastSpell(caster, SPELL_SUMMON_RAVENOUS_WORGEN_1, true);
                    for (uint8 i = 0; i < 7; i++)
                        if (Creature* runt = caster->SummonCreature(NPC_WORGEN_RUNT, WorgenRuntHousePos[i]))
                            runt->AI()->DoAction(i);
                    break;
                case 1:
                    caster->CastSpell(caster, SPELL_SUMMON_RAVENOUS_WORGEN_2, true);
                    for (uint8 i = 7; i < 16; i++)
                        if (Creature* runt = caster->SummonCreature(NPC_WORGEN_RUNT, WorgenRuntHousePos[i]))
                            runt->AI()->DoAction(i);
                    if (RAND(0, 1) == 1)
                        for (uint8 i = 0; i < RAND(1, 3); i++)
                            if (Creature* runt = caster->SummonCreature(NPC_WORGEN_RUNT, WorgenRuntHousePos[i]))
                                runt->AI()->DoAction(i);
                    break;
            }
        }
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_gilneas_prison_periodic_dummy::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

enum ThrowTorch
{
    CREDIT_ROUND_UP_WORGEN  = 35582,
    SPELL_THROW_TORCH       = 67063
};

class spell_gen_throw_torch : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo( { SPELL_THROW_TORCH });
    }

    void HandleEffect()
    {
        if (Player* player = GetCaster()->ToPlayer())
            if (GetHitUnit() && !GetHitUnit()->HasAura(SPELL_THROW_TORCH))
                player->KilledMonsterCredit(CREDIT_ROUND_UP_WORGEN);
    }

    void Register() override
    {
        BeforeHit.Register(&spell_gen_throw_torch::HandleEffect);
    }
};

class spell_gen_revserse_cast_mirror_image : public SpellScript
{
    void HandleScript(SpellEffIndex effIndex)
    {
        if (Unit* caster = GetCaster())
            GetHitUnit()->CastSpell(caster, GetSpellInfo()->Effects[effIndex].BasePoints, true);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_revserse_cast_mirror_image::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

class spell_gen_mirror_image_aura : public SpellScript
{
    void HandleScript(SpellEffIndex effIndex)
    {
        if (Unit* caster = GetCaster())
            GetHitUnit()->CastSpell(caster, GetSpellInfo()->Effects[effIndex].BasePoints, true);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_mirror_image_aura::HandleScript, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
        OnEffectHitTarget.Register(&spell_gen_mirror_image_aura::HandleScript, EFFECT_2, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};


class spell_gen_reverse_cast_ride_vehicle : public SpellScript
{
    void HandleScript(SpellEffIndex effIndex)
    {
        if (Unit* caster = GetCaster())
            GetHitUnit()->CastSpell(caster, GetSpellInfo()->Effects[effIndex].BasePoints, true);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_reverse_cast_ride_vehicle::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

class GroupMemberCheck
{
public:
    GroupMemberCheck(Player* player) : _player(player) { }

    bool operator()(WorldObject* object)
    {
        if (Player* player = object->ToPlayer())
            return !player->IsInSameGroupWith(_player);

        return false;
    }
private:
    Player* _player;
};

class spell_gen_launch_quest : public SpellScript
{
    bool Load() override
    {
        return GetCaster()->GetTypeId() == TYPEID_UNIT;
    }

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        if (targets.size() > 1)
            if (Player* player = GetCaster()->ToCreature()->GetLootRecipient())
                targets.remove_if(GroupMemberCheck(player));
    }

    void Register() override
    {
        OnObjectAreaTargetSelect.Register(&spell_gen_launch_quest::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
    }
};

// Used for some spells cast by vehicles or charmed creatures that do not send a cooldown event on their own
class spell_gen_charmed_unit_spell_cooldown : public SpellScript
{
    void HandleCast()
    {
        Unit* caster = GetCaster();
        if (Player* owner = caster->GetCharmerOrOwnerPlayerOrPlayerItself())
        {
            WorldPacket data;
            caster->GetSpellHistory()->BuildCooldownPacket(data, SPELL_COOLDOWN_FLAG_NONE, GetSpellInfo()->Id, GetSpellInfo()->RecoveryTime);
            owner->SendDirectMessage(&data);
        }
    }

    void Register() override
    {
        OnCast.Register(&spell_gen_charmed_unit_spell_cooldown::HandleCast);
    }
};

class spell_gen_flurry_of_claws : public AuraScript
{
    void HandlePeriodic(AuraEffect const* /*aurEff*/)
    {
        PreventDefaultAction();
        Unit* target = GetTarget();
        if (Unit* victim = target->GetVictim())
            target->CastSpell(victim, GetSpellInfo()->Effects[EFFECT_0].TriggerSpell, true);
    }

    void Register() override
    {
        OnEffectPeriodic.Register(&spell_gen_flurry_of_claws::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

enum Sunflower
{
    SPELL_SINGING_SUNFLOWER_DND = 93972
};

// 93971 - Sunflower (DND)
class spell_gen_sunflower_dnd : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_SINGING_SUNFLOWER_DND });
    }

    void HandlePeriodic(AuraEffect const* /*aurEff*/)
    {
        GetTarget()->CastSpell(GetTarget(), SPELL_SINGING_SUNFLOWER_DND);
    }

    void Register() override
    {
        OnEffectPeriodic.Register(&spell_gen_sunflower_dnd::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

enum GuildBattleStandard
{
    // Spells
    SPELL_GUILD_BATTLE_STANDARD_ALLIANCE    = 90216,
    SPELL_GUILD_BATTLE_STANDARD_HORDE       = 90708,

    // Creatures
    NPC_GUILD_BATTLE_STANDARD_ALLIANCE_1    = 48115,
    NPC_GUILD_BATTLE_STANDARD_ALLIANCE_2    = 48633,
    NPC_GUILD_BATTLE_STANDARD_ALLIANCE_3    = 48634,
    NPC_GUILD_BATTLE_STANDARD_HORDE_1       = 48636,
    NPC_GUILD_BATTLE_STANDARD_HORDE_2       = 48637,
    NPC_GUILD_BATTLE_STANDARD_HORDE_3       = 48638
};

// 89481 - Guild Battle Standard
class spell_gen_guild_battle_standard : public AuraScript
{
    bool Load() override
    {
        return GetCaster()->IsCreature();
    }

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_GUILD_BATTLE_STANDARD_ALLIANCE,
                SPELL_GUILD_BATTLE_STANDARD_HORDE
            });
    }

    void HandlePeriodic(AuraEffect const* aurEff)
    {
        int32 bp = 0;
        uint32 spellId = 0;

        Unit* target = GetTarget();
        switch (target->GetEntry())
        {
            case NPC_GUILD_BATTLE_STANDARD_ALLIANCE_1:
                bp = 5;
                spellId = SPELL_GUILD_BATTLE_STANDARD_ALLIANCE;
                break;
            case NPC_GUILD_BATTLE_STANDARD_ALLIANCE_2:
                bp = 10;
                spellId = SPELL_GUILD_BATTLE_STANDARD_ALLIANCE;
                break;
            case NPC_GUILD_BATTLE_STANDARD_ALLIANCE_3:
                bp = 15;
                spellId = SPELL_GUILD_BATTLE_STANDARD_ALLIANCE;
                break;
            case NPC_GUILD_BATTLE_STANDARD_HORDE_1:
                bp = 5;
                spellId = SPELL_GUILD_BATTLE_STANDARD_HORDE;
                break;
            case NPC_GUILD_BATTLE_STANDARD_HORDE_2:
                bp = 10;
                spellId = SPELL_GUILD_BATTLE_STANDARD_HORDE;
                break;
            case NPC_GUILD_BATTLE_STANDARD_HORDE_3:
                bp = 15;
                spellId = SPELL_GUILD_BATTLE_STANDARD_HORDE;
                break;
            default:
                break;
        }

        if (spellId)
            target->CastSpell(target, spellId, CastSpellExtraArgs(aurEff).AddSpellBP0(bp).AddSpellMod(SPELLVALUE_BASE_POINT1, bp).AddSpellMod(SPELLVALUE_BASE_POINT2, bp));
    }

    void Register() override
    {
        OnEffectPeriodic.Register(&spell_gen_guild_battle_standard::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

// 90216 - Guild Battle Standard
// 90708 - Guild Battle Standard
class spell_gen_guild_battle_standard_buff : public SpellScript
{
    bool Load() override
    {
        return GetCaster()->IsSummon();
    }

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        ObjectGuid guildGuid = GetCaster()->GetGuidValue(OBJECT_FIELD_DATA);
        targets.remove_if([guildGuid](WorldObject* target)->bool
        {
            return !target->IsPlayer() || target->ToPlayer()->GetGuidValue(OBJECT_FIELD_DATA) != guildGuid;
        });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect.Register(&spell_gen_guild_battle_standard_buff::FilterTargets, EFFECT_ALL, TARGET_UNIT_SRC_AREA_ALLY);
    }
};

enum MobileBanking
{
    SPELL_GUILD_CHEST_HORDE     = 88306,
    SPELL_GUILD_CHEST_ALLIANCE  = 88304
};

// 83958 - Mobile Banking
class spell_gen_mobile_banking : public SpellScript
{
    bool Load() override
    {
        return GetCaster()->IsPlayer();
    }

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_GUILD_CHEST_HORDE,
                SPELL_GUILD_CHEST_ALLIANCE
            });
    }

    SpellCastResult CheckRequirement()
    {
        // The player must have a guild reputation rank of friendly or higher to use the mobile banking ability
        if (GetCaster()->ToPlayer()->GetReputationRank(FACTION_GUILD) < REP_FRIENDLY)
            return SPELL_FAILED_REPUTATION;

        return SPELL_CAST_OK;
    }

    void HandleScriptEffect(SpellEffIndex /*effIndex*/)
    {
        Unit* target = GetHitUnit();
        target->CastSpell(target, target->ToPlayer()->GetTeamId() == TEAM_HORDE ? SPELL_GUILD_CHEST_HORDE : SPELL_GUILD_CHEST_ALLIANCE);
    }

    void Register() override
    {
        OnCheckCast.Register(&spell_gen_mobile_banking::CheckRequirement);
        OnEffectHitTarget.Register(&spell_gen_mobile_banking::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

// 92649 - Cauldron of Battle
// 92712 - Big Cauldron of Battle
class spell_gen_cauldron_of_battle : public SpellScript
{
    bool Load() override
    {
        return GetCaster()->IsPlayer();
    }

    void HandleDummyEffect(SpellEffIndex effIndex)
    {
        Player* target = GetHitPlayer();
        if (!target)
            return;

        bool handleEffect = false;

        // EFFECT_0 = Alliance Cauldron, EFFECT_1 = Horde Cauldron
        if ((effIndex == EFFECT_0 && target->GetTeamId() == TEAM_ALLIANCE) ||
            (effIndex == EFFECT_1 && target->GetTeamId() == TEAM_HORDE))
            handleEffect = true;

        if (handleEffect)
        {
            Position dest = target->GetPosition();
            uint32 spellId = GetEffectValue();

            if (SpellInfo const* spell = sSpellMgr->GetSpellInfo(spellId))
            {
                float radius = spell->Effects[EFFECT_0].CalcRadius(target) - target->GetCombatReach();
                target->GetNearPoint(target, dest.m_positionX, dest.m_positionY, dest.m_positionZ, radius, target->GetOrientation());
                target->CastSpell({ dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ() }, spellId);
            }
        }
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_cauldron_of_battle::HandleDummyEffect, EFFECT_0, SPELL_EFFECT_DUMMY);
        OnEffectHitTarget.Register(&spell_gen_cauldron_of_battle::HandleDummyEffect, EFFECT_1, SPELL_EFFECT_DUMMY);
    }
};

enum FlaskOfBattle
{
    // According to WoWHead comments the Flask of Flowing Waters effect is not being used for healers
    SPELL_FLASK_OF_STEELSKIN        = 79469,
    SPELL_FLASK_OF_TITANIC_STRENGTH = 79472,
    SPELL_FLASK_OF_THE_WINDS        = 79471,
    SPELL_FLASK_OF_DRACONIC_MIND    = 79470,
    SPELL_CHUG_A_LUG_R1             = 83945,
    SPELL_CHUG_A_LUG_R2             = 83961
};

// 92679 - Flask of Battle
class spell_gen_flask_of_battle : public SpellScript
{
    bool Load() override
    {
        return GetCaster()->IsPlayer();
    }

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_FLASK_OF_STEELSKIN,
                SPELL_FLASK_OF_TITANIC_STRENGTH,
                SPELL_FLASK_OF_THE_WINDS,
                SPELL_FLASK_OF_DRACONIC_MIND,
                SPELL_CHUG_A_LUG_R1,
                SPELL_CHUG_A_LUG_R2
            });
    }

    void HandleBuffEffect(SpellEffIndex /*effIndex*/)
    {
        Player* player = GetHitPlayer();
        if (!player)
            return;

        uint32 spellId = 0;

        uint32 primaryTalentTree = player->GetPrimaryTalentTree(player->GetActiveSpec());
        switch (player->getClass())
        {
            case CLASS_WARLOCK:
            case CLASS_MAGE:
            case CLASS_PRIEST:
                spellId = SPELL_FLASK_OF_DRACONIC_MIND;
                break;
            case CLASS_ROGUE:
            case CLASS_HUNTER:
                spellId = SPELL_FLASK_OF_THE_WINDS;
                break;
            case CLASS_DRUID:
                if (primaryTalentTree == TALENT_TREE_DRUID_FERAL_COMBAT)
                {
                    if (player->GetShapeshiftForm() == FORM_BEAR)
                        spellId = SPELL_FLASK_OF_STEELSKIN;
                    else
                        spellId = SPELL_FLASK_OF_THE_WINDS;
                }
                else
                    spellId = SPELL_FLASK_OF_DRACONIC_MIND;
                break;
            case CLASS_SHAMAN:
                spellId = primaryTalentTree == TALENT_TREE_SHAMAN_ENHANCEMENT ? SPELL_FLASK_OF_THE_WINDS : SPELL_FLASK_OF_DRACONIC_MIND;
                break;
            case CLASS_WARRIOR:
                spellId = primaryTalentTree == TALENT_TREE_WARRIOR_PROTECTION ? SPELL_FLASK_OF_STEELSKIN : SPELL_FLASK_OF_TITANIC_STRENGTH;
                break;
            case CLASS_DEATH_KNIGHT:
                spellId = primaryTalentTree == TALENT_TREE_DEATH_KNIGHT_BLOOD ? SPELL_FLASK_OF_STEELSKIN : SPELL_FLASK_OF_TITANIC_STRENGTH;
                break;
            case CLASS_PALADIN:
                if (primaryTalentTree == TALENT_TREE_PALADIN_HOLY)
                    spellId = SPELL_FLASK_OF_DRACONIC_MIND;
                else if (primaryTalentTree == TALENT_TREE_PALADIN_PROTECTION)
                    spellId = SPELL_FLASK_OF_STEELSKIN;
                else
                    spellId = SPELL_FLASK_OF_TITANIC_STRENGTH;
                break;
            default:
                break;
        }

        if (spellId)
        {
            uint32 chugALugSpellId = 0;
            if (player->HasSpell(SPELL_CHUG_A_LUG_R2))
                chugALugSpellId = SPELL_CHUG_A_LUG_R2;
            else if (player->HasSpell(SPELL_CHUG_A_LUG_R1))
                chugALugSpellId = SPELL_CHUG_A_LUG_R1;

            if (chugALugSpellId)
            {
                int32 durationBonus = sSpellMgr->AssertSpellInfo(chugALugSpellId)->Effects[EFFECT_0].CalcValue();
                int32 duration = sSpellMgr->AssertSpellInfo(spellId)->GetMaxDuration();
                AddPct(duration, durationBonus);
                player->CastSpell(player, spellId, { SPELLVALUE_DURATION, duration });
            }
            else
                player->CastSpell(player, spellId);
        }
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_flask_of_battle::HandleBuffEffect, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

enum TheQuickAndTheDead
{
    SPELL_THE_QUICK_AND_THE_DEAD_PERK = 83950,
    SPELL_THE_QUICK_AND_THE_DEAD_BUFF = 84559
};

// 8326 - Ghost
class spell_gen_ghost : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_THE_QUICK_AND_THE_DEAD_PERK,
                SPELL_THE_QUICK_AND_THE_DEAD_BUFF
            });
    }

    void AfterApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        if (target->HasAura(SPELL_THE_QUICK_AND_THE_DEAD_PERK))
            target->CastSpell(target, SPELL_THE_QUICK_AND_THE_DEAD_BUFF);
    }

    void AfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        if (target->HasAura(SPELL_THE_QUICK_AND_THE_DEAD_BUFF))
            target->RemoveAurasDueToSpell(SPELL_THE_QUICK_AND_THE_DEAD_BUFF);
    }

    void Register() override
    {
        AfterEffectApply.Register(&spell_gen_ghost::AfterApply, EFFECT_0, SPELL_AURA_GHOST, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove.Register(&spell_gen_ghost::AfterRemove, EFFECT_0, SPELL_AURA_GHOST, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_gen_zero_energy_zero_regen : public AuraScript
{
    void AfterApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->SetPower(POWER_ENERGY, 0);
    }

    void Register() override
    {
        AfterEffectApply.Register(&spell_gen_zero_energy_zero_regen::AfterApply, EFFECT_0, SPELL_AURA_MOD_POWER_REGEN_PERCENT, AURA_EFFECT_HANDLE_REAL);
    }
};

enum LaunchQuestAura
{
    SPELL_LAUNCH_QUEST_PERSONAL_SUMMONS_HORDE       = 93079,
    SPELL_LAUNCH_QUEST_PERSONAL_SUMMONS_ALLIANCE    = 93217,
};

// 93081 - Launch Quest Aura
// 93216 - Launch Quest Aura
class spell_gen_launch_quest_aura : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                SPELL_LAUNCH_QUEST_PERSONAL_SUMMONS_HORDE,
                SPELL_LAUNCH_QUEST_PERSONAL_SUMMONS_ALLIANCE
            });
    }

    // According to sniffs the quests are being launched when the aura expires, not when it gets applied.
    void AfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Player* player = GetTarget()->ToPlayer();
        if (!player)
            return;

        uint32 spellId = player->GetTeamId() == TEAM_ALLIANCE ? SPELL_LAUNCH_QUEST_PERSONAL_SUMMONS_ALLIANCE : SPELL_LAUNCH_QUEST_PERSONAL_SUMMONS_HORDE;
        GetTarget()->CastSpell(GetTarget(), spellId);
    }

    void Register() override
    {
        AfterEffectRemove.Register(&spell_gen_launch_quest_aura::AfterRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 46577 - Wounded
class spell_gen_wounded : public SpellScript
{
    void HandleScriptEffect(SpellEffIndex /*effIndex*/)
    {
        Unit* target = GetHitUnit();
        if (target->GetHealthPct() > 55.f)
            target->SetHealth(CalculatePct(target->GetMaxHealth(), frand(15.f, 55.f)));
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_wounded::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

// 69041 - Rocket Barrage (Racial)
class spell_gen_rocket_barrage : public SpellScript
{
    void HandleDamage(SpellEffIndex effIndex)
    {
        Unit* caster = GetCaster();
        int32 basePoints = GetEffectValue() + caster->getLevel() * 2;
        basePoints += caster->SpellBaseDamageBonusDone(GetSpellInfo()->GetSchoolMask()) * GetSpellInfo()->Effects[effIndex].BonusMultiplier;
        basePoints += caster->GetTotalAttackPowerValue(caster->getClass() != CLASS_HUNTER ? BASE_ATTACK : RANGED_ATTACK) * GetSpellInfo()->BonusCoefficient;
        SetEffectValue(basePoints);
    }

    void Register() override
    {
        OnEffectLaunchTarget.Register(&spell_gen_rocket_barrage::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

enum AuraProcRemoveSpells
{
    SPELL_FACE_RAGE = 99947
};

class spell_gen_face_rage : public AuraScript
{
    bool Validate(SpellInfo const* /*spell*/) override
    {
        return ValidateSpellInfo({ SPELL_FACE_RAGE });
    }

    void OnRemove(AuraEffect const* /*effect*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->RemoveAurasDueToSpell(GetSpellInfo()->Effects[EFFECT_2].TriggerSpell);
    }

    void Register() override
    {
        OnEffectRemove.Register(&spell_gen_face_rage::OnRemove, EFFECT_0, SPELL_AURA_MOD_STUN, AURA_EFFECT_HANDLE_REAL);
    }
};

enum Shadowmeld
{
    SPELL_RACIAL_ELUSIVENESS = 21009
};

// 58984 Shadowmeld (Racial)
class spell_gen_shadowmeld : public AuraScript
{
    bool Validate(SpellInfo const* /*spell*/) override
    {
        return ValidateSpellInfo({ SPELL_RACIAL_ELUSIVENESS });
    }

    void HandleStealthLevel(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        if (AuraEffect const* aurEff = GetUnitOwner()->GetAuraEffect(SPELL_RACIAL_ELUSIVENESS, EFFECT_0))
            amount += aurEff->GetAmount();
    }

    void Register() override
    {
        DoEffectCalcAmount.Register(&spell_gen_shadowmeld::HandleStealthLevel, EFFECT_2, SPELL_AURA_MOD_STEALTH);
    }
};

enum SiegeTankControl
{
    SPELL_SIEGE_TANK_CONTROL = 47963
};

class spell_gen_vehicle_control_link : public AuraScript
{
    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->RemoveAurasDueToSpell(SPELL_SIEGE_TANK_CONTROL); //aurEff->GetAmount()
    }

    void Register() override
    {
        AfterEffectRemove.Register(&spell_gen_vehicle_control_link::OnRemove, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

enum PolymorphCastVisual
{
    // Spells
    SPELL_MAGE_SQUIRREL_FORM    = 32813,
    SPELL_MAGE_GIRAFFE_FORM     = 32816,
    SPELL_MAGE_SERPENT_FORM     = 32817,
    SPELL_MAGE_DRAGONHAWK_FORM  = 32818,
    SPELL_MAGE_WORGEN_FORM      = 32819,
    SPELL_MAGE_SHEEP_FORM       = 32820,

    NPC_AUROSALIA               = 18744

};

/// @todo move out of here and rename - not a mage spell
// 32826 - Polymorph (Visual)
class spell_gen_polymorph_cast_visual : public SpellScript
{
    static const uint32 PolymorhForms[6];

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        // check if spell ids exist in dbc
        return ValidateSpellInfo(PolymorhForms);
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        if (Unit* target = GetCaster()->FindNearestCreature(NPC_AUROSALIA, 30.0f))
            if (target->GetTypeId() == TYPEID_UNIT)
                target->CastSpell(target, PolymorhForms[urand(0, 5)], true);
    }

    void Register() override
    {
        // add dummy effect spell handler to Polymorph visual
        OnEffectHitTarget.Register(&spell_gen_polymorph_cast_visual::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

uint32 const spell_gen_polymorph_cast_visual::PolymorhForms[6] =
{
    SPELL_MAGE_SQUIRREL_FORM,
    SPELL_MAGE_GIRAFFE_FORM,
    SPELL_MAGE_SERPENT_FORM,
    SPELL_MAGE_DRAGONHAWK_FORM,
    SPELL_MAGE_WORGEN_FORM,
    SPELL_MAGE_SHEEP_FORM
};

enum DalaranShopKeeper
{
    SPELL_DALARAN_SHOP_KEEPER_AOE           = 60912,
    SPELL_DALARAN_SHOP_KEEPER_PING          = 60909,
    SPELL_DALARAN_SHOP_KEEPER_DUMMY_AURA    = 61354
};

// 60913 - [DND] Dalaran - Shop Keeper Greeting
class spell_gen_dalaran_shop_keeper_greeting_periodic : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_DALARAN_SHOP_KEEPER_AOE, SPELL_DALARAN_SHOP_KEEPER_DUMMY_AURA });
    }

    void HandlePeriodic(AuraEffect const* /*aurEff*/)
    {
        if (!GetTarget()->HasAura(SPELL_DALARAN_SHOP_KEEPER_DUMMY_AURA))
            GetTarget()->CastSpell(nullptr, SPELL_DALARAN_SHOP_KEEPER_AOE);
    }

    void Register() override
    {
        OnEffectPeriodic.Register(&spell_gen_dalaran_shop_keeper_greeting_periodic::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

// 60912 - [DND] Dalaran - Shop Keeper Greeting
class spell_gen_dalaran_shop_keeper_greeting_aoe : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_DALARAN_SHOP_KEEPER_PING });
    }

    void AfterApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Unit* caster = GetCaster())
            GetTarget()->CastSpell(nullptr, SPELL_DALARAN_SHOP_KEEPER_PING, CastSpellExtraArgs(TRIGGERED_FULL_MASK).SetCustomArg(caster->GetGUID()));
    }

    void Register() override
    {
        AfterEffectApply.Register(&spell_gen_dalaran_shop_keeper_greeting_aoe::AfterApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 60909 - [DND] Dalaran - Shop Keeper Greeting
class spell_gen_dalaran_shop_keeper_greeting_ping : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_DALARAN_SHOP_KEEPER_DUMMY_AURA });
    }

    // We are going to skip an endless number of condition entries and just provide the target guid right away.
    void SelectVendor(WorldObject*& target)
    {
        if (GetSpell()->m_customArg.has_value())
        {
            ObjectGuid targetGuid = std::any_cast<ObjectGuid>(GetSpell()->m_customArg);
            target = ObjectAccessor::GetCreature(*GetCaster(), targetGuid);
        }
    }

    void HandleDummyEffect(SpellEffIndex /*effIndex*/)
    {
        GetHitUnit()->CastSpell(nullptr, SPELL_DALARAN_SHOP_KEEPER_DUMMY_AURA);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_dalaran_shop_keeper_greeting_ping::HandleDummyEffect, EFFECT_0, SPELL_EFFECT_DUMMY);
        OnObjectTargetSelect.Register(&spell_gen_dalaran_shop_keeper_greeting_ping::SelectVendor, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
    }
};

// 25046 - Arcane Torrent (Racial)
// 28730 - Arcane Torrent (Racial)
// 50613 - Arcane Torrent (Racial)
// 69179 - Arcane Torrent (Racial)
// 80483 - Arcane Torrent (Racial)
class spell_gen_arcane_torrent_racial : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfi*/) override
    {
        return ValidateSpellInfo({ SPELL_GEN_INTERRUPT });
    }

    void HandleCreatureInterrupt(SpellEffIndex /*effIndex*/)
    {
        Creature* target = GetHitCreature();
        if (!target)
            return;

        if (Unit* caster = GetCaster())
            caster->CastSpell(target, SPELL_GEN_INTERRUPT, true);
    }

    void Register() override
    {
        OnEffectHitTarget.Register(&spell_gen_arcane_torrent_racial::HandleCreatureInterrupt, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
    }
};
}

void AddSC_generic_spell_scripts()
{
    using namespace Spells::Generic;
    new spell_gen_absorb0_hitlimit1();
    new spell_gen_adaptive_warding();
    new spell_gen_alchemist_stone();
    new spell_gen_allow_cast_from_item_only();
    new spell_gen_animal_blood();
    RegisterSpellScript(spell_gen_arcane_torrent_racial);
    new spell_gen_aura_of_anger();
    new spell_gen_aura_service_uniform();
    new spell_gen_av_drekthar_presence();
    new spell_gen_bandage();
    new spell_gen_blood_reserve();
    new spell_gen_blade_warding();
    new spell_gen_bonked();
    new spell_gen_break_shield("spell_gen_break_shield");
    new spell_gen_break_shield("spell_gen_tournament_counterattack");
    new spell_gen_burn_brutallus();
    new spell_gen_burning_depths_necrolyte_image();
    new spell_gen_cannibalize();
    new spell_gen_chaos_blast();
    RegisterSpellScript(spell_gen_clear_fear_poly);
    new spell_gen_clone();
    new spell_gen_clone_weapon();
    new spell_gen_clone_weapon_aura();
    new spell_gen_count_pct_from_max_hp("spell_gen_default_count_pct_from_max_hp");
    new spell_gen_count_pct_from_max_hp("spell_gen_50pct_count_pct_from_max_hp", 50);
    RegisterSpellScript(spell_gen_create_lance);
    new spell_gen_dalaran_disguise("spell_gen_sunreaver_disguise");
    new spell_gen_dalaran_disguise("spell_gen_silver_covenant_disguise");
    RegisterSpellScript(spell_gen_dalaran_shop_keeper_greeting_periodic);
    RegisterSpellScript(spell_gen_dalaran_shop_keeper_greeting_aoe);
    RegisterSpellScript(spell_gen_dalaran_shop_keeper_greeting_ping);
    new spell_gen_decay_over_time_fungal_decay();
    new spell_gen_decay_over_time_tail_sting();
    new spell_gen_defend();
    new spell_gen_despawn_self();
    new spell_gen_divine_storm_cd_reset();
    new spell_gen_ds_flush_knockback();
    new spell_gen_dungeon_credit();
    new spell_gen_elune_candle();
    RegisterSpellScript(spell_gen_flurry_of_claws);
    RegisterSpellScript(spell_gen_feign_death_all_flags);
    RegisterSpellScript(spell_gen_feign_death_all_flags_uninteractible);
    RegisterSpellScript(spell_gen_feign_death_all_flags_no_uninteractible);
    RegisterSpellScript(spell_gen_feign_death_no_dyn_flag);
    RegisterSpellScript(spell_gen_feign_death_no_prevent_emotes);
    new spell_gen_gadgetzan_transporter_backfire();
    new spell_gen_gift_of_naaru();
    new spell_gen_gnomish_transporter();
    RegisterSpellScriptWithArgs(spell_gen_increase_stats_buff, "spell_pal_blessing_of_kings");
    RegisterSpellScriptWithArgs(spell_gen_increase_stats_buff, "spell_pal_blessing_of_might");
    RegisterSpellScriptWithArgs(spell_gen_increase_stats_buff, "spell_dru_mark_of_the_wild");
    RegisterSpellScriptWithArgs(spell_gen_increase_stats_buff, "spell_pri_power_word_fortitude");
    RegisterSpellScriptWithArgs(spell_gen_increase_stats_buff, "spell_pri_shadow_protection");
    RegisterSpellScriptWithArgs(spell_gen_increase_stats_buff, "spell_mage_arcane_brilliance");
    RegisterSpellScriptWithArgs(spell_gen_increase_stats_buff, "spell_mage_dalaran_brilliance");
    new spell_gen_interrupt();
    RegisterSpellScript(spell_gen_launch_quest_aura);
    RegisterSpellScriptWithArgs(spell_gen_lifebloom, "spell_hexlord_lifebloom", SPELL_HEXLORD_MALACRASS_LIFEBLOOM_FINAL_HEAL);
    RegisterSpellScriptWithArgs(spell_gen_lifebloom, "spell_tur_ragepaw_lifebloom", SPELL_TUR_RAGEPAW_LIFEBLOOM_FINAL_HEAL);
    RegisterSpellScriptWithArgs(spell_gen_lifebloom, "spell_cenarion_scout_lifebloom", SPELL_CENARION_SCOUT_LIFEBLOOM_FINAL_HEAL);
    RegisterSpellScriptWithArgs(spell_gen_lifebloom, "spell_twisted_visage_lifebloom", SPELL_TWISTED_VISAGE_LIFEBLOOM_FINAL_HEAL);
    RegisterSpellScriptWithArgs(spell_gen_lifebloom, "spell_faction_champion_dru_lifebloom", SPELL_FACTION_CHAMPIONS_DRU_LIFEBLOOM_FINAL_HEAL);
    new spell_gen_mounted_charge();
    new spell_gen_moss_covered_feet();
    new spell_gen_netherbloom();
    new spell_gen_obsidian_armor();
    new spell_gen_oracle_wolvar_reputation();
    new spell_gen_orc_disguise();
    new spell_gen_paralytic_poison();
    RegisterSpellScriptWithArgs(spell_gen_proc_below_pct_damaged, "spell_item_soul_harvesters_charm");
    RegisterSpellScriptWithArgs(spell_gen_proc_below_pct_damaged, "spell_item_commendation_of_kaelthas");
    RegisterSpellScriptWithArgs(spell_gen_proc_below_pct_damaged, "spell_item_corpse_tongue_coin");
    RegisterSpellScriptWithArgs(spell_gen_proc_below_pct_damaged, "spell_item_corpse_tongue_coin_heroic");
    RegisterSpellScriptWithArgs(spell_gen_proc_below_pct_damaged, "spell_item_petrified_twilight_scale");
    RegisterSpellScriptWithArgs(spell_gen_proc_below_pct_damaged, "spell_item_petrified_twilight_scale_heroic");
    RegisterSpellScriptWithArgs(spell_gen_proc_below_pct_damaged, "spell_item_proc_armor");
    RegisterSpellScriptWithArgs(spell_gen_proc_below_pct_damaged, "spell_item_proc_mastery_below_35");
    RegisterSpellScriptWithArgs(spell_gen_proc_below_pct_damaged, "spell_item_proc_dodge_below_35");
    RegisterSpellScriptWithArgs(spell_gen_proc_below_pct_damaged, "spell_item_loom_of_fate");
    new spell_gen_proc_charge_drop_only();
    new spell_gen_parachute();
    new spell_gen_pet_summoned();
    RegisterSpellScript(spell_gen_profession_research);
    RegisterSpellScript(spell_gen_remove_flight_auras);
    RegisterSpellAndAuraScriptPair(spell_gen_replenishment, spell_gen_replenishment_AuraScript);
    // Running Wild
    RegisterSpellAndAuraScriptPair(spell_gen_running_wild, spell_gen_running_wild_AuraScript);
    RegisterSpellScript(spell_gen_two_forms);
    RegisterSpellScript(spell_gen_darkflight);
    RegisterSpellScript(spell_gen_enable_worgen_altered_form);
    /*                          */
    RegisterSpellScript(spell_gen_seaforium_blast);
    RegisterSpellScript(spell_gen_spectator_cheer_trigger);
    RegisterSpellScript(spell_gen_spirit_healer_res);
    RegisterSpellScriptWithArgs(spell_gen_summon_elemental, "spell_gen_summon_fire_elemental", SPELL_SUMMON_FIRE_ELEMENTAL);
    RegisterSpellScriptWithArgs(spell_gen_summon_elemental, "spell_gen_summon_earth_elemental", SPELL_SUMMON_EARTH_ELEMENTAL);
    RegisterSpellScript(spell_gen_summon_tournament_mount);
    RegisterSpellScript(spell_gen_throw_shield);
    RegisterSpellScript(spell_gen_tournament_duel);
    RegisterSpellScript(spell_gen_tournament_pennant);
    RegisterSpellScript(spell_gen_turkey_marker);
    RegisterSpellScript(spell_gen_upper_deck_create_foam_sword);
    RegisterSpellScript(spell_gen_vampiric_touch);
    RegisterSpellScript(spell_gen_vehicle_scaling_trigger);
    RegisterSpellScript(spell_gen_vehicle_scaling);
    RegisterSpellScript(spell_gen_vendor_bark_trigger);
    RegisterSpellScript(spell_gen_wg_water);
    RegisterSpellScript(spell_gen_whisper_gulch_yogg_saron_whisper);
    RegisterSpellScript(spell_gen_eject_all_passengers);
    RegisterSpellScript(spell_gen_eject_passenger);
    RegisterSpellScript(spell_gen_gm_freeze);
    RegisterSpellScript(spell_gen_stand);
    RegisterSpellScript(spell_gen_mixology_bonus);
    RegisterSpellScript(spell_gen_landmine_knockback_achievement);
    RegisterSpellScript(spell_gen_clear_debuffs);
    RegisterSpellScript(spell_gen_pony_mount_check);
    RegisterSpellScript(spell_gen_shroud_of_death);
    RegisterSpellScript(spell_gen_armor_specialization);
    RegisterSpellScript(spell_gen_pvp_trinket);
    RegisterSpellScript(spell_gen_blink);
    RegisterSpellScript(spell_gen_toxic_blow_dart);
    RegisterSpellScript(spell_gen_projectile_goods);
    RegisterSpellScript(spell_gen_vengeance);
    RegisterSpellScript(spell_gen_vengeance_triggered);
    RegisterSpellScript(spell_gen_gilneas_prison_periodic_dummy);
    RegisterSpellScript(spell_gen_throw_torch);
    RegisterSpellScript(spell_gen_revserse_cast_mirror_image);
    RegisterSpellScript(spell_gen_mirror_image_aura);
    RegisterSpellScript(spell_gen_reverse_cast_ride_vehicle);
    RegisterSpellScript(spell_gen_launch_quest);
    RegisterSpellScript(spell_gen_charmed_unit_spell_cooldown);
    RegisterSpellScript(spell_gen_sunflower_dnd);
    RegisterSpellScript(spell_gen_guild_battle_standard);
    RegisterSpellScript(spell_gen_guild_battle_standard_buff);
    RegisterSpellScript(spell_gen_mobile_banking);
    RegisterSpellScript(spell_gen_cauldron_of_battle);
    RegisterSpellScript(spell_gen_flask_of_battle);
    RegisterSpellScript(spell_gen_ghost);
    RegisterSpellScript(spell_gen_zero_energy_zero_regen);
    RegisterSpellScript(spell_gen_wounded);
    RegisterSpellScript(spell_gen_rocket_barrage);
    RegisterSpellScript(spell_gen_face_rage);
    RegisterSpellScript(spell_gen_shadowmeld);
    RegisterSpellScript(spell_gen_vehicle_control_link);
    RegisterSpellScript(spell_gen_polymorph_cast_visual);
    RegisterSpellScript(spell_gen_prevent_emotes);
}
