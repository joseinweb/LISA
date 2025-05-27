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

#include "Debug.h"
#include "Executor.h"
#include "Filesystem.h"
#include "DataStorage.h"

#include <interfaces/ILISA.h>
#include <string>
#include <memory>
#include <mutex>
#include <map>

namespace WPEFramework {
namespace Plugin {

class LISAImplementation : public Exchange::ILISA {
public:
    LISAImplementation() = default;
    LISAImplementation(const LISAImplementation&) = delete;
    LISAImplementation& operator= (const LISAImplementation&) = delete;

    virtual ~LISAImplementation()
    {
    }

    Core::hresult Install(const std::string& type,
            const std::string& id,
            const std::string& version,
            const std::string& url,
            const std::string& appName,
            const std::string& category,
            std::string& handle /* @out */) override
    {
        return static_cast<Core::hresult>(executor.Install(type, id, version, url, appName, category, handle));
    }

    Core::hresult Uninstall(const std::string& type,
            const std::string& id,
            const std::string& version,
            const std::string& uninstallType,
            std::string& handle /* @out */) override
    {
        return static_cast<Core::hresult>(executor.Uninstall(type, id, version, uninstallType, handle));
    }

    class HandleResultImpl : public ILISA::IHandleResult
    {
    public:
        HandleResultImpl() = delete;
        HandleResultImpl(const HandleResultImpl&) = delete;
        HandleResultImpl& operator=(const HandleResultImpl&) = delete;

        HandleResultImpl(const std::string handle)
                : _handle(handle)
        {
        }

        ~HandleResultImpl() override
        {
        }

        Core::hresult Handle(std::string& handle) const override
        {
            handle = _handle;
            return Core::ERROR_NONE;
        }

    private:
        std::string _handle;

    public:
        BEGIN_INTERFACE_MAP(HandleResultImpl)
        INTERFACE_ENTRY(Exchange::ILISA::IHandleResult)
        END_INTERFACE_MAP
    }; // class HandleResultImpl

    Core::hresult Lock(const std::string& type,
                  const std::string& id,
                  const std::string& version,
                  const std::string& reason,
                  const std::string& owner,
                  ILISA::IHandleResult*& result  /* @out */) override
    {
        std::string handle;
        auto error = executor.Lock(type, id, version, reason, owner, handle);
        result = Core::Service<HandleResultImpl>::Create<ILISA::IHandleResult>(
                handle
        );
        return static_cast<Core::hresult>(error);
    }

    Core::hresult Unlock(const std::string& handle) override
    {
        return static_cast<Core::hresult>(executor.Unlock(handle));
    }

    class LockInfoImpl : public ILISA::ILockInfo
    {
    public:
        LockInfoImpl() = delete;
        LockInfoImpl(const LockInfoImpl&) = delete;
        LockInfoImpl& operator=(const LockInfoImpl&) = delete;

        LockInfoImpl(const std::string reason, const std::string owner)
                : _reason(reason), _owner(owner)
        {
        }

        ~LockInfoImpl() override
        {
        }

        Core::hresult Reason(std::string& reason) const override
        {
            reason = _reason;
            return Core::ERROR_NONE;
        }

        Core::hresult Owner(std::string& owner) const override
        {
            owner = _owner;
            return Core::ERROR_NONE;
        }

    private:
        std::string _reason;
        std::string _owner;

    public:
        BEGIN_INTERFACE_MAP(LockInfoImpl)
        INTERFACE_ENTRY(Exchange::ILISA::ILockInfo)
        END_INTERFACE_MAP
    }; // class LockInfoImpl

    Core::hresult GetLockInfo(const std::string& type,
                         const std::string& id,
                         const std::string& version,
                         ILISA::ILockInfo*& result) override
    {
        std::string reason, owner;
        auto error = executor.GetLockInfo(type, id, version, reason, owner);
        result = Core::Service<LockInfoImpl>::Create<ILISA::ILockInfo>(
                reason, owner
        );
        return static_cast<Core::hresult>(error);
    }

