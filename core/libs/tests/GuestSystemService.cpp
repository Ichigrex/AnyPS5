#include "SceTypes.hpp"
#include "prx/libSceSystemService/SystemService.hpp"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

extern "C" int APS5_VABI sceSystemServiceGetHdrToneMapLuminance(SystemServiceHdrToneMapLuminance* luminance);
extern "C" int APS5_VABI sceSystemServiceParamGetInt(int paramId, int* value);
extern "C" int APS5_VABI sceSystemServiceParamGetString(int paramId, char* buf, std::size_t bufSize);

namespace {

void Require(bool value) { if (!value) std::abort(); }

bool ParamGetStringThrows(int paramId, char* buf, std::size_t bufSize) {
    try {
        static_cast<void>(sceSystemServiceParamGetString(paramId, buf, bufSize));
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

int LanguageFor(const char* value) {
#ifdef _WIN32
    _putenv_s("ANYPS5_LANGUAGE", value);
#else
    setenv("ANYPS5_LANGUAGE", value, 1);
#endif
    int language = -1;
    Require(sceSystemServiceParamGetInt(SYSTEM_SERVICE_PARAM_ID_LANG, &language) == SYSTEM_SERVICE_OK);
    return language;
}

bool LanguageThrows(const char* value) {
    try {
        static_cast<void>(LanguageFor(value));
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

}

extern "C" int APS5_VABI sceSystemServicePowerTick(void);
extern "C" int APS5_VABI sceSystemServiceReportAbnormalTermination(const void* info);
extern "C" int APS5_VABI sceSystemServiceDisableMusicPlayer(void);
extern "C" int APS5_VABI sceSystemServiceReenableMusicPlayer(void);

int main() {
    Require(sceSystemServicePowerTick() == SYSTEM_SERVICE_OK);
    Require(sceSystemServicePowerTick() == SYSTEM_SERVICE_OK);
    Require(sceSystemServiceReportAbnormalTermination(nullptr) == SYSTEM_SERVICE_OK);
    int info = 0;
    Require(sceSystemServiceReportAbnormalTermination(&info) == SYSTEM_SERVICE_OK);
    Require(sceSystemServiceDisableMusicPlayer() == SYSTEM_SERVICE_OK);
    Require(sceSystemServiceDisableMusicPlayer() == SYSTEM_SERVICE_OK);
    Require(sceSystemServiceReenableMusicPlayer() == SYSTEM_SERVICE_OK);
    Require(sceSystemServiceReenableMusicPlayer() == SYSTEM_SERVICE_OK);
    Require(sceSystemServiceGetHdrToneMapLuminance(nullptr) == SYSTEM_SERVICE_ERROR_PARAMETER);
    SystemServiceHdrToneMapLuminance luminance{-1.0f, -1.0f, -1.0f};
    Require(sceSystemServiceGetHdrToneMapLuminance(&luminance) == SYSTEM_SERVICE_OK);
    Require(luminance.max_full_frame_tone_map_luminance == 100.0f);
    Require(luminance.max_tone_map_luminance == 100.0f);
    Require(luminance.min_tone_map_luminance == 0.0f);

    char name[SYSTEM_SERVICE_MAX_SYSTEM_NAME_LENGTH];
    std::memset(name, 'x', sizeof(name));
    Require(sceSystemServiceParamGetString(SYSTEM_SERVICE_PARAM_ID_SYSTEM_NAME, nullptr, sizeof(name)) == SYSTEM_SERVICE_ERROR_PARAMETER);
    Require(sceSystemServiceParamGetString(SYSTEM_SERVICE_PARAM_ID_SYSTEM_NAME, name, 0) == SYSTEM_SERVICE_ERROR_PARAMETER);
    Require(ParamGetStringThrows(SYSTEM_SERVICE_PARAM_ID_SYSTEM_NAME, name, sizeof(name) - 1));
    Require(ParamGetStringThrows(SYSTEM_SERVICE_PARAM_ID_LANG, name, sizeof(name)));
    Require(name[0] == 'x');
    Require(sceSystemServiceParamGetString(SYSTEM_SERVICE_PARAM_ID_SYSTEM_NAME, name, sizeof(name)) == SYSTEM_SERVICE_OK);
    Require(std::strcmp(name, "PS5") == 0);

    Require(LanguageFor("") == 1);
    Require(LanguageFor("fr") == 2 && LanguageFor("fr_FR") == 2 && LanguageFor("FR-fr") == 2);
    Require(LanguageFor("fr-CA") == 22 && LanguageFor("en-GB") == 18 && LanguageFor("ja") == 0);
    Require(LanguageFor("pt-BR") == 17 && LanguageFor("es-419") == 20 && LanguageFor("uk") == 30);
    Require(LanguageFor("4") == 4 && LanguageFor("30") == 30);
    Require(LanguageThrows("klingon") && LanguageThrows("31") && LanguageThrows("-1") && LanguageThrows("2x"));
}
