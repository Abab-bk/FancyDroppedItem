#include "RE/T/TESDataHandler.h"

class FancyDropEventSink : public RE::BSTEventSink<RE::TESDeathEvent>{
    RE::BSEventNotifyControl ProcessEvent(
        const RE::TESDeathEvent* event,
        RE::BSTEventSource<RE::TESDeathEvent>*  /*source*/
        ) override
    {
        if (event->dead) {
            return RE::BSEventNotifyControl::kContinue;
        }

        if (IsPlayerCharacter(event->actorDying)) {
            return RE::BSEventNotifyControl::kContinue;
        }

        std::vector<std::pair<RE::TESBoundObject*, int>> itemsToRemove;

        auto inventory = event->actorDying->GetInventory();
        for (const auto& entry : inventory) {
            auto item = entry.first;
            auto itemCount = entry.second.first;

            for (int i = 0; i < itemCount; i++)
            {
                RE::NiPoint3 randomPos;
                GenerateRandomPosAround(
                    randomPos,
                    event->actorDying->GetPosition(),
                    40
                    );
                auto itemRefr = PlaceObject(
                                    item,
                                    false,
                                    randomPos,
                                    event->actorDying->GetAngle(),
                                    event->actorDying->GetParentCell(),
                                    event->actorDying->GetWorldspace(),
                                    event->actorDying->CreateRefHandle()
                                        )->AsReference1();

                if (!itemRefr) {
                    logger::warn("PlaceObject failed for item: {}", item->GetName());
                    continue;
                }

                itemRefr->ApplyEffectShader(
                    RE::TESDataHandler::GetSingleton()->
                    LookupForm(0x0010bea9,
                               "Skyrim.esm")->As<RE::TESEffectShader>(),
                    20.
                );
            }

            itemsToRemove.emplace_back(item, itemCount);
        }

        for (const auto& itemToRemove : itemsToRemove) {
            event->actorDying->RemoveItem(itemToRemove.first,
                                          itemToRemove.second,
                                          RE::ITEM_REMOVE_REASON::kRemove,
                                          nullptr,
                                          nullptr
                                          );
        }

        return RE::BSEventNotifyControl::kContinue;
    }

public:
    static bool IsPlayerCharacter(const RE::NiPointer<RE::TESObjectREFR>& objPtr) {
        if (!objPtr) {
            return false;
        }

        RE::TESObjectREFR* obj = objPtr.get();
        if (!obj) {
            return false;
        }

        RE::Actor* actor = obj->As<RE::Actor>();
        if (!actor) {
            return false;
        }

        RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return false;
        }

        return actor == player;
    }

    static void GenerateRandomPosAround(
        RE::NiPoint3& outPos,
        RE::NiPoint3 originPos,
        float radius
    )
    {
        float const randomRadius = radius * sqrt(static_cast<float>(rand()) / RAND_MAX);
        float const randomAngle = 2.0F * 3.14F * static_cast<float>(rand()) / RAND_MAX;
        float const xOffset = randomRadius * cos(randomAngle);
        float const zOffset = randomRadius * sin(randomAngle);

        outPos.x = originPos.x + xOffset;
        outPos.y = originPos.y + 0.0F;
        outPos.z = originPos.z + zOffset;
    }

    static RE::NiPointer<RE::TESObjectREFR> PlaceObject(
        RE::TESBoundObject* a_baseToPlace,
        bool a_forcePersist, RE::NiPoint3& a_location, RE::NiPoint3 a_rotation,
        RE::TESObjectCELL* a_targetCell,
        RE::TESWorldSpace* a_worldSpace,
        const RE::BSPointerHandle<RE::TESObjectREFR>& a_linkedRoomRefHandle
        )
    {
        const auto handle = RE
                            ::TESDataHandler
                            ::GetSingleton()->
                            CreateReferenceAtLocation(
                                a_baseToPlace,
                                a_location,
                                a_rotation,
                                a_targetCell,
                                a_worldSpace,
                                nullptr,
                                nullptr,
                                a_linkedRoomRefHandle,
                                a_forcePersist,
                                true);
        return handle.get();
    }
};

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    Init(skse);

    const auto plugin{ SKSE::PluginDeclaration::GetSingleton() };
    const auto name{ plugin->GetName() };
    const auto version{ plugin->GetVersion() };

    logger::init();
    logger::info("{} {} is loading...", name, version);

    auto* eventSourceHandler = RE::ScriptEventSourceHolder::GetSingleton();
    eventSourceHandler->AddEventSink(new FancyDropEventSink());

    return true;
}
