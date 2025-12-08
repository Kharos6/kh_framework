#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <atomic>
#include <mutex>
#include <vector>
#include <algorithm>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Basic types
typedef uint64_t uint64;
typedef unsigned short anyID;

// Connection status
enum ConnectStatus {
    STATUS_DISCONNECTED = 0,
    STATUS_CONNECTING,
    STATUS_CONNECTED,
    STATUS_CONNECTION_ESTABLISHING,
    STATUS_CONNECTION_ESTABLISHED
};

// Talk status
enum TalkStatus {
    STATUS_NOT_TALKING = 0,
    STATUS_TALKING = 1,
    STATUS_TALKING_WHILE_DISABLED = 2
};

// Log levels
enum LogLevel {
    LogLevel_CRITICAL = 0,
    LogLevel_ERROR,
    LogLevel_WARNING,
    LogLevel_DEBUG,
    LogLevel_INFO,
    LogLevel_DEVEL
};

// Visibility
enum Visibility {
    ENTER_VISIBILITY = 0,
    RETAIN_VISIBILITY,
    LEAVE_VISIBILITY
};

// Client properties
enum ClientProperties {
    CLIENT_UNIQUE_IDENTIFIER = 0,
    CLIENT_NICKNAME,
    CLIENT_VERSION,
    CLIENT_PLATFORM,
    CLIENT_FLAG_TALKING,
    CLIENT_INPUT_MUTED,
    CLIENT_OUTPUT_MUTED,
    CLIENT_OUTPUTONLY_MUTED,
    CLIENT_INPUT_HARDWARE,
    CLIENT_OUTPUT_HARDWARE,
    CLIENT_INPUT_DEACTIVATED,
    CLIENT_IDLE_TIME,
    CLIENT_DEFAULT_CHANNEL,
    CLIENT_DEFAULT_CHANNEL_PASSWORD,
    CLIENT_SERVER_PASSWORD,
    CLIENT_META_DATA,
    CLIENT_IS_MUTED,
    CLIENT_IS_RECORDING,
    CLIENT_VOLUME_MODIFICATOR,
    CLIENT_VERSION_SIGN,
    CLIENT_SECURITY_HASH,
    CLIENT_ENCRYPTION_CIPHERS,
    CLIENT_ENDMARKER
};

// Channel properties
enum ChannelProperties {
    CHANNEL_NAME = 0,
    CHANNEL_TOPIC,
    CHANNEL_DESCRIPTION,
    CHANNEL_PASSWORD,
    CHANNEL_CODEC,
    CHANNEL_CODEC_QUALITY,
    CHANNEL_MAXCLIENTS,
    CHANNEL_MAXFAMILYCLIENTS,
    CHANNEL_ORDER,
    CHANNEL_FLAG_PERMANENT,
    CHANNEL_FLAG_SEMI_PERMANENT,
    CHANNEL_FLAG_DEFAULT,
    CHANNEL_FLAG_PASSWORD,
    CHANNEL_CODEC_LATENCY_FACTOR,
    CHANNEL_CODEC_IS_UNENCRYPTED,
    CHANNEL_SECURITY_SALT,
    CHANNEL_DELETE_DELAY,
    CHANNEL_ENDMARKER
};

// Virtual server properties
enum VirtualServerProperties {
    VIRTUALSERVER_UNIQUE_IDENTIFIER = 0,
    VIRTUALSERVER_NAME,
    VIRTUALSERVER_WELCOMEMESSAGE,
    VIRTUALSERVER_PLATFORM,
    VIRTUALSERVER_VERSION,
    VIRTUALSERVER_MAXCLIENTS,
    VIRTUALSERVER_PASSWORD,
    VIRTUALSERVER_CLIENTS_ONLINE,
    VIRTUALSERVER_CHANNELS_ONLINE,
    VIRTUALSERVER_CREATED,
    VIRTUALSERVER_UPTIME,
    VIRTUALSERVER_CODEC_ENCRYPTION_MODE,
    VIRTUALSERVER_ENDMARKER
};

// Codec types
enum CodecType {
    CODEC_SPEEX_NARROWBAND = 0,
    CODEC_SPEEX_WIDEBAND,
    CODEC_SPEEX_ULTRAWIDEBAND,
    CODEC_CELT_MONO,
    CODEC_OPUS_VOICE,
    CODEC_OPUS_MUSIC
};

// Codec encryption mode
enum CodecEncryptionMode {
    CODEC_ENCRYPTION_PER_CHANNEL = 0,
    CODEC_ENCRYPTION_FORCED_OFF,
    CODEC_ENCRYPTION_FORCED_ON
};

// Text message target mode
enum TextMessageTargetMode {
    TextMessageTarget_CLIENT = 1,
    TextMessageTarget_CHANNEL,
    TextMessageTarget_SERVER,
    TextMessageTarget_MAX
};

// Mute input/output status
enum MuteInputStatus {
    MUTEINPUT_NONE = 0,
    MUTEINPUT_MUTED
};

enum MuteOutputStatus {
    MUTEOUTPUT_NONE = 0,
    MUTEOUTPUT_MUTED
};

// Hardware input/output status
enum HardwareInputStatus {
    HARDWAREINPUT_DISABLED = 0,
    HARDWAREINPUT_ENABLED
};

enum HardwareOutputStatus {
    HARDWAREOUTPUT_DISABLED = 0,
    HARDWAREOUTPUT_ENABLED
};

// Input deactivation status
enum InputDeactivationStatus {
    INPUT_ACTIVE = 0,
    INPUT_DEACTIVATED
};

// Reason identifier
enum ReasonIdentifier {
    REASON_NONE = 0,
    REASON_MOVED,
    REASON_SUBSCRIPTION,
    REASON_LOST_CONNECTION,
    REASON_KICK_CHANNEL,
    REASON_KICK_SERVER,
    REASON_KICK_SERVER_BAN,
    REASON_SERVERSTOP,
    REASON_CLIENTDISCONNECT,
    REASON_CHANNELUPDATE,
    REASON_CHANNELEDIT,
    REASON_CLIENTDISCONNECT_SERVER_SHUTDOWN
};

// Channel group change type
enum ChannelGroupChangeType {
    ChannelGroupChange_ADD = 0,
    ChannelGroupChange_DEL
};

// Server group change type
enum ServerGroupChangeType {
    ServerGroupChange_ADD = 0,
    ServerGroupChange_DEL
};

// Plugin GUI item type
enum PluginGuiItemType {
    PLUGIN_GUI_SOUND_CAPTURE = 0,
    PLUGIN_GUI_SOUND_PLAYBACK,
    PLUGIN_GUI_HOTKEY,
    PLUGIN_GUI_SOUNDPACK,
    PLUGIN_GUI_SOUNDPACK_CATEGORY
};

// Plugin menu type
enum PluginMenuType {
    PLUGIN_MENU_TYPE_GLOBAL = 0,
    PLUGIN_MENU_TYPE_CHANNEL,
    PLUGIN_MENU_TYPE_CLIENT
};

// Plugin message target
enum PluginMessageTarget {
    PLUGIN_MESSAGE_TARGET_SERVER = 0,
    PLUGIN_MESSAGE_TARGET_CHANNEL,
    PLUGIN_MESSAGE_TARGET_CLIENT
};

// Plugin menu item
struct PluginMenuItem {
    enum PluginMenuType type;
    int id;
    char text[128];
    char icon[128];
};

// Plugin hotkey
struct PluginHotkey {
    char keyword[128];
    char description[128];
};

// Plugin bookmark item
struct PluginBookmarkItem {
    char* name;
    unsigned char isFolder;
    unsigned char reserved[3];
    union {
        char* uuid;
        struct PluginBookmarkList* folder;
    };
};

// Plugin bookmark list
struct PluginBookmarkList {
    int itemcount;
    struct PluginBookmarkItem* items;
};

// Connect tab
enum PluginConnectTab {
    PLUGIN_CONNECT_TAB_NEW = 0,
    PLUGIN_CONNECT_TAB_CURRENT,
    PLUGIN_CONNECT_TAB_NEW_IF_CURRENT_CONNECTED
};

// Error codes
enum Ts3ErrorType {
    ERROR_ok = 0x0000,
    ERROR_undefined = 0x0001,
    ERROR_not_implemented = 0x0002,
    ERROR_ok_no_update = 0x0003,
    ERROR_dont_notify = 0x0004,
    ERROR_lib_time_limit_reached = 0x0005,
    ERROR_command_not_found = 0x0100,
    ERROR_unable_to_bind_network_port = 0x0101,
    ERROR_no_network_port_available = 0x0102,
    ERROR_port_already_in_use = 0x0103,
    ERROR_client_invalid_id = 0x0200,
    ERROR_client_nickname_inuse = 0x0201,
    ERROR_client_protocol_limit_reached = 0x0203,
    ERROR_client_invalid_type = 0x0204,
    ERROR_client_already_subscribed = 0x0205,
    ERROR_client_not_logged_in = 0x0206,
    ERROR_client_could_not_validate_identity = 0x0207,
    ERROR_client_invalid_password = 0x0208,
    ERROR_client_too_many_clones_connected = 0x0209,
    ERROR_client_version_outdated = 0x020a,
    ERROR_client_is_online = 0x020b,
    ERROR_client_is_flooding = 0x020c,
    ERROR_client_hacked = 0x020d,
    ERROR_client_cannot_verify_now = 0x020e,
    ERROR_client_login_not_permitted = 0x020f,
    ERROR_client_not_subscribed = 0x0210,
    ERROR_channel_invalid_id = 0x0300,
    ERROR_channel_protocol_limit_reached = 0x0301,
    ERROR_channel_already_in = 0x0302,
    ERROR_channel_name_inuse = 0x0303,
    ERROR_channel_not_empty = 0x0304,
    ERROR_channel_can_not_delete_default = 0x0305,
    ERROR_channel_default_require_permanent = 0x0306,
    ERROR_channel_invalid_flags = 0x0307,
    ERROR_channel_parent_not_permanent = 0x0308,
    ERROR_channel_maxclients_reached = 0x0309,
    ERROR_channel_maxfamily_reached = 0x030a,
    ERROR_channel_invalid_order = 0x030b,
    ERROR_channel_no_filetransfer_supported = 0x030c,
    ERROR_channel_invalid_password = 0x030d,
    ERROR_channel_is_private_channel = 0x030e,
    ERROR_channel_invalid_security_hash = 0x030f,
    ERROR_server_invalid_id = 0x0400,
    ERROR_server_running = 0x0401,
    ERROR_server_is_shutting_down = 0x0402,
    ERROR_server_maxclients_reached = 0x0403,
    ERROR_server_invalid_password = 0x0404,
    ERROR_server_deployment_active = 0x0405,
    ERROR_server_unable_to_stop_own_server = 0x0406,
    ERROR_server_is_virtual = 0x0407,
    ERROR_server_wrong_machineid = 0x0408,
    ERROR_server_is_not_running = 0x0409,
    ERROR_server_is_booting = 0x040a,
    ERROR_server_status_invalid = 0x040b,
    ERROR_server_modal_quit = 0x040c,
    ERROR_server_version_outdated = 0x040d,
    ERROR_server_duplicated_running = 0x040e,
    ERROR_database = 0x0500,
    ERROR_database_empty_result = 0x0501,
    ERROR_database_duplicate_entry = 0x0502,
    ERROR_database_no_modifications = 0x0503,
    ERROR_database_constraint = 0x0504,
    ERROR_database_reinvoke = 0x0505,
    ERROR_parameter_quote = 0x0600,
    ERROR_parameter_invalid_count = 0x0601,
    ERROR_parameter_invalid = 0x0602,
    ERROR_parameter_not_found = 0x0603,
    ERROR_parameter_convert = 0x0604,
    ERROR_parameter_invalid_size = 0x0605,
    ERROR_parameter_missing = 0x0606,
    ERROR_parameter_checksum = 0x0607,
    ERROR_vs_critical = 0x0700,
    ERROR_connection_lost = 0x0701,
    ERROR_not_connected = 0x0702,
    ERROR_no_cached_connection_info = 0x0703,
    ERROR_currently_not_possible = 0x0704,
    ERROR_failed_connection_initialisation = 0x0705,
    ERROR_could_not_resolve_hostname = 0x0706,
    ERROR_invalid_server_connection_handler_id = 0x0707,
    ERROR_could_not_initialise_input_client = 0x0708,
    ERROR_clientlibrary_not_initialised = 0x0709,
    ERROR_serverlibrary_not_initialised = 0x070a,
    ERROR_whisper_too_many_targets = 0x070b,
    ERROR_whisper_no_targets = 0x070c,
    ERROR_connection_ip_protocol_missing = 0x070d,
    ERROR_file_invalid_name = 0x0800,
    ERROR_file_invalid_permissions = 0x0801,
    ERROR_file_already_exists = 0x0802,
    ERROR_file_not_found = 0x0803,
    ERROR_file_io_error = 0x0804,
    ERROR_file_invalid_transfer_id = 0x0805,
    ERROR_file_invalid_path = 0x0806,
    ERROR_file_no_files_available = 0x0807,
    ERROR_file_overwrite_excludes_resume = 0x0808,
    ERROR_file_invalid_size = 0x0809,
    ERROR_file_already_in_use = 0x080a,
    ERROR_file_could_not_open_connection = 0x080b,
    ERROR_file_no_space_left_on_device = 0x080c,
    ERROR_file_exceeds_file_system_maximum_size = 0x080d,
    ERROR_file_transfer_connection_timeout = 0x080e,
    ERROR_file_connection_lost = 0x080f,
    ERROR_file_exceeds_supplied_size = 0x0810
};

