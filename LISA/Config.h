/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Liberty Global Service B.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <string>

namespace WPEFramework {
namespace Plugin {
namespace LISA {

const std::string DAC_CONFIG_MIMETYPE{"application/LISA"};
const std::string DAC_CONFIG_APP_ID{"lisa.dac.config"};
const std::string DAC_CONFIG_APP_VERSION{"0"};
const std::string DACBUNDLEPLATFORMNAMEOVERRIDE_KEY_NAME{"dacBundlePlatformNameOverride"};
const std::string DACBUNDLEFIRMWARECOMPATIBILITYKEY_KEY_NAME{"dacBundleFirmwareCompatibilityKey"};
const std::string ASMS_URL_KEY_NAME{"asmsUrl"};

class Config
{
public:
    Config() = default;
    Config(const std::string& aConfig);

    const std::string& getDatabasePath() const;
    const std::string& getAppsTmpPath() const;
    const std::string& getAppsPath() const;
    const std::string& getAppsStoragePath() const;
    const std::string& getAnnotationsFile() const;
    const std::string& getAnnotationsRegex() const;
    unsigned int getDownloadRetryAfterSeconds() const;
    unsigned int getDownloadRetryMaxTimes() const;
    unsigned int getDownloadTimeoutSeconds() const;
    const std::string& getDacBundlePlatformNameOverride() const;
    const std::string& getDacBundleFirmwareCompatibilityKey() const;
    const std::string& getAsmsUrl() const;

    friend std::ostream& operator<<(std::ostream& out, const Config& config);

private:
    std::string databasePath{"/mnt/apps/dac/db/"};
    std::string appsPath{"/mnt/apps/dac/images/"};
    std::string appsTmpPath{"/mnt/apps/dac/images/tmp/"};
    std::string appsStoragePath{"/mnt/data/dac/"};
    std::string annotationsFile;
    std::string annotationsRegex;
    unsigned int downloadRetryAfterSeconds{30};
    unsigned int downloadRetryMaxTimes{4};
    unsigned int downloadTimeoutSeconds{15 * 60};
    std::string dacBundlePlatformNameOverride;
    std::string dacBundleFirmwareCompatibilityKey;
    std::string asmsUrl;
};

} // namespace LISA
} // namespace Plugin
} // namespace WPEFramework