    Core::hresult Download(const std::string& type,
            const std::string& id,
            const std::string& version,
            const std::string& resKey,
            const std::string& url,
            std::string& handle /* @out */) override
    {
        handle = "Download";
        return Core::ERROR_NONE;
    }

    Core::hresult Reset(const std::string& type,
            const std::string& id,
            const std::string& version,
            const std::string& resetType) override
    {
        return Core::ERROR_NONE;
    }

    class StorageImpl : public ILISA::IStorage
    {
    public:
        StorageImpl() = delete;
        StorageImpl(const StorageImpl&) = delete;
        StorageImpl& operator=(const StorageImpl&) = delete;

        StorageImpl(
                const std::string& path,
                const std::string& quota,
                const std::string& usedKB) :
                _path(path), _quota(quota), _usedKB(usedKB)
        {
        }

        ~StorageImpl() override
        {
        }

        Core::hresult Path(string& path) const override {
            path = _path;
            return Core::ERROR_NONE;
        }

        Core::hresult QuotaKB(string& quota) const override {
            quota = _quota;
            return Core::ERROR_NONE;
        }

        Core::hresult UsedKB(string& usedKB) const override {
            usedKB = _usedKB;
            return Core::ERROR_NONE;
        }
    private:
        std::string _path;
        std::string _quota;
        std::string _usedKB;

    public:
        BEGIN_INTERFACE_MAP(StorageImpl)
            INTERFACE_ENTRY(Exchange::ILISA::IStorage)
        END_INTERFACE_MAP
    }; // StorageImpl

    class StoragePayloadImpl : public ILISA::IStoragePayload
    {
    public:
        StoragePayloadImpl() = delete;
        StoragePayloadImpl(const StoragePayloadImpl&) = delete;
        StoragePayloadImpl& operator=(const StoragePayloadImpl&) = delete;

        explicit StoragePayloadImpl(const LISA::Filesystem::StorageDetails& details) :
                _appPath(details.appPath), _appQuota(details.appQuota), _appUsedKB(details.appUsedKB),
                _persistentPath(details.persistentPath), _persistentQuota(details.persistentQuota), _persistentUsedKB(details.persistentUsedKB)
        {
        }

        ~StoragePayloadImpl() override
        {
        }

        Core::hresult Apps(ILISA::IStorage*& storage) const override
        {
            storage = (Core::Service<StorageImpl>::Create<ILISA::IStorage>(_appPath, _appQuota, _appUsedKB));
            ASSERT(storage);
            return Core::ERROR_NONE;
        }

        Core::hresult Persistent(ILISA::IStorage*& storage) const override
        {
            storage = (Core::Service<StorageImpl>::Create<ILISA::IStorage>( _persistentPath, _persistentQuota, _persistentUsedKB));
            ASSERT(storage);
            return Core::ERROR_NONE;
        }
    private:
        std::string _appPath;
        std::string _appQuota;
        std::string _appUsedKB;
        std::string _persistentPath;
        std::string _persistentQuota;
        std::string _persistentUsedKB;

    public:
        BEGIN_INTERFACE_MAP(StoragePayloadImpl)
            INTERFACE_ENTRY(Exchange::ILISA::IStoragePayload)
        END_INTERFACE_MAP
    }; // StoragePayload

    class KeyValueImpl : public ILISA::IKeyValue
    {
    public:
        KeyValueImpl() = delete;
        KeyValueImpl(const KeyValueImpl&) = delete;
        KeyValueImpl& operator=(const KeyValueImpl&) = delete;

        KeyValueImpl(const std::string key, const std::string value)
            : _key(key), _value(value)
        {
        }

        ~KeyValueImpl() override
        {
        }

        Core::hresult Key(std::string& key) const override
        {
            key = _key;
            return Core::ERROR_NONE;
        }

        Core::hresult Value(std::string& value) const override
        {
            value = _value;
            return Core::ERROR_NONE;
        }

    private:
        std::string _key;
        std::string _value;