// Function pointer typedefs for TS3 client library
typedef unsigned int (*getClientLibVersionFunc)(char** result);
typedef unsigned int (*getClientLibVersionNumberFunc)(uint64* result);
typedef unsigned int (*spawnNewServerConnectionHandlerFunc)(int port, uint64* result);
typedef unsigned int (*destroyServerConnectionHandlerFunc)(uint64 serverConnectionHandlerID);
typedef unsigned int (*getErrorMessageFunc)(unsigned int errorCode, char** error);
typedef unsigned int (*freeMemoryFunc)(void* pointer);
typedef unsigned int (*logMessageFunc)(const char* logMessage, enum LogLevel severity, const char* channel, uint64 logID);
typedef unsigned int (*setLogVerbosityFunc)(enum LogLevel logVerbosity);
typedef unsigned int (*getPlaybackDeviceListFunc)(const char* modeID, char**** result);
typedef unsigned int (*getPlaybackModeListFunc)(char*** result);
typedef unsigned int (*getCaptureDeviceListFunc)(const char* modeID, char**** result);
typedef unsigned int (*getCaptureModeListFunc)(char*** result);
typedef unsigned int (*getDefaultPlaybackDeviceFunc)(const char* modeID, char*** result);
typedef unsigned int (*getDefaultPlayBackModeFunc)(char** result);
typedef unsigned int (*getDefaultCaptureDeviceFunc)(const char* modeID, char*** result);
typedef unsigned int (*getDefaultCaptureModeFunc)(char** result);
typedef unsigned int (*openPlaybackDeviceFunc)(uint64 serverConnectionHandlerID, const char* modeID, const char* playbackDevice);
typedef unsigned int (*openCaptureDeviceFunc)(uint64 serverConnectionHandlerID, const char* modeID, const char* captureDevice);
typedef unsigned int (*getCurrentPlaybackDeviceNameFunc)(uint64 serverConnectionHandlerID, char** result, int* isDefault);
typedef unsigned int (*getCurrentPlayBackModeFunc)(uint64 serverConnectionHandlerID, char** result);
typedef unsigned int (*getCurrentCaptureDeviceNameFunc)(uint64 serverConnectionHandlerID, char** result, int* isDefault);
typedef unsigned int (*getCurrentCaptureModeFunc)(uint64 serverConnectionHandlerID, char** result);
typedef unsigned int (*initiateGracefulPlaybackShutdownFunc)(uint64 serverConnectionHandlerID);
typedef unsigned int (*closePlaybackDeviceFunc)(uint64 serverConnectionHandlerID);
typedef unsigned int (*closeCaptureDeviceFunc)(uint64 serverConnectionHandlerID);
typedef unsigned int (*activateCaptureDeviceFunc)(uint64 serverConnectionHandlerID);
typedef unsigned int (*playWaveFileFunc)(uint64 serverConnectionHandlerID, const char* path);
typedef unsigned int (*playWaveFileHandleFunc)(uint64 serverConnectionHandlerID, const char* path, int loop, uint64* waveHandle);
typedef unsigned int (*pauseWaveFileHandleFunc)(uint64 serverConnectionHandlerID, uint64 waveHandle, int pause);
typedef unsigned int (*closeWaveFileHandleFunc)(uint64 serverConnectionHandlerID, uint64 waveHandle);
typedef unsigned int (*registerCustomDeviceFunc)(const char* deviceID, const char* deviceDisplayName, int capFrequency, int capChannels, int playFrequency, int playChannels);
typedef unsigned int (*unregisterCustomDeviceFunc)(const char* deviceID);
typedef unsigned int (*processCustomCaptureDataFunc)(const char* deviceName, const short* buffer, int samples);
typedef unsigned int (*acquireCustomPlaybackDataFunc)(const char* deviceName, short* buffer, int samples);
typedef unsigned int (*setLocalTestModeFunc)(uint64 serverConnectionHandlerID, int status);
typedef unsigned int (*startVoiceRecordingFunc)(uint64 serverConnectionHandlerID);
typedef unsigned int (*stopVoiceRecordingFunc)(uint64 serverConnectionHandlerID);
typedef unsigned int (*allowWhispersFromFunc)(uint64 serverConnectionHandlerID, anyID clientID);
typedef unsigned int (*removeFromAllowedWhispersFromFunc)(uint64 serverConnectionHandlerID, anyID clientID);
typedef unsigned int (*systemset3DListenerAttributesFunc)(uint64 serverConnectionHandlerID, const void* position, const void* forward, const void* up);
typedef unsigned int (*set3DWaveAttributesFunc)(uint64 serverConnectionHandlerID, uint64 waveHandle, const void* position);
typedef unsigned int (*systemset3DSettingsFunc)(uint64 serverConnectionHandlerID, float distanceFactor, float rolloffScale);
typedef unsigned int (*channelset3DAttributesFunc)(uint64 serverConnectionHandlerID, anyID clientID, const void* position);
typedef unsigned int (*startConnectionFunc)(uint64 serverConnectionHandlerID, const char* identity, const char* ip, unsigned int port, const char* nickname, const char** defaultChannelArray, const char* defaultChannelPassword, const char* serverPassword);
typedef unsigned int (*stopConnectionFunc)(uint64 serverConnectionHandlerID, const char* quitMessage);
typedef unsigned int (*requestClientMoveFunc)(uint64 serverConnectionHandlerID, anyID clientID, uint64 newChannelID, const char* password, const char* returnCode);
typedef unsigned int (*requestClientVariablesFunc)(uint64 serverConnectionHandlerID, anyID clientID, const char* returnCode);
typedef unsigned int (*requestClientKickFromChannelFunc)(uint64 serverConnectionHandlerID, anyID clientID, const char* kickReason, const char* returnCode);
typedef unsigned int (*requestClientKickFromServerFunc)(uint64 serverConnectionHandlerID, anyID clientID, const char* kickReason, const char* returnCode);
typedef unsigned int (*requestChannelDeleteFunc)(uint64 serverConnectionHandlerID, uint64 channelID, int force, const char* returnCode);
typedef unsigned int (*requestChannelMoveFunc)(uint64 serverConnectionHandlerID, uint64 channelID, uint64 newChannelParentID, uint64 newChannelOrder, const char* returnCode);
typedef unsigned int (*requestSendPrivateTextMsgFunc)(uint64 serverConnectionHandlerID, const char* message, anyID targetClientID, const char* returnCode);
typedef unsigned int (*requestSendChannelTextMsgFunc)(uint64 serverConnectionHandlerID, const char* message, uint64 targetChannelID, const char* returnCode);
typedef unsigned int (*requestSendServerTextMsgFunc)(uint64 serverConnectionHandlerID, const char* message, const char* returnCode);
typedef unsigned int (*requestConnectionInfoFunc)(uint64 serverConnectionHandlerID, anyID clientID, const char* returnCode);
typedef unsigned int (*requestClientSetWhisperListFunc)(uint64 serverConnectionHandlerID, anyID clientID, const uint64* targetChannelIDArray, const anyID* targetClientIDArray, const char* returnCode);
typedef unsigned int (*requestChannelSubscribeFunc)(uint64 serverConnectionHandlerID, const uint64* channelIDArray, const char* returnCode);
typedef unsigned int (*requestChannelSubscribeAllFunc)(uint64 serverConnectionHandlerID, const char* returnCode);
typedef unsigned int (*requestChannelUnsubscribeFunc)(uint64 serverConnectionHandlerID, const uint64* channelIDArray, const char* returnCode);
typedef unsigned int (*requestChannelUnsubscribeAllFunc)(uint64 serverConnectionHandlerID, const char* returnCode);
typedef unsigned int (*requestChannelDescriptionFunc)(uint64 serverConnectionHandlerID, uint64 channelID, const char* returnCode);
typedef unsigned int (*requestMuteClientsFunc)(uint64 serverConnectionHandlerID, const anyID* clientIDArray, const char* returnCode);
typedef unsigned int (*requestUnmuteClientsFunc)(uint64 serverConnectionHandlerID, const anyID* clientIDArray, const char* returnCode);
typedef unsigned int (*requestClientIDsFunc)(uint64 serverConnectionHandlerID, const char* clientUniqueIdentifier, const char* returnCode);
typedef unsigned int (*getClientIDFunc)(uint64 serverConnectionHandlerID, anyID* result);
typedef unsigned int (*getConnectionStatusFunc)(uint64 serverConnectionHandlerID, int* result);
typedef unsigned int (*getConnectionVariableAsUInt64Func)(uint64 serverConnectionHandlerID, anyID clientID, size_t flag, uint64* result);
typedef unsigned int (*getConnectionVariableAsDoubleFunc)(uint64 serverConnectionHandlerID, anyID clientID, size_t flag, double* result);
typedef unsigned int (*getConnectionVariableAsStringFunc)(uint64 serverConnectionHandlerID, anyID clientID, size_t flag, char** result);
typedef unsigned int (*cleanUpConnectionInfoFunc)(uint64 serverConnectionHandlerID, anyID clientID);
typedef unsigned int (*requestServerConnectionInfoFunc)(uint64 serverConnectionHandlerID, const char* returnCode);
typedef unsigned int (*getClientSelfVariableAsIntFunc)(uint64 serverConnectionHandlerID, size_t flag, int* result);
typedef unsigned int (*getClientSelfVariableAsStringFunc)(uint64 serverConnectionHandlerID, size_t flag, char** result);
typedef unsigned int (*setClientSelfVariableAsIntFunc)(uint64 serverConnectionHandlerID, size_t flag, int value);
typedef unsigned int (*setClientSelfVariableAsStringFunc)(uint64 serverConnectionHandlerID, size_t flag, const char* value);
typedef unsigned int (*flushClientSelfUpdatesFunc)(uint64 serverConnectionHandlerID, const char* returnCode);
typedef unsigned int (*getClientVariableAsIntFunc)(uint64 serverConnectionHandlerID, anyID clientID, size_t flag, int* result);
typedef unsigned int (*getClientVariableAsUInt64Func)(uint64 serverConnectionHandlerID, anyID clientID, size_t flag, uint64* result);
typedef unsigned int (*getClientVariableAsStringFunc)(uint64 serverConnectionHandlerID, anyID clientID, size_t flag, char** result);
typedef unsigned int (*getClientListFunc)(uint64 serverConnectionHandlerID, anyID** result);
typedef unsigned int (*getChannelOfClientFunc)(uint64 serverConnectionHandlerID, anyID clientID, uint64* result);
typedef unsigned int (*getChannelVariableAsIntFunc)(uint64 serverConnectionHandlerID, uint64 channelID, size_t flag, int* result);
typedef unsigned int (*getChannelVariableAsUInt64Func)(uint64 serverConnectionHandlerID, uint64 channelID, size_t flag, uint64* result);
typedef unsigned int (*getChannelVariableAsStringFunc)(uint64 serverConnectionHandlerID, uint64 channelID, size_t flag, char** result);
typedef unsigned int (*getChannelIDFromChannelNamesFunc)(uint64 serverConnectionHandlerID, char** channelNameArray, uint64* result);
typedef unsigned int (*setChannelVariableAsIntFunc)(uint64 serverConnectionHandlerID, uint64 channelID, size_t flag, int value);
typedef unsigned int (*setChannelVariableAsUInt64Func)(uint64 serverConnectionHandlerID, uint64 channelID, size_t flag, uint64 value);
typedef unsigned int (*setChannelVariableAsStringFunc)(uint64 serverConnectionHandlerID, uint64 channelID, size_t flag, const char* value);
typedef unsigned int (*flushChannelUpdatesFunc)(uint64 serverConnectionHandlerID, uint64 channelID, const char* returnCode);
typedef unsigned int (*flushChannelCreationFunc)(uint64 serverConnectionHandlerID, uint64 channelParentID, const char* returnCode);
typedef unsigned int (*getChannelListFunc)(uint64 serverConnectionHandlerID, uint64** result);
typedef unsigned int (*getChannelClientListFunc)(uint64 serverConnectionHandlerID, uint64 channelID, anyID** result);
typedef unsigned int (*getParentChannelOfChannelFunc)(uint64 serverConnectionHandlerID, uint64 channelID, uint64* result);
typedef unsigned int (*getServerConnectionHandlerListFunc)(uint64** result);
typedef unsigned int (*getServerVariableAsIntFunc)(uint64 serverConnectionHandlerID, size_t flag, int* result);
typedef unsigned int (*getServerVariableAsUInt64Func)(uint64 serverConnectionHandlerID, size_t flag, uint64* result);
typedef unsigned int (*getServerVariableAsStringFunc)(uint64 serverConnectionHandlerID, size_t flag, char** result);
typedef unsigned int (*requestServerVariablesFunc)(uint64 serverConnectionHandlerID);
typedef unsigned int (*getConnectionInfoFunc)(uint64 serverConnectionHandlerID, anyID clientID, unsigned short* ping, unsigned int* packetLoss);
typedef unsigned int (*createIdentityFunc)(char** result);
typedef unsigned int (*identityStringToUniqueIdentifierFunc)(const char* identityString, char** result);
typedef unsigned int (*getClientNeededPermissionFunc)(uint64 serverConnectionHandlerID, anyID clientID, unsigned int permissionID, int* result);
typedef unsigned int (*printMessageFunc)(uint64 serverConnectionHandlerID, const char* message, int targetMode);
typedef void (*notifyKeyEventFunc)(const char* pluginID, const char* keyIdentifier, int up_down);
typedef unsigned int (*sendPluginCommandFunc)(uint64 serverConnectionHandlerID, const char* pluginID, const char* command, int targetMode, const anyID* targetIDs, const char* returnCode);
typedef void (*getPluginPathFunc)(char* path, size_t maxLen, const char* pluginID);
typedef void (*getAppPathFunc)(char* path, size_t maxLen);
typedef void (*getResourcesPathFunc)(char* path, size_t maxLen);
typedef void (*getConfigPathFunc)(char* path, size_t maxLen);
typedef unsigned int (*requestSetClientChannelGroupFunc)(uint64 serverConnectionHandlerID, const uint64* channelGroupIDArray, const uint64* channelIDArray, const uint64* clientDatabaseIDArray, int arraySize, const char* returnCode);
typedef unsigned int (*getPreProcessorInfoValueFloatFunc)(uint64 serverConnectionHandlerID, const char* ident, float* result);
typedef unsigned int (*getPreProcessorConfigValueFunc)(uint64 serverConnectionHandlerID, const char* ident, char** result);
typedef unsigned int (*setPreProcessorConfigValueFunc)(uint64 serverConnectionHandlerID, const char* ident, const char* value);
typedef unsigned int (*getPlaybackConfigValueAsFloatFunc)(uint64 serverConnectionHandlerID, const char* ident, float* result);
typedef unsigned int (*setPlaybackConfigValueFunc)(uint64 serverConnectionHandlerID, const char* ident, const char* value);
typedef unsigned int (*setClientVolumeModifierFunc)(uint64 serverConnectionHandlerID, anyID clientID, float value);
typedef unsigned int (*printMessageToCurrentTabFunc)(const char* message);
typedef unsigned int (*getChannelConnectInfoFunc)(uint64 serverConnectionHandlerID, uint64 channelID, char* path, size_t maxLen);
typedef unsigned int (*sendFileFunc)(uint64 serverConnectionHandlerID, uint64 channelID, const char* channelPW, const char* file, int overwrite, int resume, const char* sourceDirectory, anyID* result, const char* returnCode);
typedef unsigned int (*requestFileFunc)(uint64 serverConnectionHandlerID, uint64 channelID, const char* channelPW, const char* file, int overwrite, int resume, const char* destinationDirectory, anyID* result, const char* returnCode);
typedef unsigned int (*haltTransferFunc)(uint64 serverConnectionHandlerID, anyID transferID, int deleteUnfinishedFile, const char* returnCode);
typedef unsigned int (*requestFileListFunc)(uint64 serverConnectionHandlerID, uint64 channelID, const char* channelPW, const char* path, const char* returnCode);
typedef unsigned int (*requestFileInfoFunc)(uint64 serverConnectionHandlerID, uint64 channelID, const char* channelPW, const char* file, const char* returnCode);
typedef unsigned int (*requestDeleteFileFunc)(uint64 serverConnectionHandlerID, uint64 channelID, const char* channelPW, const char** file, const char* returnCode);
typedef unsigned int (*requestCreateDirectoryFunc)(uint64 serverConnectionHandlerID, uint64 channelID, const char* channelPW, const char* directoryPath, const char* returnCode);
typedef unsigned int (*requestRenameFileFunc)(uint64 serverConnectionHandlerID, uint64 fromChannelID, const char* channelPW, uint64 toChannelID, const char* toChannelPW, const char* oldFile, const char* newFile, const char* returnCode);
typedef uint64 (*getTransferFileNameFunc)(anyID transferID);
typedef uint64 (*getTransferFilePathFunc)(anyID transferID);
typedef uint64 (*getTransferFileRemotePathFunc)(anyID transferID);
typedef uint64 (*getTransferFileSizeFunc)(anyID transferID);
typedef uint64 (*getTransferFileSizeDoneFunc)(anyID transferID);
typedef int (*isTransferSenderFunc)(anyID transferID);
typedef int (*getTransferStatusFunc)(anyID transferID);
typedef float (*getCurrentTransferSpeedFunc)(anyID transferID);
typedef float (*getAverageTransferSpeedFunc)(anyID transferID);
typedef uint64 (*getTransferRunTimeFunc)(anyID transferID);
typedef unsigned int (*serverPropertyFlagToStringFunc)(size_t serverPropertyFlag, char** resultString);
typedef unsigned int (*channelPropertyFlagToStringFunc)(size_t channelPropertyFlag, char** resultString);
typedef unsigned int (*clientPropertyFlagToStringFunc)(size_t clientPropertyFlag, char** resultString);
typedef void (*showHotkeySetupFunc)();
typedef void (*requestHotkeyInputDialogFunc)(const char* pluginID, const char* keyword, int isDown, void* qParentWindow);
typedef unsigned int (*getHotkeyFromKeywordFunc)(const char* pluginID, const char** keywords, char** hotkeys, size_t arrayLen, size_t hotkeyBufSize);
typedef unsigned int (*getClientDisplayNameFunc)(uint64 serverConnectionHandlerID, anyID clientID, char* result, size_t maxLen);
typedef unsigned int (*getBookmarkListFunc)(struct PluginBookmarkList** list);
typedef unsigned int (*getProfileListFunc)(enum PluginGuiItemType profile, int defaultProfileIdx, char*** result);
typedef unsigned int (*guiConnectFunc)(enum PluginConnectTab connectTab, const char* serverLabel, const char* serverAddress, const char* serverPassword, const char* nickname, const char* channel, const char* channelPassword, const char* captureProfile, const char* playbackProfile, const char* hotkeyProfile, const char* soundProfile, const char* userIdentity, const char* oneTimeKey, const char* phoneticName, uint64* serverConnectionHandlerID);
typedef unsigned int (*guiConnectBookmarkFunc)(enum PluginConnectTab connectTab, const char* bookmarkuuid, uint64* serverConnectionHandlerID);
typedef unsigned int (*createBookmarkFunc)(const char* bookmarkuuid, const char* serverLabel, const char* serverAddress, const char* serverPassword, const char* nickname, const char* channel, const char* channelPassword, const char* captureProfile, const char* playbackProfile, const char* hotkeyProfile, const char* soundProfile, const char* uniqueUserId, const char* oneTimeKey, const char* phoneticName);
typedef unsigned int (*getPermissionIDByNameFunc)(uint64 serverConnectionHandlerID, const char* permissionName, unsigned int* result);

