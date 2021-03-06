/*
 * AntiDisguise.cpp
 *
 *  Created on: Nov 16, 2016
 *      Author: nullifiedcat
 */

#include <settings/Bool.hpp>
#include "common.hpp"

static settings::Bool enable{ "remove.disguise", "0" };
static settings::Bool no_invisibility{ "remove.cloak", "0" };

namespace hacks::tf2::antidisguise
{

void Draw()
{
    CachedEntity *ent;
    if (!*enable && !*no_invisibility)
        return;

    for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
    {
        ent = ENTITY(i);
        if (CE_BAD(ent) || ent == LOCAL_E || ent->m_Type() != ENTITY_PLAYER || CE_INT(ent, netvar.iClass) != tf_class::tf_spy)
        {
            continue;
        }
        if (*enable)
            RemoveCondition<TFCond_Disguised>(ent);
        if (*no_invisibility)
        {
            RemoveCondition<TFCond_Cloaked>(ent);
            RemoveCondition<TFCond_CloakFlicker>(ent);
        }
    }
}
#if ENABLE_VISUALS
static InitRoutine EC([]() { EC::Register(EC::Draw, Draw, "antidisguise", EC::average); });
#endif
} // namespace hacks::tf2::antidisguise
