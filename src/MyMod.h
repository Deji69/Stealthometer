#pragma once

#include <IPluginInterface.h>

#include <Glacier/SGameUpdateEvent.h>

class MyMod : public IPluginInterface {
public:
    void OnEngineInitialized() override;
    ~MyMod() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;

private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
    DECLARE_PLUGIN_DETOUR(MyMod, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& p_SceneData);

private:
    bool m_ShowMessage = false;
};

DEFINE_ZHM_PLUGIN(MyMod)