// TS3Functions structure
struct TS3Functions {
    getClientLibVersionFunc getClientLibVersion;
    getClientLibVersionNumberFunc getClientLibVersionNumber;
    spawnNewServerConnectionHandlerFunc spawnNewServerConnectionHandler;
    destroyServerConnectionHandlerFunc destroyServerConnectionHandler;
    getErrorMessageFunc getErrorMessage;
    freeMemoryFunc freeMemory;
    logMessageFunc logMessage;
    setLogVerbosityFunc setLogVerbosity;
    getPlaybackDeviceListFunc getPlaybackDeviceList;
    getPlaybackModeListFunc getPlaybackModeList;
    getCaptureDeviceListFunc getCaptureDeviceList;
    getCaptureModeListFunc getCaptureModeList;
    getDefaultPlaybackDeviceFunc getDefaultPlaybackDevice;
    getDefaultPlayBackModeFunc getDefaultPlayBackMode;
    getDefaultCaptureDeviceFunc getDefaultCaptureDevice;
    getDefaultCaptureModeFunc getDefaultCaptureMode;
    openPlaybackDeviceFunc openPlaybackDevice;
    openCaptureDeviceFunc openCaptureDevice;
    getCurrentPlaybackDeviceNameFunc getCurrentPlaybackDeviceName;
    getCurrentPlayBackModeFunc getCurrentPlayBackMode;
    getCurrentCaptureDeviceNameFunc getCurrentCaptureDeviceName;
    getCurrentCaptureModeFunc getCurrentCaptureMode;
    initiateGracefulPlaybackShutdownFunc initiateGracefulPlaybackShutdown;
    closePlaybackDeviceFunc closePlaybackDevice;
    closeCaptureDeviceFunc closeCaptureDevice;
    activateCaptureDeviceFunc activateCaptureDevice;
    playWaveFileFunc playWaveFile;
    playWaveFileHandleFunc playWaveFileHandle;
    pauseWaveFileHandleFunc pauseWaveFileHandle;
    closeWaveFileHandleFunc closeWaveFileHandle;
    registerCustomDeviceFunc registerCustomDevice;
    unregisterCustomDeviceFunc unregisterCustomDevice;
    processCustomCaptureDataFunc processCustomCaptureData;
    acquireCustomPlaybackDataFunc acquireCustomPlaybackData;
    setLocalTestModeFunc setLocalTestMode;
    startVoiceRecordingFunc startVoiceRecording;
    stopVoiceRecordingFunc stopVoiceRecording;
    allowWhispersFromFunc allowWhispersFrom;
    removeFromAllowedWhispersFromFunc removeFromAllowedWhispersFrom;
    systemset3DListenerAttributesFunc systemset3DListenerAttributes;
    set3DWaveAttributesFunc set3DWaveAttributes;
    systemset3DSettingsFunc systemset3DSettings;
    channelset3DAttributesFunc channelset3DAttributes;
    startConnectionFunc startConnection;
    stopConnectionFunc stopConnection;
    requestClientMoveFunc requestClientMove;
    requestClientVariablesFunc requestClientVariables;
    requestClientKickFromChannelFunc requestClientKickFromChannel;
    requestClientKickFromServerFunc requestClientKickFromServer;
    requestChannelDeleteFunc requestChannelDelete;
    requestChannelMoveFunc requestChannelMove;
    requestSendPrivateTextMsgFunc requestSendPrivateTextMsg;
    requestSendChannelTextMsgFunc requestSendChannelTextMsg;
    requestSendServerTextMsgFunc requestSendServerTextMsg;
    requestConnectionInfoFunc requestConnectionInfo;
    requestClientSetWhisperListFunc requestClientSetWhisperList;
    requestChannelSubscribeFunc requestChannelSubscribe;
    requestChannelSubscribeAllFunc requestChannelSubscribeAll;
    requestChannelUnsubscribeFunc requestChannelUnsubscribe;
    requestChannelUnsubscribeAllFunc requestChannelUnsubscribeAll;
    requestChannelDescriptionFunc requestChannelDescription;
    requestMuteClientsFunc requestMuteClients;
    requestUnmuteClientsFunc requestUnmuteClients;
    requestClientIDsFunc requestClientIDs;
    getClientIDFunc getClientID;
    getConnectionStatusFunc getConnectionStatus;
    getConnectionVariableAsUInt64Func getConnectionVariableAsUInt64;
    getConnectionVariableAsDoubleFunc getConnectionVariableAsDouble;
    getConnectionVariableAsStringFunc getConnectionVariableAsString;
    cleanUpConnectionInfoFunc cleanUpConnectionInfo;
    requestServerConnectionInfoFunc requestServerConnectionInfo;
    getClientSelfVariableAsIntFunc getClientSelfVariableAsInt;
    getClientSelfVariableAsStringFunc getClientSelfVariableAsString;
    setClientSelfVariableAsIntFunc setClientSelfVariableAsInt;
    setClientSelfVariableAsStringFunc setClientSelfVariableAsString;
    flushClientSelfUpdatesFunc flushClientSelfUpdates;
    getClientVariableAsIntFunc getClientVariableAsInt;
    getClientVariableAsUInt64Func getClientVariableAsUInt64;
    getClientVariableAsStringFunc getClientVariableAsString;
    getClientListFunc getClientList;
    getChannelOfClientFunc getChannelOfClient;
    getChannelVariableAsIntFunc getChannelVariableAsInt;
    getChannelVariableAsUInt64Func getChannelVariableAsUInt64;
    getChannelVariableAsStringFunc getChannelVariableAsString;
    getChannelIDFromChannelNamesFunc getChannelIDFromChannelNames;
    setChannelVariableAsIntFunc setChannelVariableAsInt;
    setChannelVariableAsUInt64Func setChannelVariableAsUInt64;
    setChannelVariableAsStringFunc setChannelVariableAsString;
    flushChannelUpdatesFunc flushChannelUpdates;
    flushChannelCreationFunc flushChannelCreation;
    getChannelListFunc getChannelList;
    getChannelClientListFunc getChannelClientList;
    getParentChannelOfChannelFunc getParentChannelOfChannel;
    getServerConnectionHandlerListFunc getServerConnectionHandlerList;
    getServerVariableAsIntFunc getServerVariableAsInt;
    getServerVariableAsUInt64Func getServerVariableAsUInt64;
    getServerVariableAsStringFunc getServerVariableAsString;
    requestServerVariablesFunc requestServerVariables;
    getConnectionInfoFunc getConnectionInfo;
    printMessageToCurrentTabFunc printMessageToCurrentTab;
    printMessageFunc printMessage;
};