    public:
        BEGIN_INTERFACE_MAP(KeyValueImpl)
            INTERFACE_ENTRY(Exchange::ILISA::IKeyValue)
        END_INTERFACE_MAP
    }; // class KeyValueImpl

    class KeyValueIteratorImpl : public ILISA::IKeyValueIterator {
        public:
            KeyValueIteratorImpl() = delete;
            KeyValueIteratorImpl(const KeyValueIteratorImpl&) = delete;
            KeyValueIteratorImpl& operator=(const KeyValueIteratorImpl&) = delete;

            KeyValueIteratorImpl(const std::list<KeyValueImpl*>& container)
            {
                std::list<KeyValueImpl*>::const_iterator index = container.begin();
                while (index != container.end()) {
                    ILISA::IKeyValue* element = (*index);
                    element->AddRef();
                    _list.push_back(element);
                    index++;
                }
            }

            ~KeyValueIteratorImpl() override
            {
                while (_list.size() != 0) {
                    _list.front()->Release();
                    _list.pop_front();
                }
            }

        public:
            Core::hresult Reset() override
            {
                _index = 0;
                return Core::ERROR_NONE;
            }

            bool IsValid() const
            {
                return ((_index != 0) && (_index <= _list.size()));
            }

            Core::hresult IsValid(bool& isValid) const override
            {
                isValid = IsValid();
                return Core::ERROR_NONE;
            }

            Core::hresult Next(bool& hasNext) override
            {
                if (_index == 0) {
                    _index = 1;
                    _iterator = _list.begin();
                } else if (_index <= _list.size()) {
                    _index++;
                    _iterator++;
                }
                hasNext = IsValid();
                return Core::ERROR_NONE;
            }

            Core::hresult Current(ILISA::IKeyValue*& keyValue) const override
            {
                ASSERT(IsValid() == true);
                ILISA::IKeyValue* result = nullptr;
                result = (*_iterator);
                ASSERT(result != nullptr);
                result->AddRef();
                keyValue = result;
                return Core::ERROR_NONE;
            }

        private:
            uint32_t _index = 0;
            std::list<ILISA::IKeyValue*> _list;
            std::list<ILISA::IKeyValue*>::iterator _iterator;

        public:
            BEGIN_INTERFACE_MAP(KeyValueIteratorImpl)
                INTERFACE_ENTRY(Exchange::ILISA::IKeyValueIterator)
            END_INTERFACE_MAP
    }; // class KeyValueIteratorImpl

    class MetadataPayloadImpl : public ILISA::IMetadataPayload
    {
    public:
        MetadataPayloadImpl() = delete;
        MetadataPayloadImpl(const MetadataPayloadImpl&) = delete;
        MetadataPayloadImpl& operator=(const MetadataPayloadImpl&) = delete;

        MetadataPayloadImpl(const std::string appName, const std::string category, const std::string url,
        const std::list<KeyValueImpl*>& resources, const std::list<KeyValueImpl*>& auxMetadata)
        {
            _appName = appName;
            _category = category;
            _url = url;

            {
                std::list<KeyValueImpl*>::const_iterator index = resources.begin();
                while (index != resources.end()) {
                    KeyValueImpl* element = (*index);
                    element->AddRef();
                    _resources.push_back(element);
                    index++;
                }
            }
            {
                std::list<KeyValueImpl*>::const_iterator index = auxMetadata.begin();
                while (index != auxMetadata.end()) {
                    KeyValueImpl* element = (*index);
                    element->AddRef();
                    _auxMetadata.push_back(element);
                    index++;
                }
            }
        }

        ~MetadataPayloadImpl() override
        {
            while (_resources.size() != 0) {
                _resources.front()->Release();
                _resources.pop_front();
            }
            while (_auxMetadata.size() != 0) {
                _auxMetadata.front()->Release();
                _auxMetadata.pop_front();
            }
        }

        Core::hresult AppName(string& appName /* @out */) const override
        {
            appName = _appName;
            return Core::ERROR_NONE;
        }

        Core::hresult Category(string& category /* @out */) const override
        {
            category = _category;
            return Core::ERROR_NONE;
        }

