#include <hex/plugin.hpp>
#include <discord.h>
#include <hex/api/imhex_api.hpp>
#include <hex/api/content_registry.hpp>
#include <hex/api/event_manager.hpp>
#include <hex/api/localization_manager.hpp>
#include <hex/providers/provider.hpp>
#include <romfs/romfs.hpp>
#include <nlohmann/json.hpp>
#include <hex/helpers/logger.hpp>
#include <hex/helpers/fmt.hpp>
#include <wolv/utils/guards.hpp>
#include <hex/api/events/events_provider.hpp>
#include <hex/api/events/events_interaction.hpp>
#include <hex/api/events/events_gui.hpp>  
#include <hex/api/events/events_lifecycle.hpp>  

using namespace hex;

// Make sure these event types exist and are included:
using hex::EventProviderChanged;
using hex::EventProviderOpened;
using hex::EventRegionSelected;
using hex::EventFrameEnd;
using hex::EventWindowClosing;
using hex::EventAnySettingChanged;

constexpr static auto DiscordClientID = 1060827018196955177;
constexpr static auto LargeIconID = "imhex_logo";

static bool s_rpcEnabled = false;
static bool s_showProvider = false;
static bool s_showSelection = false;
static bool s_showTimestamp = false;

static time_t g_startTime = 0;

namespace {

    discord::Core *core = nullptr;

    void updateActivity() {
        if (!s_rpcEnabled) {
            core->ActivityManager().UpdateActivity({}, [](auto) { });
            return;
        }

        static bool updateTimeStamp = true;

        discord::Activity activity = {};
        activity.SetType(discord::ActivityType::Playing);

        if (s_showTimestamp) {
            if (updateTimeStamp) {
                g_startTime = std::time(nullptr);
                updateTimeStamp = false;
            }
            activity.GetTimestamps().SetStart(g_startTime);
        } else {
            activity.GetTimestamps().SetStart(0);
            updateTimeStamp = true;
        }

        if (ImHexApi::Provider::isValid()) {
            std::string state;
            std::string details;

            if (s_showSelection) {
                auto selection = ImHexApi::HexEditor::getSelection();
                if (selection.has_value()) {
                    details += hex::format("[ 0x{0:04x} - 0x{1:04x} ]\n", selection->getStartAddress(), selection->getEndAddress());
                }
            }

            activity.SetDetails(details.c_str());

            if (s_showProvider)
                state = ImHexApi::Provider::get()->getName();

            activity.SetState(state.c_str());
        }

        activity.GetAssets().SetLargeText(IMHEX_VERSION);
        activity.GetAssets().SetLargeImage(LargeIconID);

        core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
            if (result == discord::Result::Ok)
                log::info("Successfully updated activity");
            else
                log::error("Failed to update activity");
        });
    }

    void registerEvents() {
        EventManager::subscribe<EventProviderChanged>([](prv::Provider* oldProvider, prv::Provider* currentProvider) {
            (void)oldProvider;
            (void)currentProvider;
            updateActivity();
        });

        EventManager::subscribe<EventProviderOpened>([](prv::Provider* provider) {
            (void)provider;
            updateActivity();
        });

        EventManager::subscribe<EventRegionSelected>(nullptr, [](const ImHexApi::HexEditor::ProviderRegion &region) {
            updateActivity();
        });

        EventManager::subscribe<EventFrameEnd>([]{
            core->RunCallbacks();
        });

        EventManager::subscribe<EventWindowClosing>([](GLFWwindow* window){
            (void)window;
            core->ActivityManager().ClearActivity([](discord::Result result) {
                if (result == discord::Result::Ok)
                    log::info("Successfully cleared activity");
                else
                    log::error("Failed to clear activity");
            });
        });

        EventManager::subscribe<EventAnySettingChanged>([]{
            s_rpcEnabled = ContentRegistry::Settings::read<bool>("hex.discord_rpc.settings", "hex.discord_rpc.settings.enabled", false);
            s_showProvider = ContentRegistry::Settings::read<bool>("hex.discord_rpc.settings", "hex.discord_rpc.settings.show_provider", false);
            s_showSelection = ContentRegistry::Settings::read<bool>("hex.discord_rpc.settings", "hex.discord_rpc.settings.show_selection", false);
            s_showTimestamp = ContentRegistry::Settings::read<bool>("hex.discord_rpc.settings", "hex.discord_rpc.settings.show_timestamp", false);

            updateActivity();
        });
    }

    bool rpcEnabled() {
        return s_rpcEnabled;
    }

    void registerSettings() {
        ContentRegistry::Settings::setCategoryDescription("hex.discord_rpc.settings", "hex.discord_rpc.settings.desc");
        ContentRegistry::Settings::add<ContentRegistry::Settings::Widgets::Checkbox>("hex.discord_rpc.settings", "", "hex.discord_rpc.settings.enabled", false);
        ContentRegistry::Settings::add<ContentRegistry::Settings::Widgets::Checkbox>("hex.discord_rpc.settings", "", "hex.discord_rpc.settings.show_provider", false)
            .setEnabledCallback(rpcEnabled);
        ContentRegistry::Settings::add<ContentRegistry::Settings::Widgets::Checkbox>("hex.discord_rpc.settings", "", "hex.discord_rpc.settings.show_selection", false)
            .setEnabledCallback(rpcEnabled);
        ContentRegistry::Settings::add<ContentRegistry::Settings::Widgets::Checkbox>("hex.discord_rpc.settings", "", "hex.discord_rpc.settings.show_timestamp", false)
            .setEnabledCallback(rpcEnabled);
    }

}

IMHEX_PLUGIN_SETUP("Discord RPC", "WerWolv", "Discord Rich Presence Integration") {
    hex::log::debug("Using romfs: '{}'", romfs::name());
    for (auto &path : romfs::list("lang"))
        hex::ContentRegistry::Language::addLocalization(nlohmann::json::parse(romfs::get(path).string()));

    discord::Core::Create(DiscordClientID, DiscordCreateFlags_Default, &core);

    registerEvents();
    registerSettings();
}