// Plugin configuration offers
enum {
    PLUGIN_OFFERS_NO_CONFIGURE = 0,
    PLUGIN_OFFERS_CONFIGURE_NEW_THREAD,
    PLUGIN_OFFERS_CONFIGURE_QT_THREAD
};

// Plugin return codes
enum {
    PLUGIN_RETURN_OK = 0,
    PLUGIN_RETURN_ERROR = 1,
    PLUGIN_RETURN_CONTINUE = 0,  // For event handling - continue processing
    PLUGIN_RETURN_HANDLED = 1    // For event handling - stop processing
};

#ifdef __cplusplus
}
#endif

// Plugin information
static const char* PLUGIN_NAME = "KH Framework Teamspeak";
static const char* PLUGIN_VERSION = "1.0.0";
static const char* PLUGIN_AUTHOR = "Kharos";
static const char* PLUGIN_DESCRIPTION = "Plugin containing Arma 3-controlled voice modulation.";
static const int PLUGIN_API_VERSION = 26;

// Shared memory structure (must match extension)
#pragma pack(push, 1)
struct TSVoiceEffectConfig {
    uint32_t version;
    uint32_t sequence_number;
    uint8_t effects_enabled;
    uint8_t reserved[3];
    
    // Ordered effect chain
    static constexpr uint8_t MAX_EFFECT_CHAIN = 32;
    uint8_t effect_chain_count;
    uint8_t effect_chain_types[MAX_EFFECT_CHAIN];
    float effect_chain_values[MAX_EFFECT_CHAIN];
    
    // Validation
    uint32_t checksum;
};

struct TSPluginStatus {
    uint32_t version;
    uint8_t plugin_active;
    uint8_t capturing;
    uint8_t transmitting;
    uint8_t connected;
    uint32_t sample_rate;
    uint32_t last_heartbeat;
};
#pragma pack(pop)

enum class TSEffectType : uint8_t {
    NONE = 0,
    LOWPASS,
    HIGHPASS,
    DISTORTION,
    BITCRUSH,
    NOISE,
    RING_MOD,
    REVERB,
    ECHO_DELAY,
    ECHO_DECAY,
    VOLUME,
    RADIO_STATIC,
    RADIO_SQUELCH,
    FREQUENCY_WOBBLE,
    PITCH_SHIFT,
    TREMOLO_RATE,
    TREMOLO_DEPTH,
    COMPRESSOR_THRESHOLD,
    COMPRESSOR_RATIO,
    TELEPHONE,
    UNDERWATER,
    AGC,
    CHORUS,
    CHORUS_RATE,
    FLANGER,
    FLANGER_RATE,
    BASS_BOOST
};

constexpr uint32_t TS_IPC_VERSION = 1;
constexpr const char* TS_SHARED_MEMORY_NAME = "KH_TeamSpeak_VoiceEffects";
constexpr const char* TS_STATUS_MEMORY_NAME = "KH_TeamSpeak_Status";
constexpr const char* TS_MUTEX_NAME = "KH_TeamSpeak_Mutex";

// TeamSpeak functions
static struct TS3Functions ts3Functions;

// IPC handles
static HANDLE g_shared_memory_handle = nullptr;
static HANDLE g_status_memory_handle = nullptr;
static HANDLE g_mutex_handle = nullptr;
static TSVoiceEffectConfig* g_effect_config = nullptr;
static TSPluginStatus* g_plugin_status = nullptr;

// Effect state
static std::mutex g_effect_mutex;
static TSVoiceEffectConfig g_cached_effects = {};
static uint32_t g_last_sequence = 0;

// Audio processing state (per channel state not needed for mono pre-processing)
struct EffectSlotState {
    // Filter states
    float lp_prev = 0.0f;
    float hp_prev_in = 0.0f;
    float hp_prev_out = 0.0f;
    
    // Ring modulator
    float ring_mod_phase = 0.0f;
    
    // Echo buffer
    std::vector<float> echo_buffer;
    size_t echo_pos = 0;
    float last_echo_delay = 0.0f;
    
    // Reverb buffers
    std::vector<float> reverb_buf1;
    std::vector<float> reverb_buf2;
    std::vector<float> reverb_buf3;
    size_t rev_pos1 = 0;
    size_t rev_pos2 = 0;
    size_t rev_pos3 = 0;
    
    // Radio effect state
    float squelch_state = 0.0f;
    float wobble_phase = 0.0f;
    
    // Pitch shift state
    std::vector<float> pitch_buffer;
    size_t pitch_write_pos = 0;
    size_t pitch_grain_size = 0;
    float pitch_read_pos_a = 0.0f;       // Grain A read position
    float pitch_read_pos_b = 0.0f;       // Grain B read position  
    float pitch_phase_a = 0.0f;          // Grain A phase (0-1)
    float pitch_phase_b = 0.5f;          // Grain B phase (0-1), starts offset
    bool pitch_initialized = false;
    
    // Tremolo state
    float tremolo_phase = 0.0f;
    
    // Compressor state
    float compressor_envelope = 0.0f;
    
    // Telephone filter state (bandpass)
    float tel_lp_prev = 0.0f;
    float tel_hp_prev_in = 0.0f;
    float tel_hp_prev_out = 0.0f;
    
    // Underwater filter state
    float uw_lp_prev1 = 0.0f;
    float uw_lp_prev2 = 0.0f;
    
    // AGC state
    float agc_gain = 1.0f;
    float agc_envelope = 0.0f;
    
    // Chorus state
    std::vector<float> chorus_buffer;
    size_t chorus_write_pos = 0;
    float chorus_lfo_phase = 0.0f;
    
    // Flanger state
    std::vector<float> flanger_buffer;
    size_t flanger_write_pos = 0;
    float flanger_lfo_phase = 0.0f;

    // Bass boost state
    float bass_lp_prev1 = 0.0f;
    float bass_lp_prev2 = 0.0f;
    
    void init(int sample_rate) {
        reverb_buf1.resize(static_cast<size_t>(sample_rate * 0.03f), 0.0f);
        reverb_buf2.resize(static_cast<size_t>(sample_rate * 0.04f), 0.0f);
        reverb_buf3.resize(static_cast<size_t>(sample_rate * 0.05f), 0.0f);
        size_t pitch_buf_size = static_cast<size_t>(sample_rate * 0.3f);  // 300ms buffer
        pitch_buffer.resize(pitch_buf_size, 0.0f);
        pitch_grain_size = static_cast<size_t>(sample_rate * 0.06f);      // 60ms grains
        pitch_initialized = false;
        chorus_buffer.resize(static_cast<size_t>(sample_rate * 0.05f), 0.0f);
        flanger_buffer.resize(static_cast<size_t>(sample_rate * 0.01f), 0.0f);
    }
    
    void reset() {
        lp_prev = 0.0f;
        hp_prev_in = 0.0f;
        hp_prev_out = 0.0f;
        ring_mod_phase = 0.0f;
        echo_pos = 0;
        last_echo_delay = 0.0f;
        rev_pos1 = rev_pos2 = rev_pos3 = 0;
        squelch_state = 0.0f;
        wobble_phase = 0.0f;
        pitch_read_pos_a = 0.0f;
        pitch_read_pos_b = 0.0f;
        pitch_phase_a = 0.0f;
        pitch_phase_b = 0.5f;
        pitch_write_pos = 0;
        pitch_initialized = false;
        tremolo_phase = 0.0f;
        compressor_envelope = 0.0f;
        tel_lp_prev = 0.0f;
        tel_hp_prev_in = 0.0f;
        tel_hp_prev_out = 0.0f;
        uw_lp_prev1 = 0.0f;
        uw_lp_prev2 = 0.0f;
        agc_gain = 1.0f;
        agc_envelope = 0.0f;
        chorus_write_pos = 0;
        chorus_lfo_phase = 0.0f;
        flanger_write_pos = 0;
        flanger_lfo_phase = 0.0f;
        bass_lp_prev1 = 0.0f;
        bass_lp_prev2 = 0.0f;
        if (!echo_buffer.empty()) std::fill(echo_buffer.begin(), echo_buffer.end(), 0.0f);
        if (!reverb_buf1.empty()) std::fill(reverb_buf1.begin(), reverb_buf1.end(), 0.0f);
        if (!reverb_buf2.empty()) std::fill(reverb_buf2.begin(), reverb_buf2.end(), 0.0f);
        if (!reverb_buf3.empty()) std::fill(reverb_buf3.begin(), reverb_buf3.end(), 0.0f);
        if (!pitch_buffer.empty()) std::fill(pitch_buffer.begin(), pitch_buffer.end(), 0.0f);
        if (!chorus_buffer.empty()) std::fill(chorus_buffer.begin(), chorus_buffer.end(), 0.0f);
        if (!flanger_buffer.empty()) std::fill(flanger_buffer.begin(), flanger_buffer.end(), 0.0f);
    }
};