        Core::hresult Url(string& url /* @out */) const override
        {
            url = _url;
            return Core::ERROR_NONE;
        }

        Core::hresult Resources(ILISA::IKeyValueIterator*& resources) const override
        {
            resources = (Core::Service<KeyValueIteratorImpl>::Create<ILISA::IKeyValueIterator>(_resources));
            return Core::ERROR_NONE;
        }

        Core::hresult AuxMetadata(ILISA::IKeyValueIterator*& auxMetadata /* @out */) const override
        {
            auxMetadata = (Core::Service<KeyValueIteratorImpl>::Create<ILISA::IKeyValueIterator>(_auxMetadata));
            return Core::ERROR_NONE;
        }

    private:
        std::string _appName;
        std::string _category;
        std::string _url;
        std::list<KeyValueImpl*> _resources;
        std::list<KeyValueImpl*> _auxMetadata;

    public:
        BEGIN_INTERFACE_MAP(MetadataPayloadImpl)
            INTERFACE_ENTRY(Exchange::ILISA::IMetadataPayload)
        END_INTERFACE_MAP
    }; // MetadataPayloadImpl

    Core::hresult SetAuxMetadata(const std::string& type,
            const std::string& id,
            const std::string& version,
            const std::string& key,
            const std::string& value) override
    {
        return static_cast<Core::hresult>(executor.SetMetadata(type, id, version, key, value));
    }

    Core::hresult ClearAuxMetadata(const std::string& type,
            const std::string& id,
            const std::string& version,
            const std::string& key) override
    {
        return static_cast<Core::hresult>(executor.ClearMetadata(type, id, version, key));
    }

    Core::hresult GetMetadata(const std::string& type,
            const std::string& id,
            const std::string& version,
            ILISA::IMetadataPayload*& result) override
    {
        LISA::DataStorage::AppMetadata appMetadata;
        auto rc = executor.GetMetadata(type, id, version, appMetadata);

        std::list<KeyValueImpl*> resources;
        std::list<KeyValueImpl*> auxMetadata;

        if (rc == Core::ERROR_NONE) {
            // TODO: add resources (downloads) to the result here when download function is implemented

            // Add metadata to the result
            for (auto pair : appMetadata.metadata)
            {
                KeyValueImpl* keyValue = Core::Service<KeyValueImpl>::Create<KeyValueImpl>(
                    pair.first, pair.second);
                auxMetadata.push_back(keyValue);
            }
        }

        ILISA::IMetadataPayload* metadataPayload = Core::Service<MetadataPayloadImpl>::Create<ILISA::IMetadataPayload>(
            appMetadata.appDetails.appName, appMetadata.appDetails.category, appMetadata.appDetails.url,
            resources, auxMetadata
        );
        result = metadataPayload;

        for (auto res : resources) {
            res->Release();
        }
        for (auto meta : auxMetadata) {
            meta->Release();
        }
        return static_cast<Core::hresult>(rc);
    }

    Core::hresult Cancel(const std::string& handle) override
    {
        return static_cast<Core::hresult>(executor.Cancel(handle));
    }

    Core::hresult GetProgress(const std::string& handle, uint32_t& progress) override
    {
        return static_cast<Core::hresult>(executor.GetProgress(handle, progress));
    }

    Core::hresult Configure(const std::string& config) override
    {
        return static_cast<Core::hresult>(executor.Configure(config));
    }

    virtual Core::hresult Register(ILISA::INotification* notification) override
    {
        LockGuard lock(notificationMutex);
        // Make sure a callback is not registered multiple times.
        ASSERT(std::find(_notificationCallbacks.begin(), _notificationCallbacks.end(), notification) == _notificationCallbacks.end());

        _notificationCallbacks.push_back(notification);
        notification->AddRef();

        INFO("Register INotification: ", notification);

        return Core::ERROR_NONE;
    }

