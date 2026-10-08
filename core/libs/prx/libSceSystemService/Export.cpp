#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include "prx/libc/include/Shutdown.hpp"
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libSceSystemService/SystemService.hpp"

namespace {

struct LanguageCode {
    const char* code;
    int id;
};

constexpr LanguageCode Languages[] = {
    {"ja", 0}, {"ja-JP", 0}, {"en", 1}, {"en-US", 1}, {"fr", 2}, {"fr-FR", 2}, {"es", 3}, {"es-ES", 3},
    {"de", 4}, {"de-DE", 4}, {"it", 5}, {"it-IT", 5}, {"nl", 6}, {"nl-NL", 6}, {"pt", 7}, {"pt-PT", 7},
    {"ru", 8}, {"ru-RU", 8}, {"ko", 9}, {"ko-KR", 9}, {"zh-TW", 10}, {"zh-Hant", 10}, {"zh", 11}, {"zh-CN", 11},
    {"zh-Hans", 11}, {"fi", 12}, {"fi-FI", 12}, {"sv", 13}, {"sv-SE", 13}, {"da", 14}, {"da-DK", 14},
    {"nb", 15}, {"no", 15}, {"nb-NO", 15}, {"pl", 16}, {"pl-PL", 16}, {"pt-BR", 17}, {"en-GB", 18},
    {"tr", 19}, {"tr-TR", 19}, {"es-419", 20}, {"es-LA", 20}, {"ar", 21}, {"fr-CA", 22}, {"cs", 23},
    {"cs-CZ", 23}, {"hu", 24}, {"hu-HU", 24}, {"el", 25}, {"el-GR", 25}, {"ro", 26}, {"ro-RO", 26},
    {"th", 27}, {"th-TH", 27}, {"vi", 28}, {"vi-VN", 28}, {"id", 29}, {"id-ID", 29}, {"uk", 30}, {"uk-UA", 30},
};

bool SameCode(const char* left, const char* right) {
    for (; *left != '\0' && *right != '\0'; ++left, ++right) {
        const char a = *left == '_' ? '-' : (*left >= 'A' && *left <= 'Z' ? static_cast<char>(*left - 'A' + 'a') : *left);
        const char b = *right >= 'A' && *right <= 'Z' ? static_cast<char>(*right - 'A' + 'a') : *right;
        if (a != b) return false;
    }
    return *left == '\0' && *right == '\0';
}

int SystemLanguage() {
    const char* value = std::getenv("ANYPS5_LANGUAGE");
    if (value == nullptr || *value == '\0') return SYSTEM_SERVICE_PARAM_LANG_ENGLISH_US;
    for (const auto& language : Languages) {
        if (SameCode(value, language.code)) return language.id;
    }
    char* end = nullptr;
    const long id = std::strtol(value, &end, 10);
    if (*end == '\0' && id >= 0 && id <= 30) return static_cast<int>(id);
    throw std::runtime_error(std::string("ANYPS5_LANGUAGE: unknown language '") + value + "'");
}

}