struct AudioProcessorState {
    // Per-slot states for each position in the effect chain
    static constexpr size_t MAX_SLOTS = 32;
    EffectSlotState slots[MAX_SLOTS];
    
    // Shared state (not effect-specific)
    uint32_t rng_state = 12345;
    int sample_rate = 48000;
    
    void init(int rate) {
        sample_rate = rate;

        for (size_t i = 0; i < MAX_SLOTS; i++) {
            slots[i].init(rate);
        }
    }
    
    void reset() {
        rng_state = 12345;

        for (size_t i = 0; i < MAX_SLOTS; i++) {
            slots[i].reset();
        }
    }
};

static AudioProcessorState g_audio_state;
static std::mutex g_audio_state_mutex;
static std::atomic<bool> g_plugin_initialized{false};
static std::atomic<bool> g_connected{false};
static std::atomic<bool> g_transmitting{false};
static std::atomic<bool> g_local_test_mode{false};
static constexpr size_t LOOPBACK_BUFFER_SIZE = 48000; // ~1 second at 48kHz
static std::vector<float> g_loopback_buffer;
static size_t g_loopback_write_pos = 0;
static size_t g_loopback_read_pos = 0;
static std::atomic<size_t> g_loopback_available{0};
static std::mutex g_loopback_mutex;
static std::atomic<bool> g_tail_active{false};
static std::atomic<DWORD> g_tail_start_time{0};
static std::atomic<DWORD> g_tail_duration_ms{0};
static uint64 g_current_server_connection_handler = 0;

// Audio effect functions
namespace AudioEffects {
    static constexpr float PI = 3.14159265358979323846f;
    static constexpr float TWO_PI = 2.0f * PI;
    
    inline float lowpass(float sample, float cutoff, float& prev, int sample_rate) {
        if (cutoff >= 1.0f) return sample;
        float freq = 200.0f * std::pow(100.0f, cutoff);
        float rc = 1.0f / (freq * TWO_PI);
        float dt = 1.0f / static_cast<float>(sample_rate);
        float alpha = dt / (rc + dt);
        prev = prev + alpha * (sample - prev);
        return prev;
    }
    
    inline float highpass(float sample, float cutoff, float& prev_in, float& prev_out, int sample_rate) {
        if (cutoff <= 0.0f) return sample;
        float freq = 20.0f * std::pow(100.0f, cutoff);
        float rc = 1.0f / (freq * TWO_PI);
        float dt = 1.0f / static_cast<float>(sample_rate);
        float alpha = rc / (rc + dt);
        float output = alpha * (prev_out + sample - prev_in);
        prev_in = sample;
        prev_out = output;
        return output;
    }
        
    inline float distort(float sample, float amount) {
        if (amount <= 0.0f) return sample;
        float drive = 1.0f + amount * 10.0f;
        sample *= drive;
        float x = std::clamp(sample, -1.5f, 1.5f);
        return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
    }
    
    inline float bitcrush(float sample, float amount) {
        if (amount <= 0.0f) return sample;
        float bits = 16.0f - amount * 14.0f;
        float levels = std::pow(2.0f, bits);
        return std::round(sample * levels) / levels;
    }
    
    inline float add_noise(float sample, float amount, uint32_t& rng_state) {
        if (amount <= 0.0f) return sample;
        rng_state ^= rng_state << 13;
        rng_state ^= rng_state >> 17;
        rng_state ^= rng_state << 5;
        float noise = (static_cast<float>(rng_state) / static_cast<float>(UINT32_MAX)) * 2.0f - 1.0f;
        return sample + noise * amount * 0.3f;
    }
    
    inline float ring_mod(float sample, float& phase, float freq, int sample_rate) {
        if (freq <= 0.0f) return sample;
        float result = sample * std::sin(phase * TWO_PI);
        phase += freq / static_cast<float>(sample_rate);
        if (phase >= 1.0f) phase -= 1.0f;
        return result;
    }
    
    inline float reverb_comb(float sample, float amount, std::vector<float>& buffer, size_t& pos) {
        if (amount <= 0.0f || buffer.empty()) return sample;
        float delayed = buffer[pos];
        float output = sample + delayed * amount * 0.6f;
        buffer[pos] = sample + delayed * 0.5f;
        pos = (pos + 1) % buffer.size();
        return output;
    }
    
    inline float apply_reverb(float sample, float amount, EffectSlotState& state) {
        if (amount <= 0.0f) return sample;
        float rev1 = reverb_comb(sample, amount, state.reverb_buf1, state.rev_pos1);
        float rev2 = reverb_comb(sample, amount * 0.8f, state.reverb_buf2, state.rev_pos2);
        float rev3 = reverb_comb(sample, amount * 0.6f, state.reverb_buf3, state.rev_pos3);
        return (rev1 + rev2 + rev3) / 3.0f;
    }
    
    inline void update_echo_buffer(float echo_delay, int sample_rate, EffectSlotState& state) {
        if (echo_delay > 0.0f && std::abs(echo_delay - state.last_echo_delay) > 0.001f) {
            size_t echo_samples = static_cast<size_t>(std::min(echo_delay, 2.0f) * sample_rate);

            if (echo_samples > 0 && echo_samples != state.echo_buffer.size()) {
                size_t old_size = state.echo_buffer.size();
                state.echo_buffer.resize(echo_samples, 0.0f);
                
                // Keep position valid, wrap if buffer shrunk
                if (state.echo_pos >= echo_samples) {
                    state.echo_pos = 0;
                }
            }

            state.last_echo_delay = echo_delay;
        }
    }
    
    inline float apply_echo(float sample, float delay, float decay, EffectSlotState& state) {
        if (delay <= 0.0f || state.echo_buffer.empty()) return sample;
        float echo = state.echo_buffer[state.echo_pos];
        state.echo_buffer[state.echo_pos] = sample + echo * decay;
        state.echo_pos = (state.echo_pos + 1) % state.echo_buffer.size();
        return sample + echo * 0.5f;
    }

    inline float add_radio_static(float sample, float amount, uint32_t& rng_state) {
        if (amount <= 0.0f) return sample;
        rng_state ^= rng_state << 13;
        rng_state ^= rng_state >> 17;
        rng_state ^= rng_state << 5;
        float noise = (static_cast<float>(rng_state) / static_cast<float>(UINT32_MAX)) * 2.0f - 1.0f;
        
        // Occasional pops
        float pop = 0.0f;

        if ((rng_state & 0xFFFF) < static_cast<uint32_t>(amount * 100)) {
            pop = (noise > 0 ? 1.0f : -1.0f) * amount * 0.5f;
        }
        
        return sample * (1.0f - amount * 0.3f) + noise * amount * 0.15f + pop;
    }
    
    inline float apply_squelch(float sample, float amount, float& squelch_state) {
        if (amount <= 0.0f) return sample;
        float abs_sample = std::abs(sample);
        float threshold = (1.0f - amount) * 0.1f;
        
        // Squelch gate with hysteresis
        if (abs_sample > threshold * 1.2f) {
            squelch_state = 1.0f;
        } else if (abs_sample < threshold * 0.8f) {
            float release_coeff = std::exp(-1.0f / (0.010f * static_cast<float>(sample_rate))); // 10ms release
            squelch_state *= release_coeff;
        }
        
        return sample * squelch_state;
    }
    
    inline float apply_frequency_wobble(float sample, float& phase, float amount, int sample_rate) {
        if (amount <= 0.0f) return sample;
        float wobble_freq = 2.0f + amount * 5.0f; // 2-7 Hz
        float mod = std::sin(phase * TWO_PI) * amount * 0.1f;
        phase += wobble_freq / static_cast<float>(sample_rate);
        if (phase >= 1.0f) phase -= 1.0f;
        return sample * (1.0f + mod);
    }

    inline float apply_pitch_shift(float sample, float semitones, EffectSlotState& state, int sample_rate) {
        if (state.pitch_buffer.empty()) return sample;
        const size_t buf_size = state.pitch_buffer.size();
        const float buf_size_f = static_cast<float>(buf_size);
        const float grain_size_f = static_cast<float>(state.pitch_grain_size);
        const float phase_inc = 1.0f / grain_size_f;
        
        // Write input sample to circular buffer
        state.pitch_buffer[state.pitch_write_pos] = sample;
        float write_pos_f = static_cast<float>(state.pitch_write_pos);
        state.pitch_write_pos = (state.pitch_write_pos + 1) % buf_size;
        
        // Negligible shift - just return delayed sample for consistent latency
        if (std::abs(semitones) < 0.01f) {
            size_t read_idx = (state.pitch_write_pos + buf_size - state.pitch_grain_size) % buf_size;
            return state.pitch_buffer[read_idx];
        }
        
        const float pitch_ratio = std::pow(2.0f, semitones / 12.0f);
        
        // Hann window for smooth crossfade
        auto hann = [](float phase) -> float {
            return 0.5f * (1.0f - std::cos(phase * TWO_PI));
        };
        
        // Wrap position to buffer bounds
        auto wrap = [buf_size_f](float pos) -> float {
            while (pos < 0.0f) pos += buf_size_f;
            while (pos >= buf_size_f) pos -= buf_size_f;
            return pos;
        };
        
        // Linear interpolation read
        auto read_interp = [&](float pos) -> float {
            pos = wrap(pos);
            size_t idx0 = static_cast<size_t>(pos);
            size_t idx1 = (idx0 + 1) % buf_size;
            float frac = pos - static_cast<float>(idx0);
            return state.pitch_buffer[idx0] * (1.0f - frac) + state.pitch_buffer[idx1] * frac;
        };
        
        // Reset a grain's read position to optimal location behind write head
        auto reset_grain = [&](float& read_pos) {
            read_pos = wrap(write_pos_f - grain_size_f * 1.5f);
        };
        
        // Initialize on first use
        if (!state.pitch_initialized) {
            reset_grain(state.pitch_read_pos_a);
            state.pitch_phase_a = 0.0f;
            reset_grain(state.pitch_read_pos_b);
            state.pitch_phase_b = 0.5f;
            // Offset grain B's read position to match its phase offset
            state.pitch_read_pos_b = wrap(state.pitch_read_pos_b + grain_size_f * 0.5f * pitch_ratio);
            state.pitch_initialized = true;
        }
        
        // --- Process Grain A ---
        float weight_a = hann(state.pitch_phase_a);
        float sample_a = read_interp(state.pitch_read_pos_a);
        state.pitch_read_pos_a = wrap(state.pitch_read_pos_a + pitch_ratio);
        state.pitch_phase_a += phase_inc;
        
        if (state.pitch_phase_a >= 1.0f) {
            state.pitch_phase_a -= 1.0f;
            reset_grain(state.pitch_read_pos_a);
        }
        
        // --- Process Grain B ---
        float weight_b = hann(state.pitch_phase_b);
        float sample_b = read_interp(state.pitch_read_pos_b);
        state.pitch_read_pos_b = wrap(state.pitch_read_pos_b + pitch_ratio);
        state.pitch_phase_b += phase_inc;
        
        if (state.pitch_phase_b >= 1.0f) {
            state.pitch_phase_b -= 1.0f;
            reset_grain(state.pitch_read_pos_b);
        }
        
        // Combine - Hann windows at 50% overlap sum to 1.0
        return sample_a * weight_a + sample_b * weight_b;
    }

    inline float apply_tremolo(float sample, float& phase, float rate, float depth, int sample_rate) {
        if (rate <= 0.0f || depth <= 0.0f) return sample;
        float mod = 1.0f - depth * 0.5f * (1.0f + std::sin(phase * TWO_PI));
        phase += rate / static_cast<float>(sample_rate);
        if (phase >= 1.0f) phase -= 1.0f;
        return sample * mod;
    }