    virtual Core::hresult Unregister(ILISA::INotification* notification) override
    {
        LockGuard lock(notificationMutex);
        auto index(std::find(_notificationCallbacks.begin(), _notificationCallbacks.end(), notification));

        // Make sure you do not unregister something you did not register !!!
        ASSERT(index != _notificationCallbacks.end());

        if (index != _notificationCallbacks.end()) {
            (*index)->Release();
            _notificationCallbacks.erase(index);
        }
        return Core::ERROR_NONE;
    }

private:
    void onOperationStatus(const LISA::Executor::OperationStatusEvent& event)
    {
        INFO("LISA onOperationStatus handle:", event.handle, " status: ", event.status, " details: ", event.details);
        LockGuard lock(notificationMutex);
        for(const auto index: _notificationCallbacks) {
            index->OperationStatus(event.handle, event.operationStr(), event.type, event.id, event.version, event.statusStr(), event.details);
        }
    }

public:
    BEGIN_INTERFACE_MAP(LISAImplementation)
        INTERFACE_ENTRY(Exchange::ILISA)
    END_INTERFACE_MAP

public:
    class AppVersionImpl : public ILISA::IAppVersion
    {
    public:
        class IteratorImpl : public ILISA::IAppVersion::IIterator {
        public:
            IteratorImpl() = delete;
            IteratorImpl(const IteratorImpl&) = delete;
            IteratorImpl& operator=(const IteratorImpl&) = delete;

            IteratorImpl(const std::list<AppVersionImpl*>& container)
            {
                std::list<AppVersionImpl*>::const_iterator index = container.begin();
                while (index != container.end()) {
                    ILISA::IAppVersion* element = (*index);
                    element->AddRef();
                    _list.push_back(element);
                    index++;
                }
            }

            ~IteratorImpl() override
            {
                while (_list.size() != 0) {
                    _list.front()->Release();
                    _list.pop_front();
                }
            }

        public:
            Core::hresult Reset() override
            {
                _index = 0;
                return Core::ERROR_NONE;
            }

            bool IsValid() const
            {
                return ((_index != 0) && (_index <= _list.size()));
            }

            Core::hresult IsValid(bool& isValid) const override
            {
                isValid = IsValid();
                return Core::ERROR_NONE;
            }

            Core::hresult Next(bool& hasNext) override
            {
                if (_index == 0) {
                    _index = 1;
                    _iterator = _list.begin();
                } else if (_index <= _list.size()) {
                    _index++;
                    _iterator++;
                }
                hasNext = IsValid();
                return Core::ERROR_NONE;
            }

            Core::hresult Current(ILISA::IAppVersion*& version) const override
            {
                ASSERT(IsValid() == true);
                ILISA::IAppVersion* result = nullptr;
                result = (*_iterator);
                ASSERT(result != nullptr);
                result->AddRef();
                version = result;
                return Core::ERROR_NONE;
            }

        public:
            BEGIN_INTERFACE_MAP(AppVersionImpl::IteratorImpl)
                INTERFACE_ENTRY(Exchange::ILISA::IAppVersion::IIterator)
            END_INTERFACE_MAP

        private:
            uint32_t _index = 0;
            std::list<ILISA::IAppVersion*> _list;
            std::list<ILISA::IAppVersion*>::iterator _iterator;
        }; // class IteratorImpl

    public:
        AppVersionImpl() = delete;
        AppVersionImpl(const AppVersionImpl&) = delete;
        AppVersionImpl& operator=(const AppVersionImpl&) = delete;

        AppVersionImpl(const std::string version, const std::string appName, const std::string category, const std::string url) 
            : _version(version), _appName(appName), _category(category), _url(url)
        {
        }

        ~AppVersionImpl() override
        {
        }

        Core::hresult Version(std::string& version) const override
        {
            version = _version;
            return Core::ERROR_NONE;
        }

        Core::hresult AppName(std::string& appName) const override
        {
            appName = _appName;
            return Core::ERROR_NONE;
        }

        Core::hresult Category(std::string& category) const override
        {
            category = _category;
            return Core::ERROR_NONE;
        }

        Core::hresult Url(std::string& url) const override
        {
            url = _url;
            return Core::ERROR_NONE;
        }