extern "C" {

int APS5_VABI sceSystemServiceLoadExec(const char* path, const char* const* arguments) {
    if (!path || !*path) return SYSTEM_SERVICE_ERROR_PARAMETER;
    if (std::strcmp(path, "exit") != 0) {
        NotImplemented_nid_no_patch("sceSystemServiceLoadExec: executable replacement");
    }
    (void)arguments;
    LibcRunShutdown_nid_postfix();
    std::exit(0);
}

int APS5_VABI sceSystemServiceDisableNoticeScreenSkipFlagAutoSet(void) {
 return SYSTEM_SERVICE_OK;
}

int APS5_VABI sceSystemServiceGetDisplaySafeAreaInfo(SystemServiceDisplaySafeAreaInfo* info) {
 if (info == nullptr) {
  return SYSTEM_SERVICE_ERROR_PARAMETER;
 }
 *info = SystemServiceDisplaySafeAreaInfo{};
 info->ratio = 1.0f;
 return SYSTEM_SERVICE_OK;
}

int APS5_VABI sceSystemServiceGetHdrToneMapLuminance(SystemServiceHdrToneMapLuminance* luminance) {
 if (luminance == nullptr) {
  return SYSTEM_SERVICE_ERROR_PARAMETER;
 }
 constexpr float SdrReferenceWhiteNits = 100.0f;
 luminance->max_full_frame_tone_map_luminance = SdrReferenceWhiteNits;
 luminance->max_tone_map_luminance = SdrReferenceWhiteNits;
 luminance->min_tone_map_luminance = 0.0f;
 return SYSTEM_SERVICE_OK;
}

int APS5_VABI sceSystemServiceGetNoticeScreenSkipFlag(bool* value) {
 if (value == nullptr) {
  return SYSTEM_SERVICE_ERROR_PARAMETER;
 }
 *value = false;
 return SYSTEM_SERVICE_OK;
}

int APS5_VABI sceSystemServiceGetStatus(SystemServiceStatus* status) {
 if (status == nullptr) {
  return SYSTEM_SERVICE_ERROR_PARAMETER;
 }
 *status = SystemServiceStatus{};
 return SYSTEM_SERVICE_OK;
}

int APS5_VABI sceSystemServiceHideSplashScreen(void) {
 return SYSTEM_SERVICE_OK;
}

int APS5_VABI sceSystemServiceParamGetInt(int paramId, int* value) {
 if (value == nullptr) {
  return SYSTEM_SERVICE_ERROR_PARAMETER;
 }
 switch (paramId) {
  case SYSTEM_SERVICE_PARAM_ID_LANG: *value = SystemLanguage(); break;
  case SYSTEM_SERVICE_PARAM_ID_DATE_FORMAT: *value = SYSTEM_SERVICE_PARAM_DATE_FORMAT_DDMMYYYY; break;
  case SYSTEM_SERVICE_PARAM_ID_TIME_FORMAT: *value = SYSTEM_SERVICE_PARAM_TIME_FORMAT_24HOUR; break;
  case SYSTEM_SERVICE_PARAM_ID_TIME_ZONE: *value = 0; break;
  case SYSTEM_SERVICE_PARAM_ID_SUMMERTIME: *value = 0; break;
  case SYSTEM_SERVICE_PARAM_ID_GAME_PARENTAL_LEVEL: *value = SYSTEM_SERVICE_PARAM_GAME_PARENTAL_OFF; break;
  case SYSTEM_SERVICE_PARAM_ID_ENTER_BUTTON_ASSIGN: *value = SYSTEM_SERVICE_PARAM_ENTER_BUTTON_CROSS; break;
  default: *value = 0; break;
 }
 return SYSTEM_SERVICE_OK;
}

int APS5_VABI sceSystemServiceParamGetString(int paramId, char* buf, size_t bufSize) {
 if (buf == nullptr || bufSize == 0) {
  return SYSTEM_SERVICE_ERROR_PARAMETER;
 }
 if (paramId != SYSTEM_SERVICE_PARAM_ID_SYSTEM_NAME) {
  NotImplemented_nid_no_patch("sceSystemServiceParamGetString: parameter other than the system name");
 }
 if (bufSize < SYSTEM_SERVICE_MAX_SYSTEM_NAME_LENGTH) {
  NotImplemented_nid_no_patch("sceSystemServiceParamGetString: buffer shorter than 65 bytes");
 }
 constexpr char SystemName[] = "PS5";
 std::memcpy(buf, SystemName, sizeof(SystemName));
 return SYSTEM_SERVICE_OK;
}

int APS5_VABI sceSystemServicePowerTick(void) {
 return SYSTEM_SERVICE_OK;
}

int APS5_VABI sceSystemServiceReceiveEvent(SystemServiceEvent* event) {
 if (event == nullptr) {
  return SYSTEM_SERVICE_ERROR_PARAMETER;
 }
 return SYSTEM_SERVICE_ERROR_NO_EVENT;
}

int APS5_VABI sceSystemServiceReportAbnormalTermination(const void* info) {
 (void)info;
 return SYSTEM_SERVICE_OK;
}

int APS5_VABI sceSystemServiceSetNoticeScreenSkipFlag(void) {
 return SYSTEM_SERVICE_OK;
}

int APS5_VABI sceSystemServiceInitializePlayerDialogParam(void* param) {
 if (param == nullptr) return SYSTEM_SERVICE_ERROR_PARAMETER;
 return SYSTEM_SERVICE_OK;
}

int APS5_VABI sceSystemServiceDisableMediaPlay() {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceSystemServiceReenableMediaPlay() {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceSystemServiceLaunchPlayerDialog(const void* param) {
 if (param == nullptr) return SYSTEM_SERVICE_ERROR_PARAMETER;
 return SYSTEM_SERVICE_OK;
}

int APS5_VABI sceSystemServiceDisableMusicPlayer(void) {
 return SYSTEM_SERVICE_OK;
}

int APS5_VABI sceSystemServiceOpenChallengeActivity(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceSystemServiceOpenTournamentOccurrence(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceSystemServiceReenableMusicPlayer(void) {
 return SYSTEM_SERVICE_OK;
}

int APS5_VABI sceSystemServiceShowControllerSettings(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

}