    inline float apply_compressor(float sample, float threshold, float ratio, float& envelope, int sample_rate) {
        if (threshold <= 0.0f || ratio <= 1.0f) return sample;
        float abs_sample = std::abs(sample);
        float attack_time = 0.010f;
        float release_time = 0.100f;
        float attack = 1.0f - std::exp(-1.0f / (attack_time * static_cast<float>(sample_rate)));
        float release = 1.0f - std::exp(-1.0f / (release_time * static_cast<float>(sample_rate)));
        
        if (abs_sample > envelope) {
            envelope += attack * (abs_sample - envelope);
        } else {
            envelope += release * (abs_sample - envelope);
        }
        
        // Apply compression
        float thresh_linear = threshold;

        if (envelope > thresh_linear) {
            float over = envelope - thresh_linear;
            float compressed_over = over / ratio;
            float gain = (thresh_linear + compressed_over) / envelope;
            return sample * gain;
        }
        
        return sample;
    }

    inline float apply_telephone_filter(float sample, float amount, 
                                        float& lp_prev, float& hp_prev_in, float& hp_prev_out,
                                        int sample_rate) {
        if (amount <= 0.0f) return sample;
        float hp_freq = 300.0f;
        float hp_rc = 1.0f / (hp_freq * TWO_PI);
        float dt = 1.0f / static_cast<float>(sample_rate);
        float hp_alpha = hp_rc / (hp_rc + dt);
        float hp_out = hp_alpha * (hp_prev_out + sample - hp_prev_in);
        hp_prev_in = sample;
        hp_prev_out = hp_out;
        
        // Low-pass at ~3400Hz
        float lp_freq = 3400.0f;
        float lp_rc = 1.0f / (lp_freq * TWO_PI);
        float lp_alpha = dt / (lp_rc + dt);
        lp_prev = lp_prev + lp_alpha * (hp_out - lp_prev);
        
        // Blend based on amount
        return sample * (1.0f - amount) + lp_prev * amount;
    }
    
    // Underwater effect - heavy lowpass with slight resonance
    inline float apply_underwater(float sample, float amount, 
                                  float& lp_prev1, float& lp_prev2,
                                  int sample_rate) {
        if (amount <= 0.0f) return sample;
        float cutoff = 200.0f + (1.0f - amount) * 800.0f;  // 200-1000 Hz
        float rc = 1.0f / (cutoff * TWO_PI);
        float dt = 1.0f / static_cast<float>(sample_rate);
        float alpha = dt / (rc + dt);
        
        // Two-pole filter for steeper rolloff
        lp_prev1 = lp_prev1 + alpha * (sample - lp_prev1);
        lp_prev2 = lp_prev2 + alpha * (lp_prev1 - lp_prev2);
        
        // Add slight "bubble" modulation
        float bubble = std::sin(lp_prev2 * PI * 4.0f) * amount * 0.1f;
        return sample * (1.0f - amount) + (lp_prev2 + bubble) * amount;
    }
    
    // AGC - Automatic Gain Control
    inline float apply_agc(float sample, float amount, float& gain, float& envelope, int sample_rate) {
        if (amount <= 0.0f) return sample;
        float target_level = 0.3f;
        float abs_sample = std::abs(sample);
        float attack_time = 0.001f;
        float release_time = 0.050f;
        float attack = 1.0f - std::exp(-1.0f / (attack_time * static_cast<float>(sample_rate)));
        float release = 1.0f - std::exp(-1.0f / (release_time * static_cast<float>(sample_rate)));
        
        if (abs_sample > envelope) {
            envelope += attack * (abs_sample - envelope);
        } else {
            envelope += release * (abs_sample - envelope);
        }
        
        // Calculate desired gain
        float desired_gain = 1.0f;

        if (envelope > 0.001f) {
            desired_gain = target_level / envelope;
            desired_gain = std::clamp(desired_gain, 0.1f, 10.0f);
        }
        
        // Smooth gain changes
        float smooth_coeff = 1.0f - std::exp(-1.0f / (0.010f * static_cast<float>(sample_rate))); // 10ms
        gain += smooth_coeff * (desired_gain - gain);
        
        // Blend based on amount
        float processed = sample * gain;
        return sample * (1.0f - amount) + processed * amount;
    }
    
    // Chorus effect
    inline float apply_chorus(float sample, float amount, float rate, EffectSlotState& state, int sample_rate) {
        if (amount <= 0.0f || state.chorus_buffer.empty()) return sample;
        size_t buf_size = state.chorus_buffer.size();
        state.chorus_buffer[state.chorus_write_pos] = sample;
        
        // LFO for delay modulation
        float lfo = 0.5f + 0.5f * std::sin(state.chorus_lfo_phase * TWO_PI);
        state.chorus_lfo_phase += rate / static_cast<float>(sample_rate);
        if (state.chorus_lfo_phase >= 1.0f) state.chorus_lfo_phase -= 1.0f;
        
        // Calculate delay (5-25ms modulated)
        float min_delay = sample_rate * 0.005f;
        float max_delay = sample_rate * 0.025f;
        float delay_samples = min_delay + lfo * (max_delay - min_delay);
        
        // Read from buffer with interpolation
        float read_pos = static_cast<float>(state.chorus_write_pos) - delay_samples;
        if (read_pos < 0) read_pos += buf_size;
        size_t read_idx = static_cast<size_t>(read_pos) % buf_size;
        size_t read_idx_next = (read_idx + 1) % buf_size;
        float frac = read_pos - std::floor(read_pos);
        float delayed = state.chorus_buffer[read_idx] * (1.0f - frac) + 
                        state.chorus_buffer[read_idx_next] * frac;
        
        state.chorus_write_pos = (state.chorus_write_pos + 1) % buf_size;
        
        // Mix dry and wet
        return sample * (1.0f - amount * 0.5f) + delayed * amount * 0.5f;
    }
    
    // Flanger effect
    inline float apply_flanger(float sample, float amount, float rate, EffectSlotState& state, int sample_rate) {
        if (amount <= 0.0f || state.flanger_buffer.empty()) return sample;
        size_t buf_size = state.flanger_buffer.size();
        
        // LFO for delay modulation
        float lfo = 0.5f + 0.5f * std::sin(state.flanger_lfo_phase * TWO_PI);
        state.flanger_lfo_phase += rate / static_cast<float>(sample_rate);
        if (state.flanger_lfo_phase >= 1.0f) state.flanger_lfo_phase -= 1.0f;
        float min_delay = sample_rate * 0.0005f;
        float max_delay = sample_rate * 0.005f;
        float delay_samples = min_delay + lfo * (max_delay - min_delay);
        
        // Read from buffer with interpolation
        float read_pos = static_cast<float>(state.flanger_write_pos) - delay_samples;
        if (read_pos < 0) read_pos += buf_size;
        size_t read_idx = static_cast<size_t>(read_pos) % buf_size;
        size_t read_idx_next = (read_idx + 1) % buf_size;
        float frac = read_pos - std::floor(read_pos);
        
        float delayed = state.flanger_buffer[read_idx] * (1.0f - frac) + 
                        state.flanger_buffer[read_idx_next] * frac;

        state.flanger_buffer[state.flanger_write_pos] = sample + delayed * amount * 0.5f;
        state.flanger_write_pos = (state.flanger_write_pos + 1) % buf_size;
        return sample + delayed * amount;
    }

    inline float apply_bass_boost(float sample, float amount, 
                                  float& lp_prev1, float& lp_prev2,
                                  int sample_rate) {
        if (amount <= 0.0f) return sample;
        
        // Cutoff frequency scales with amount: 80Hz (subtle) to 200Hz (full)
        float cutoff = 80.0f + amount * 120.0f;
        float rc = 1.0f / (cutoff * TWO_PI);
        float dt = 1.0f / static_cast<float>(sample_rate);
        float alpha = dt / (rc + dt);
        
        // Two-pole lowpass to extract bass frequencies
        lp_prev1 = lp_prev1 + alpha * (sample - lp_prev1);
        lp_prev2 = lp_prev2 + alpha * (lp_prev1 - lp_prev2);
        float bass = lp_prev2;
        
        // Boost gain: 1x to 4x based on amount
        float boost_gain = 1.0f + amount * 3.0f;
        float boosted_bass = bass * boost_gain;
        
        // Soft saturation for warmth and to prevent harsh clipping
        // tanh provides smooth limiting
        boosted_bass = std::tanh(boosted_bass * 1.5f) / 1.5f;
        
        // Replace original bass with boosted bass
        float output = sample - bass + boosted_bass;
        return output;
    }
}

// Calculate checksum for validation
static uint32_t calculate_checksum(const TSVoiceEffectConfig* config) {
    uint32_t sum = 0;
    const uint8_t* data = reinterpret_cast<const uint8_t*>(config);
    size_t size = offsetof(TSVoiceEffectConfig, checksum);
    
    for (size_t i = 0; i < size; i++) {
        sum = (sum << 1) | (sum >> 31);
        sum ^= data[i];
    }
    
    return sum;
}

static const char* get_effect_name(TSEffectType type) {
    switch (type) {
        case TSEffectType::LOWPASS: return "lowpass";
        case TSEffectType::HIGHPASS: return "highpass";
        case TSEffectType::DISTORTION: return "distortion";
        case TSEffectType::BITCRUSH: return "bitcrush";
        case TSEffectType::NOISE: return "noise";
        case TSEffectType::RING_MOD: return "ring_mod";
        case TSEffectType::REVERB: return "reverb";
        case TSEffectType::ECHO_DELAY: return "echo_delay";
        case TSEffectType::ECHO_DECAY: return "echo_decay";
        case TSEffectType::VOLUME: return "volume";
        case TSEffectType::RADIO_STATIC: return "radio_static";
        case TSEffectType::RADIO_SQUELCH: return "radio_squelch";
        case TSEffectType::FREQUENCY_WOBBLE: return "frequency_wobble";
        case TSEffectType::PITCH_SHIFT: return "pitch_shift";
        case TSEffectType::TREMOLO_RATE: return "tremolo_rate";
        case TSEffectType::TREMOLO_DEPTH: return "tremolo_depth";
        case TSEffectType::COMPRESSOR_THRESHOLD: return "compressor_threshold";
        case TSEffectType::COMPRESSOR_RATIO: return "compressor_ratio";
        case TSEffectType::TELEPHONE: return "telephone";
        case TSEffectType::UNDERWATER: return "underwater";
        case TSEffectType::AGC: return "agc";
        case TSEffectType::CHORUS: return "chorus";
        case TSEffectType::CHORUS_RATE: return "chorus_rate";
        case TSEffectType::FLANGER: return "flanger";
        case TSEffectType::FLANGER_RATE: return "flanger_rate";
        case TSEffectType::BASS_BOOST: return "bass_boost";
        default: return "unknown";
    }
}

static bool init_shared_memory() {
    // Open mutex
    g_mutex_handle = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, TS_MUTEX_NAME);

    if (g_mutex_handle == nullptr) {
        // Mutex doesn't exist yet, extension not running - that's OK
        return false;
    }
    
    // Open shared memory for effects
    g_shared_memory_handle = OpenFileMappingA(FILE_MAP_READ, FALSE, TS_SHARED_MEMORY_NAME);

    if (g_shared_memory_handle == nullptr) {
        CloseHandle(g_mutex_handle);
        g_mutex_handle = nullptr;
        return false;
    }
    
    g_effect_config = static_cast<TSVoiceEffectConfig*>(
        MapViewOfFile(g_shared_memory_handle, FILE_MAP_READ, 0, 0, sizeof(TSVoiceEffectConfig))
    );
    
    if (g_effect_config == nullptr) {
        CloseHandle(g_shared_memory_handle);
        g_shared_memory_handle = nullptr;
        CloseHandle(g_mutex_handle);
        g_mutex_handle = nullptr;
        return false;
    }
    
    // Open or create shared memory for status
    g_status_memory_handle = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, TS_STATUS_MEMORY_NAME);

    if (g_status_memory_handle == nullptr) {
        // Create it if it doesn't exist
        g_status_memory_handle = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            sizeof(TSPluginStatus),
            TS_STATUS_MEMORY_NAME
        );
        
        if (g_status_memory_handle == nullptr) {
            char msg[256];
            snprintf(msg, sizeof(msg), "KH Plugin: Could not create status shared memory, error: %lu", GetLastError());

            if (ts3Functions.logMessage) {
                ts3Functions.logMessage(msg, LogLevel_WARNING, PLUGIN_NAME, 0);
            }

            return true;  // Effects still work without status
        }
    }
    
    if (g_status_memory_handle != nullptr) {
        g_plugin_status = static_cast<TSPluginStatus*>(
            MapViewOfFile(g_status_memory_handle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(TSPluginStatus))
        );
        
        if (g_plugin_status == nullptr) {
            // Failed to map status memory - clean it up but continue
            CloseHandle(g_status_memory_handle);
            g_status_memory_handle = nullptr;
            
            // Log but don't fail - effects still work
            char msg[256];
            snprintf(msg, sizeof(msg), "KH Plugin: Could not map status shared memory, error: %lu", GetLastError());
            
            if (ts3Functions.logMessage) {
                ts3Functions.logMessage(msg, LogLevel_WARNING, PLUGIN_NAME, 0);
            }
        } else {
            if (WaitForSingleObject(g_mutex_handle, 100) == WAIT_OBJECT_0) {
                g_plugin_status->version = TS_IPC_VERSION;
                g_plugin_status->plugin_active = 1;
                g_plugin_status->sample_rate = g_audio_state.sample_rate;
                g_plugin_status->last_heartbeat = GetTickCount();
                ReleaseMutex(g_mutex_handle);
            }
        }
    }
    
    return true;
}