    private:
        std::string _version;
        std::string _appName;
        std::string _category;
        std::string _url;

    public:
        BEGIN_INTERFACE_MAP(AppVersionImpl)
            INTERFACE_ENTRY(Exchange::ILISA::IAppVersion)
        END_INTERFACE_MAP
    }; // class AppVersionImpl

public:
    class AppImpl : public ILISA::IApp
    {
    public:
        class IteratorImpl : public ILISA::IApp::IIterator {
        public:
            IteratorImpl() = delete;
            IteratorImpl(const IteratorImpl&) = delete;
            IteratorImpl& operator=(const IteratorImpl&) = delete;

            IteratorImpl(const std::list<AppImpl*>& container)
            {
                std::list<AppImpl*>::const_iterator index = container.begin();
                while (index != container.end()) {
                    ILISA::IApp* element = (*index);
                    element->AddRef();
                    _list.push_back(element);
                    index++;
                }
            }

            ~IteratorImpl() override
            {
                while (_list.size() != 0) {
                    _list.front()->Release();
                    _list.pop_front();
                }
            }

        public:
            Core::hresult Reset() override
            {
                _index = 0;
                return Core::ERROR_NONE;
            }

            bool IsValid() const
            {
                return ((_index != 0) && (_index <= _list.size()));
            }

            Core::hresult IsValid(bool& isValid) const override
            {
                isValid = IsValid();
                return Core::ERROR_NONE;
            }

            Core::hresult Next(bool& hasNext) override
            {
                if (_index == 0) {
                    _index = 1;
                    _iterator = _list.begin();
                } else if (_index <= _list.size()) {
                    _index++;
                    _iterator++;
                }
                hasNext = IsValid();
                return Core::ERROR_NONE;
            }

            Core::hresult Current(ILISA::IApp*& app) const override
            {
                ASSERT(IsValid() == true);
                ILISA::IApp* result = nullptr;
                result = (*_iterator);
                ASSERT(result != nullptr);
                result->AddRef();
                app = result;
                return Core::ERROR_NONE;
            }

        public:
            BEGIN_INTERFACE_MAP(AppImpl::IteratorImpl)
                INTERFACE_ENTRY(Exchange::ILISA::IApp::IIterator)
            END_INTERFACE_MAP

        private:
            uint32_t _index = 0;
            std::list<ILISA::IApp*> _list;
            std::list<ILISA::IApp*>::iterator _iterator;
        }; // class IteratorImpl

    public:
        AppImpl() = delete;
        AppImpl(const AppImpl&) = delete;
        AppImpl& operator=(const AppImpl&) = delete;

        AppImpl(const std::string type, const std::string id, const std::list<AppVersionImpl*>& versions) 
            : _type(type), _id(id)
        {
            std::list<AppVersionImpl*>::const_iterator index = versions.begin();
            while (index != versions.end()) {
                AppVersionImpl* element = (*index);
                element->AddRef();
                _versions.push_back(element);
                index++;
            }
        }

        ~AppImpl() override
        {
            while (_versions.size() != 0) {
                _versions.front()->Release();
                _versions.pop_front();
            }
        }

        Core::hresult Type(string& type) const override
        {
            type = _type;
            return Core::ERROR_NONE;
        }

        Core::hresult Id(string& id) const override
        {
            id = _id;
            return Core::ERROR_NONE;
        }

        Core::hresult Installed(ILISA::IAppVersion::IIterator*& versions) const override
        {
            versions = Core::Service<AppVersionImpl::IteratorImpl>::Create<ILISA::IAppVersion::IIterator>(_versions);
            ASSERT(versions);
            return Core::ERROR_NONE;
        }

    private:
        std::string _type;
        std::string _id;
        std::list<AppVersionImpl*> _versions;

