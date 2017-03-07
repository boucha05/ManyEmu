#include "YamlSerializer.h"
#include "Base64.h"

#pragma warning(push, 3)
#include <yaml-cpp/yaml.h>
#pragma warning(pop)

#include <cstdio>
#include <functional>
#include <string>
#include <strstream>
#include <vector>

namespace
{
    class YamlReaderImpl : public emu::YamlReader
    {
    public:
        YamlReaderImpl()
            : mSuccess(true)
        {
            mContext.push_back(YAML::Node());
        }

        void safeExecute(std::function<void()> func)
        {
            try
            {
                func();
            }
            catch (YAML::Exception except)
            {
                mSuccess = false;
                printf("YamlReaderImpl: %s\n", except.what());
            }
        }

        virtual bool success() const
        {
            return mSuccess;
        }

        virtual bool isWriting() const override
        {
            return false;
        }

        template <typename T>
        void valueRead(T& item)
        {
            safeExecute([&]()
            {
                item = mContext.back().as<std::remove_reference<decltype(item)>::type>();
            });
        }

        virtual void value(bool& item) override
        {
            valueRead(item);
        }

        virtual void value(char& item) override
        {
            valueRead(item);
        }

        virtual void value(int8_t& item) override
        {
            valueRead(item);
        }

        virtual void value(uint8_t& item) override
        {
            valueRead(item);
        }

        virtual void value(int16_t& item) override
        {
            valueRead(item);
        }

        virtual void value(uint16_t& item) override
        {
            valueRead(item);
        }

        virtual void value(int32_t& item) override
        {
            valueRead(item);
        }

        virtual void value(uint32_t& item) override
        {
            valueRead(item);
        }

        virtual void value(int64_t& item) override
        {
            valueRead(item);
        }

        virtual void value(uint64_t& item) override
        {
            valueRead(item);
        }

        virtual void value(float& item) override
        {
            valueRead(item);
        }

        virtual void value(double& item) override
        {
            valueRead(item);
        }

        virtual void value(std::string& item) override
        {
            valueRead(item);
        }

        virtual void value(emu::Buffer& item) override
        {
            std::string value;
            valueRead(value);
            emu::Base64::decode(item, value);
        }

        virtual bool nodeBegin(const char* name) override
        {
            auto node = mContext.back()[name];
            bool success = !!node;
            if (node)
            {
                mContext.push_back(node);
            }
            return success;
        }

        virtual void nodeEnd() override
        {
            mContext.pop_back();
        }

        virtual bool sequenceBegin(size_t& size) override
        {
            size = mContext.back().size();
            auto iterator = mContext.back().begin();
            mIterator.push_back(iterator);
            mContext.push_back(YAML::Node());
            return true;
        }

        virtual void sequenceEnd() override
        {
            mContext.pop_back();
            mIterator.pop_back();
        }

        virtual void sequenceItem() override
        {
            auto& iterator = mIterator.back();
            mContext.pop_back();
            mContext.push_back(*iterator);
            ++iterator;
        }

        virtual bool read(const std::string& data) override
        {
            safeExecute([&]()
            {
                mContext.back() = YAML::Load(data.c_str());
            });
            return mSuccess;
        }

    private:
        bool                            mSuccess;
        std::vector<YAML::Node>         mContext;
        std::vector<YAML::iterator>     mIterator;
    };

    ////////////////////////////////////////////////////////////////////////////

    class YamlWriterImpl : public emu::YamlWriter
    {
    public:
        YamlWriterImpl()
            : mSuccess(true)
        {
            mContext.push_back(YAML::Node());
        }

        void safeExecute(std::function<void()> func)
        {
            try
            {
                func();
            }
            catch (YAML::Exception except)
            {
                mSuccess = false;
                printf("YamlWriterImpl: %s\n", except.what());
            }
        }

        virtual bool success() const
        {
            return mSuccess;
        }

        virtual bool isWriting() const override
        {
            return true;
        }

        template <typename T>
        void valueWrite(T& item)
        {
            safeExecute([&]()
            {
                mContext.back() = item;
            });
        }

        virtual void value(bool& item) override
        {
            valueWrite(item);
        }

        virtual void value(char& item) override
        {
            valueWrite(item);
        }

        virtual void value(int8_t& item) override
        {
            valueWrite(item);
        }

        virtual void value(uint8_t& item) override
        {
            valueWrite(item);
        }

        virtual void value(int16_t& item) override
        {
            valueWrite(item);
        }

        virtual void value(uint16_t& item) override
        {
            valueWrite(item);
        }

        virtual void value(int32_t& item) override
        {
            valueWrite(item);
        }

        virtual void value(uint32_t& item) override
        {
            valueWrite(item);
        }

        virtual void value(int64_t& item) override
        {
            valueWrite(item);
        }

        virtual void value(uint64_t& item) override
        {
            valueWrite(item);
        }

        virtual void value(float& item) override
        {
            valueWrite(item);
        }

        virtual void value(double& item) override
        {
            valueWrite(item);
        }

        virtual void value(std::string& item) override
        {
            valueWrite(item);
        }

        virtual void value(emu::Buffer& item) override
        {
            std::string value;
            emu::Base64::encode(value, item.data(), item.size());
            valueWrite(value);
        }

        virtual bool nodeBegin(const char* name) override
        {
            auto node = mContext.back()[name] = YAML::Node();
            mContext.push_back(node);
            return true;
        }

        virtual void nodeEnd() override
        {
            mContext.pop_back();
        }

        virtual bool sequenceBegin(size_t& size) override
        {
            (void)size;
            mContext.push_back(YAML::Node());
            return true;
        }

        virtual void sequenceEnd() override
        {
            mContext.pop_back();
        }

        virtual void sequenceItem() override
        {
            mContext.pop_back();
            auto node = YAML::Node();
            mContext.back().push_back(node);
            mContext.push_back(node);
        }

        virtual bool write(std::string& data) override
        {
            safeExecute([&]()
            {
                std::string result = YAML::Dump(mContext.back());
                data = result.c_str();
            });
            return mSuccess;
        }

    private:
        bool                        mSuccess;
        std::vector<YAML::Node>     mContext;
    };
}

namespace emu
{
    YamlReader* YamlReader::create()
    {
        return new YamlReaderImpl();
    }

    void YamlReader::destroy(YamlReader& instance)
    {
        delete &static_cast<YamlReaderImpl&>(instance);
    }

    YamlWriter* YamlWriter::create()
    {
        return new YamlWriterImpl();
    }

    void YamlWriter::destroy(YamlWriter& instance)
    {
        delete &static_cast<YamlWriterImpl&>(instance);
    }
}