// Cleanup shared memory
static void cleanup_shared_memory() {
    if (g_plugin_status != nullptr) {
        if (g_mutex_handle != nullptr) {
            if (WaitForSingleObject(g_mutex_handle, 100) == WAIT_OBJECT_0) {
                g_plugin_status->plugin_active = 0;
                ReleaseMutex(g_mutex_handle);
            }
        }

        UnmapViewOfFile(g_plugin_status);
        g_plugin_status = nullptr;
    }
    
    if (g_effect_config != nullptr) {
        UnmapViewOfFile(g_effect_config);
        g_effect_config = nullptr;
    }
    
    if (g_shared_memory_handle != nullptr) {
        CloseHandle(g_shared_memory_handle);
        g_shared_memory_handle = nullptr;
    }
    
    if (g_status_memory_handle != nullptr) {
        CloseHandle(g_status_memory_handle);
        g_status_memory_handle = nullptr;
    }
    
    if (g_mutex_handle != nullptr) {
        CloseHandle(g_mutex_handle);
        g_mutex_handle = nullptr;
    }
}

// Update cached effects from shared memory
static void update_cached_effects() {
    if (g_effect_config == nullptr || g_mutex_handle == nullptr) {
        // Try to reconnect
        init_shared_memory();
        
        if (g_effect_config == nullptr || g_mutex_handle == nullptr) {
            return;
        }
    }

    if (WaitForSingleObject(g_mutex_handle, 0) == WAIT_OBJECT_0) {
        uint32_t current_seq = g_effect_config->sequence_number;
        
        if (current_seq != g_last_sequence) {
            uint32_t expected_checksum = calculate_checksum(g_effect_config);

            if (g_effect_config->checksum == expected_checksum && 
                g_effect_config->version == TS_IPC_VERSION) {
                std::lock_guard<std::mutex> lock(g_effect_mutex);
                memcpy(&g_cached_effects, g_effect_config, sizeof(TSVoiceEffectConfig));
                g_last_sequence = current_seq;
            }
        }
        
        ReleaseMutex(g_mutex_handle);
    }
}

static void update_plugin_status() {
    if (g_plugin_status == nullptr || g_mutex_handle == nullptr) return;
    
    if (WaitForSingleObject(g_mutex_handle, 10) == WAIT_OBJECT_0) {
        g_plugin_status->plugin_active = 1;
        g_plugin_status->connected = g_connected.load() ? 1 : 0;
        g_plugin_status->capturing = g_transmitting.load() ? 1 : 0;
        g_plugin_status->transmitting = g_transmitting.load() ? 1 : 0;
        g_plugin_status->sample_rate = g_audio_state.sample_rate;
        g_plugin_status->last_heartbeat = GetTickCount();
        ReleaseMutex(g_mutex_handle);
    }
}

static DWORD calculate_tail_duration_ms(const TSVoiceEffectConfig& effects) {
    DWORD max_tail_ms = 0;
    
    for (uint8_t i = 0; i < effects.effect_chain_count && i < TSVoiceEffectConfig::MAX_EFFECT_CHAIN; i++) {
        TSEffectType type = static_cast<TSEffectType>(effects.effect_chain_types[i]);
        float value = effects.effect_chain_values[i];
        
        switch (type) {
            case TSEffectType::ECHO_DELAY:
                // Echo can repeat multiple times based on decay
                // Estimate ~3-4 audible repeats
                max_tail_ms = std::max(max_tail_ms, static_cast<DWORD>(value * 1000.0f * 4));
                break;
            case TSEffectType::REVERB:
                // Reverb tail ~500ms at full amount
                max_tail_ms = std::max(max_tail_ms, static_cast<DWORD>(value * 500.0f));
                break;
            case TSEffectType::CHORUS:
            case TSEffectType::FLANGER:
                // Short tails ~50ms
                max_tail_ms = std::max(max_tail_ms, static_cast<DWORD>(50));
                break;
            default:
                break;
        }
    }
    
    return max_tail_ms;
}

static void start_tail_transmission(uint64 serverConnectionHandlerID) {
    TSVoiceEffectConfig effects;

    {
        std::lock_guard<std::mutex> lock(g_effect_mutex);
        effects = g_cached_effects;
    }
    
    if (!effects.effects_enabled) return;
    DWORD tail_ms = calculate_tail_duration_ms(effects);
    if (tail_ms == 0) return;
    g_current_server_connection_handler = serverConnectionHandlerID;
    g_tail_duration_ms.store(tail_ms);
    g_tail_start_time.store(GetTickCount());
    g_tail_active.store(true);
    ts3Functions.startVoiceRecording(serverConnectionHandlerID);
}

static void check_tail_completion() {
    if (!g_tail_active.load()) return;
    DWORD elapsed = GetTickCount() - g_tail_start_time.load();

    if (elapsed >= g_tail_duration_ms.load()) {
        g_tail_active.store(false);
        ts3Functions.stopVoiceRecording(g_current_server_connection_handler);
    }
}

static void process_audio(short* samples, int sample_count, int channels) {
    check_tail_completion();
    update_cached_effects();
    static std::atomic<DWORD> last_status_update{0};
    DWORD now = GetTickCount();

    if (now - last_status_update.load(std::memory_order_relaxed) > 100) {
        update_plugin_status();
        last_status_update.store(now, std::memory_order_relaxed);
    }
    
    // Get current effects
    TSVoiceEffectConfig effects;

    {
        std::lock_guard<std::mutex> lock(g_effect_mutex);
        effects = g_cached_effects;
    }

    std::lock_guard<std::mutex> audio_lock(g_audio_state_mutex);
    
    // Check if effects are enabled
    if (!effects.effects_enabled || effects.effect_chain_count == 0) return;
    
    // Pre-extract paired parameters
    float tremolo_depth_value = 0.5f;
    float echo_decay_value = 0.5f;
    float chorus_rate_value = 1.5f;
    float flanger_rate_value = 0.5f;
    float compressor_ratio_value = 4.0f;
    
    for (uint8_t j = 0; j < effects.effect_chain_count; j++) {
        TSEffectType t = static_cast<TSEffectType>(effects.effect_chain_types[j]);
        float v = effects.effect_chain_values[j];

        switch (t) {
            case TSEffectType::TREMOLO_DEPTH: tremolo_depth_value = v; break;
            case TSEffectType::ECHO_DECAY: echo_decay_value = v; break;
            case TSEffectType::CHORUS_RATE: chorus_rate_value = v; break;
            case TSEffectType::FLANGER_RATE: flanger_rate_value = v; break;
            case TSEffectType::COMPRESSOR_RATIO: compressor_ratio_value = v; break;
            default: break;
        }
    }
    
    // Process each sample
    for (int i = 0; i < sample_count; i++) {
        // Average all channels to mono for processing
        float mono_sample = 0.0f;
        
        for (int ch = 0; ch < channels; ch++) {
            mono_sample += static_cast<float>(samples[i * channels + ch]) / 32768.0f;
        }

        mono_sample /= static_cast<float>(channels);
        
        // Apply effects in user-specified order (on mono signal)
        for (uint8_t fx_idx = 0; fx_idx < effects.effect_chain_count && fx_idx < TSVoiceEffectConfig::MAX_EFFECT_CHAIN; fx_idx++) {
            TSEffectType effect_type = static_cast<TSEffectType>(effects.effect_chain_types[fx_idx]);
            float value = effects.effect_chain_values[fx_idx];
            EffectSlotState& slot = g_audio_state.slots[fx_idx];
            float sample = mono_sample;
                
            switch (effect_type) {
                case TSEffectType::PITCH_SHIFT:
                    if (std::abs(value) > 0.01f) {
                        sample = AudioEffects::apply_pitch_shift(sample, value, slot, g_audio_state.sample_rate);
                    }
                    
                    break;

                case TSEffectType::DISTORTION:
                    if (value > 0.0f) {
                        sample = AudioEffects::distort(sample, value);
                    }

                    break;

                case TSEffectType::BITCRUSH:
                    if (value > 0.0f) {
                        sample = AudioEffects::bitcrush(sample, value);
                    }

                    break;

                case TSEffectType::RING_MOD:
                    if (value > 0.0f) {
                        sample = AudioEffects::ring_mod(sample, slot.ring_mod_phase, value, g_audio_state.sample_rate);
                    }
                    
                    break;

                case TSEffectType::TREMOLO_RATE:
                    if (value > 0.0f) {
                        sample = AudioEffects::apply_tremolo(sample, slot.tremolo_phase, value, tremolo_depth_value, g_audio_state.sample_rate);
                    }

                    break;

                case TSEffectType::CHORUS:
                    if (value > 0.0f) {
                        sample = AudioEffects::apply_chorus(sample, value, chorus_rate_value, slot, g_audio_state.sample_rate);
                    }

                    break;

                case TSEffectType::FLANGER:
                    if (value > 0.0f) {
                        sample = AudioEffects::apply_flanger(sample, value, flanger_rate_value, slot, g_audio_state.sample_rate);
                    }

                    break;

                case TSEffectType::NOISE:
                    if (value > 0.0f) {
                        sample = AudioEffects::add_noise(sample, value, g_audio_state.rng_state);
                    }

                    break;

                case TSEffectType::RADIO_STATIC:
                    if (value > 0.0f) {
                        sample = AudioEffects::add_radio_static(sample, value, g_audio_state.rng_state);
                    }

                    break;

                case TSEffectType::REVERB:
                    if (value > 0.0f) {
                        sample = AudioEffects::apply_reverb(sample, value, slot);
                    }

                    break;

                case TSEffectType::LOWPASS:
                    if (value > 0.0f && value < 1.0f) {
                        sample = AudioEffects::lowpass(sample, value, slot.lp_prev, g_audio_state.sample_rate);
                    }

                    break;

                case TSEffectType::HIGHPASS:
                    if (value > 0.0f) {
                        sample = AudioEffects::highpass(sample, value, slot.hp_prev_in, slot.hp_prev_out, g_audio_state.sample_rate);
                    }

                    break;

                case TSEffectType::TELEPHONE:
                    if (value > 0.0f) {
                        sample = AudioEffects::apply_telephone_filter(sample, value, slot.tel_lp_prev, slot.tel_hp_prev_in, slot.tel_hp_prev_out, g_audio_state.sample_rate);
                    }

                    break;

                case TSEffectType::UNDERWATER:
                    if (value > 0.0f) {
                        sample = AudioEffects::apply_underwater(sample, value, slot.uw_lp_prev1, slot.uw_lp_prev2, g_audio_state.sample_rate);
                    }

                    break;

                case TSEffectType::ECHO_DELAY:
                    if (value > 0.0f) {
                        AudioEffects::update_echo_buffer(value, g_audio_state.sample_rate, slot);
                        sample = AudioEffects::apply_echo(sample, value, echo_decay_value, slot);
                    }

                    break;

                case TSEffectType::RADIO_SQUELCH:
                    if (value > 0.0f) {
                        sample = AudioEffects::apply_squelch(sample, value, slot.squelch_state);
                    }

                    break;

                case TSEffectType::FREQUENCY_WOBBLE:
                    if (value > 0.0f) {
                        sample = AudioEffects::apply_frequency_wobble(sample, slot.wobble_phase, value, g_audio_state.sample_rate);
                    }

                    break;

                case TSEffectType::COMPRESSOR_THRESHOLD:
                    if (value > 0.0f && compressor_ratio_value > 1.0f) {
                        sample = AudioEffects::apply_compressor(sample, value, compressor_ratio_value, slot.compressor_envelope, g_audio_state.sample_rate);
                    }

                    break;

                case TSEffectType::AGC:
                    if (value > 0.0f) {
                        sample = AudioEffects::apply_agc(sample, value, slot.agc_gain, slot.agc_envelope, g_audio_state.sample_rate);
                    }

                    break;

                case TSEffectType::BASS_BOOST:
                    if (value > 0.0f) {
                        sample = AudioEffects::apply_bass_boost(sample, value, slot.bass_lp_prev1, slot.bass_lp_prev2, g_audio_state.sample_rate);
                    }

                    break;

                case TSEffectType::VOLUME:
                    sample *= value;
                    break;

                default:
                    break;
            }
            
            mono_sample = sample;
        }
        
        // Clamp and apply processed mono to all channels
        mono_sample = std::clamp(mono_sample, -1.0f, 1.0f);
        short output_sample = static_cast<short>(mono_sample * 32767.0f);
        
        for (int ch = 0; ch < channels; ch++) {
            samples[i * channels + ch] = output_sample;
        }
    }

    if (g_local_test_mode.load(std::memory_order_relaxed)) {
        std::lock_guard<std::mutex> lock(g_loopback_mutex);
        
        for (int i = 0; i < sample_count; i++) {
            // Average channels to mono for loopback
            float mono_sample = 0.0f;

            for (int ch = 0; ch < channels; ch++) {
                mono_sample += static_cast<float>(samples[i * channels + ch]) / 32768.0f;
            }

            mono_sample /= static_cast<float>(channels);
            g_loopback_buffer[g_loopback_write_pos] = mono_sample;
            g_loopback_write_pos = (g_loopback_write_pos + 1) % LOOPBACK_BUFFER_SIZE;
        }
        
        size_t new_available = g_loopback_available.load() + sample_count;

        if (new_available > LOOPBACK_BUFFER_SIZE) {
            new_available = LOOPBACK_BUFFER_SIZE;
            g_loopback_read_pos = (g_loopback_write_pos + 1) % LOOPBACK_BUFFER_SIZE;
        }

        g_loopback_available.store(new_available);
    }
}