    public:
        BEGIN_INTERFACE_MAP(AppImpl)
            INTERFACE_ENTRY(Exchange::ILISA::IApp)
        END_INTERFACE_MAP
    }; // class AppImpl

public:
    class AppsPayloadImpl : public ILISA::IAppsPayload
    {
    public:
        AppsPayloadImpl() = delete;
        AppsPayloadImpl(const AppsPayloadImpl&) = delete;
        AppsPayloadImpl& operator=(const AppsPayloadImpl&) = delete;

        AppsPayloadImpl(const std::list<AppImpl*>&  apps)
        {
            std::list<AppImpl*>::const_iterator index = apps.begin();
            while (index != apps.end()) {
                AppImpl* element = (*index);
                element->AddRef();
                _apps.push_back(element);
                index++;
            }
        }

        ~AppsPayloadImpl() override
        {
            while (_apps.size() != 0) {
                _apps.front()->Release();
                _apps.pop_front();
            }
        }

        Core::hresult Apps(ILISA::IApp::IIterator*& apps) const override
        {
            apps = (Core::Service<AppImpl::IteratorImpl>::Create<ILISA::IApp::IIterator>(_apps));
            ASSERT(apps);
            return Core::ERROR_NONE;
        }
    private:
        std::list<AppImpl*> _apps;

    public:
        BEGIN_INTERFACE_MAP(AppsPayloadImpl)
            INTERFACE_ENTRY(Exchange::ILISA::IAppsPayload)
        END_INTERFACE_MAP
    }; // AppsPayloadImpl

    /* @brief List installed applications. */
    Core::hresult GetList(
        const std::string& type,
        const std::string& id,
        const std::string& version,
        const std::string& appName,
        const std::string& category,
        IAppsPayload*& result) const override
    {
        INFO(" ");

        std::vector<LISA::DataStorage::AppDetails> appsDetailsList{};
        auto rc = executor.GetAppDetailsList(type, id, version, appName, category, appsDetailsList);
        if (rc == Core::ERROR_NONE) {

            std::map<std::pair<std::string, std::string>, std::list<AppVersionImpl*>> appsDet;
            for(const auto& app: appsDetailsList)
            {
                if (app.version.empty()) {
                    appsDet[{app.type, app.id}].clear();
                } else {
                    appsDet[{app.type, app.id}].push_back(
                            Core::Service<AppVersionImpl>::Create<AppVersionImpl>(app.version, app.appName,
                                                                                  app.category, app.url));
                }
            }
            std::list<AppImpl*> apps;
            for(const auto& app: appsDet)
            {
                auto appVersions = app.second;
                apps.push_back(Core::Service<AppImpl>::Create<AppImpl>(app.first.first, app.first.second, appVersions));
                for(auto appVersion : appVersions) {
                    appVersion->Release();
                }
            }

            // Create apps payload which will be returned as the result
            ILISA::IAppsPayload* appsPayload = Core::Service<AppsPayloadImpl>::Create<ILISA::IAppsPayload>(apps);
            result = appsPayload;

            for (auto app : apps) {
                app->Release();
            }
        }
        return rc;
    }

    Core::hresult GetStorageDetails(const std::string& type,
            const std::string& id,
            const std::string& version,
            ILISA::IStoragePayload*& result) override
    {
        uint32_t ret = Core::ERROR_NONE;
        LISA::Filesystem::StorageDetails details;
        if(type.empty() && !cachedStorageDetails.appPath.empty()) {
            INFO("Using cached values");
            details = cachedStorageDetails;
        } else {
            INFO("Calculating storage usage");
            ret = executor.GetStorageDetails(type, id, version, details);
        }
        result = Core::Service<StoragePayloadImpl>::Create<ILISA::IStoragePayload>(details);
        return ret;
    }

private:
    LISA::Executor executor{
        [this](const LISA::Executor::OperationStatusEvent& event)
        {
            this->onOperationStatus(event);
        }
    };
    using LockGuard = std::lock_guard<std::mutex>;
    std::list<Exchange::ILISA::INotification*> _notificationCallbacks{};
    std::mutex notificationMutex{};
    LISA::Filesystem::StorageDetails cachedStorageDetails;
};

SERVICE_REGISTRATION(LISAImplementation, 1, 0);
}
}