extern "C" {

// Required plugin functions
const char* ts3plugin_name() {
    return PLUGIN_NAME;
}

const char* ts3plugin_version() {
    return PLUGIN_VERSION;
}

int ts3plugin_apiVersion() {
    return PLUGIN_API_VERSION;
}

const char* ts3plugin_author() {
    return PLUGIN_AUTHOR;
}

const char* ts3plugin_description() {
    return PLUGIN_DESCRIPTION;
}

void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

int ts3plugin_init() {
    g_audio_state.init(48000);

    {
        std::lock_guard<std::mutex> lock(g_loopback_mutex);
        g_loopback_buffer.resize(LOOPBACK_BUFFER_SIZE, 0.0f);
        g_loopback_write_pos = 0;
        g_loopback_read_pos = 0;
        g_loopback_available.store(0);
    }
    
    init_shared_memory();
    g_plugin_initialized.store(true);
    char msg[256];
    snprintf(msg, sizeof(msg), "%s initialized", PLUGIN_NAME);
    ts3Functions.logMessage(msg, LogLevel_INFO, PLUGIN_NAME, 0);
    return 0;
}

void ts3plugin_shutdown() {
    g_plugin_initialized.store(false);
    cleanup_shared_memory();
    char msg[256];
    snprintf(msg, sizeof(msg), "%s shutdown", PLUGIN_NAME);
    ts3Functions.logMessage(msg, LogLevel_INFO, PLUGIN_NAME, 0);
}

int ts3plugin_offersConfigure() {
    return PLUGIN_OFFERS_NO_CONFIGURE;
}

// Connection events
void ts3plugin_onConnectStatusChangeEvent(uint64 serverConnectionHandlerID, int newStatus, unsigned int errorNumber) {
    g_connected.store(newStatus == STATUS_CONNECTION_ESTABLISHED);
    
    if (newStatus == STATUS_CONNECTION_ESTABLISHED) {
        g_current_server_connection_handler = serverConnectionHandlerID;
        
        // Reset audio state on new connection
        {
            std::lock_guard<std::mutex> lock(g_audio_state_mutex);
            g_audio_state.reset();
        }
    } else if (newStatus == STATUS_DISCONNECTED) {
        // Clean up tail state on disconnect
        g_tail_active.store(false);
    }
    
    update_plugin_status();
}

// Voice processing - this is called for outgoing voice data
void ts3plugin_onEditCapturedVoiceDataEvent(uint64 serverConnectionHandlerID, short* samples, int sampleCount, int channels, int* edited) {
    if (!g_plugin_initialized.load()) return;
    
    // Process the audio
    process_audio(samples, sampleCount, channels);
    
    // Indicate we modified the data
    *edited |= 0x1;
}

// Called when starting to talk
void ts3plugin_onTalkStatusChangeEvent(uint64 serverConnectionHandlerID, int status, int isReceivedWhisper, anyID clientID) {
    // Check if this is the local client
    anyID myID;
    
    if (ts3Functions.getClientID(serverConnectionHandlerID, &myID) == ERROR_ok) {
        if (clientID == myID) {
            bool was_transmitting = g_transmitting.load();
            bool now_talking = (status == STATUS_TALKING);
            g_transmitting.store(now_talking);
            
            if (now_talking) {
                // User started talking - cancel any active tail
                if (g_tail_active.load()) {
                    g_tail_active.store(false);
                    ts3Functions.stopVoiceRecording(serverConnectionHandlerID);
                }
                
                // Reset some effect states on new transmission
                std::lock_guard<std::mutex> lock(g_audio_state_mutex);

                for (size_t i = 0; i < AudioProcessorState::MAX_SLOTS; i++) {
                    g_audio_state.slots[i].ring_mod_phase = 0.0f;
                    g_audio_state.slots[i].wobble_phase = 0.0f;
                }
            } else if (was_transmitting && !g_tail_active.load()) {
                // User stopped talking naturally - start tail if needed
                start_tail_transmission(serverConnectionHandlerID);
            }
            
            update_plugin_status();
        }
    }
}

void ts3plugin_onEditMixedPlaybackVoiceDataEvent(uint64 serverConnectionHandlerID, short* samples, int sampleCount, int channels, const unsigned int* channelSpeakerArray, unsigned int* channelFillMask) {
    if (!g_plugin_initialized.load() || !g_local_test_mode.load(std::memory_order_relaxed)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(g_loopback_mutex);
    size_t available = g_loopback_available.load(std::memory_order_acquire);

    if (available == 0) {
        return;
    }

    size_t samples_to_read = std::min(static_cast<size_t>(sampleCount), available);
    
    for (size_t i = 0; i < samples_to_read; i++) {
        float loopback_sample = g_loopback_buffer[g_loopback_read_pos];
        g_loopback_read_pos = (g_loopback_read_pos + 1) % LOOPBACK_BUFFER_SIZE;
        
        // Mix into all channels
        for (int ch = 0; ch < channels; ch++) {
            int idx = static_cast<int>(i) * channels + ch;
            float existing = static_cast<float>(samples[idx]) / 32768.0f;
            float mixed = existing + loopback_sample * 0.7f;
            mixed = std::clamp(mixed, -1.0f, 1.0f);
            samples[idx] = static_cast<short>(mixed * 32767.0f);
        }
    }
    
    g_loopback_available.store(available - samples_to_read, std::memory_order_release);
}

// Provide plugin commands
// REPLACE LINES 1775-1804 WITH:
int ts3plugin_processCommand(uint64 serverConnectionHandlerID, const char* command) {
    char msg[2048];
    char debug[256];
    snprintf(debug, sizeof(debug), "KH Plugin received command: '%s'", command);
    ts3Functions.logMessage(debug, LogLevel_INFO, PLUGIN_NAME, 0);
    
    if (strcmp(command, "status") == 0) {
        bool shm_connected = (g_effect_config != nullptr);
        bool effects_active = g_cached_effects.effects_enabled != 0;
        bool test_mode = g_local_test_mode.load();
        
        int offset = snprintf(msg, sizeof(msg), 
            "[b]KH Voice Modulation Status:[/b]\n"
            "IPC Connected: %s\n"
            "Effects Active: %s\n"
            "Local Test Mode: %s\n",
            shm_connected ? "Yes" : "No",
            effects_active ? "Yes" : "No",
            test_mode ? "Enabled" : "Disabled"
        );
        
        if (effects_active && g_cached_effects.effect_chain_count > 0) {
            offset += snprintf(msg + offset, sizeof(msg) - offset, "\n[b]Effect Chain (%d effects):[/b]\n", g_cached_effects.effect_chain_count);
            
            for (uint8_t i = 0; i < g_cached_effects.effect_chain_count && i < TSVoiceEffectConfig::MAX_EFFECT_CHAIN; i++) {
                TSEffectType type = static_cast<TSEffectType>(g_cached_effects.effect_chain_types[i]);
                float value = g_cached_effects.effect_chain_values[i];
                offset += snprintf(msg + offset, sizeof(msg) - offset, "  %d. %s: %.3f\n", i + 1, get_effect_name(type), value);
                if (offset >= static_cast<int>(sizeof(msg) - 64)) break;
            }
        } else if (effects_active) {
            offset += snprintf(msg + offset, sizeof(msg) - offset, "\nNo effects in chain.\n");
        }
        
        ts3Functions.printMessage(serverConnectionHandlerID, msg, PLUGIN_MESSAGE_TARGET_SERVER);
        ts3Functions.logMessage(msg, LogLevel_INFO, PLUGIN_NAME, 0);
        return 0;
    }
    
    if (strcmp(command, "test") == 0) {
        bool current = g_local_test_mode.load();
        bool new_state = !current;

        {
            std::lock_guard<std::mutex> lock(g_loopback_mutex);
            g_loopback_write_pos = 0;
            g_loopback_read_pos = 0;
            g_loopback_available.store(0);
            std::fill(g_loopback_buffer.begin(), g_loopback_buffer.end(), 0.0f);
        }
        
        g_local_test_mode.store(new_state);
        
        snprintf(msg, sizeof(msg), "[b]KH Voice Modulation:[/b] Local test mode %s. You will %shear your own voice with effects applied.",
            new_state ? "enabled" : "disabled",
            new_state ? "" : "no longer ");

        ts3Functions.printMessage(serverConnectionHandlerID, msg, PLUGIN_MESSAGE_TARGET_SERVER);
        ts3Functions.logMessage(new_state ? "Local test mode enabled" : "Local test mode disabled", LogLevel_INFO, PLUGIN_NAME, 0);
        return 0;
    }
    
    if (strcmp(command, "help") == 0) {
        snprintf(msg, sizeof(msg),
            "[b]KH Voice Modulation Commands:[/b]\n"
            "  /kh status - Show connection status and active effects\n"
            "  /kh test - Toggle local test mode (hear your own filtered voice)\n"
            "  /kh help - Show this help message"
        );

        ts3Functions.printMessage(serverConnectionHandlerID, msg, PLUGIN_MESSAGE_TARGET_SERVER);
        return 0;
    }
    
    snprintf(msg, sizeof(msg), "[b]KH Voice Modulation:[/b] Unknown command '%s'. Use '/kh help' for available commands.", command);
    ts3Functions.printMessage(serverConnectionHandlerID, msg, PLUGIN_MESSAGE_TARGET_SERVER);
    return 0;
}

const char* ts3plugin_commandKeyword() {
    return "kh";
}

int ts3plugin_requestAutoload() {
    return 1;  // Return 1 to request autoload
}

} // extern "C"