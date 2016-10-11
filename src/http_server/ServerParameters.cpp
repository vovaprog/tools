#include <ServerParameters.h>

#include <tinyxml2.h>


tinyxml2::XMLElement* getChild(tinyxml2::XMLElement *parent, const char *elementName, bool required)
{
    tinyxml2::XMLElement *el = parent->FirstChildElement(elementName);
    if(el == nullptr)
    {
        if(required)
        {
            printf("element %s not found\n", elementName);
            return nullptr;
        }
        return nullptr;
    }
    return el;
}

int readElementInt(tinyxml2::XMLElement *parent, const char *elementName, int &result, bool required = false)
{
    tinyxml2::XMLElement *el = getChild(parent, elementName, false);
    if(el != nullptr)
    {
        if(el->QueryIntText(&result) != 0)
        {
            printf("invalid %s value\n", elementName);
            return -1;
        }
        return 0;
    }
    else
    {
        return required ? -1 : 0;
    }
}

int readElementString(tinyxml2::XMLElement *parent, const char *elementName, std::string &result, bool required = false)
{
    tinyxml2::XMLElement *el = getChild(parent, elementName, required);
    if(el != nullptr)
    {
        result = el->GetText();
        return 0;
    }
    else
    {
        return required ? -1 : 0;
    }
}

#define INT_PARAM(paramName) if(readElementInt(root, #paramName, paramName) != 0) { return -1; }
#define STRING_PARAM(paramName) if(readElementString(root, #paramName, paramName) != 0) { return -1; }
#define STRING_PARAM_RET(parent, paramName, ret) if(readElementString(parent, #paramName, ret) != 0) { return -1; }

int ServerParameters::load(const char *fileName)
{
    setDefaults();

    tinyxml2::XMLDocument doc;
    doc.LoadFile(fileName);

    if(doc.ErrorID() != 0)
    {
        printf("can't load config file\n");
        return -1;
    }

    tinyxml2::XMLElement *root = doc.FirstChildElement();
    if(root == nullptr)
    {
        printf("invalid config file\n");
        return -1;
    }

    INT_PARAM(maxClients);
    INT_PARAM(threadCount);
    INT_PARAM(executorTimeoutMillis);
    INT_PARAM(logFileSize);
    INT_PARAM(logArchiveCount);
    STRING_PARAM(rootFolder);
    STRING_PARAM(logFolder);

    std::string s = "info";
    STRING_PARAM_RET(root, logLevel, s);
    if(s == "debug") logLevel = Log::Level::debug;
    else if(s == "info") logLevel = Log::Level::info;
    else if(s == "warning") logLevel = Log::Level::warning;
    else if(s == "error") logLevel = Log::Level::error;
    else
    {
        printf("invalid logLevel\n");
        return -1;
    }

    s = "stdout";
    STRING_PARAM_RET(root, logType, s);
    if(s == "stdout") logType = Log::Type::stdout;
    else if(s == "mmap") logType = Log::Type::mmap;
    else
    {
        printf("invalid logType\n");
        return -1;
    }

    tinyxml2::XMLElement *parent = root->FirstChildElement("httpPorts");
    if(parent != nullptr)
    {
        for(tinyxml2::XMLElement *child = parent->FirstChildElement("port"); child != NULL; child = child->NextSiblingElement())
        {
            int port;
            if(child->QueryIntText(&port) != 0)
            {
                printf("invalid httpPort value\n");
                return -1;
            }
            httpPorts.push_back(port);
        }
    }
    if(httpPorts.size() == 0)
    {
        httpPorts.push_back(8080);
    }

    parent = root->FirstChildElement("httpsPorts");
    if(parent != nullptr)
    {
        for(tinyxml2::XMLElement *child = parent->FirstChildElement("port"); child != NULL; child = child->NextSiblingElement())
        {
            int port;
            if(child->QueryIntText(&port) != 0)
            {
                printf("invalid httpPort value\n");
                return -1;
            }
            httpsPorts.push_back(port);
        }
    }

    parent = root->FirstChildElement("uwsgiApplications");
    if(parent != nullptr)
    {
        for(tinyxml2::XMLElement *child = parent->FirstChildElement("application"); child != NULL; child = child->NextSiblingElement())
        {
            UwsgiApplicationParameters app;

            if(readElementString(child, "prefix", app.prefix, true) != 0)
            {
                return -1;
            }
            if(readElementInt(child, "port", app.port, true) != 0)
            {
                return -1;
            }

            uwsgiApplications.push_back(app);
        }
    }

    return 0;
}

void ServerParameters::writeToLog(Log *log)
{
    log->info("----- server parameters -----\n");
    log->info("rootFolder: %s\n", rootFolder.c_str());
    log->info("maxClients: %d\n", maxClients);
    log->info("threadCount: %d\n", threadCount);
    log->info("executorTimeoutMillis: %d\n", executorTimeoutMillis);
    log->info("logLevel: %s\n", Log::logLevelString(logLevel));
    log->info("logType: %s\n", Log::logTypeString(logType));
    log->info("logFileSize: %d\n", logFileSize);
    log->info("logArchiveCount: %d\n", logArchiveCount);
    for(int port : httpPorts)
    {
        log->info("httpPort: %d\n", port);
    }
    for(int port : httpsPorts)
    {
        log->info("httpsPort: %d\n", port);
    }
    for(UwsgiApplicationParameters & app : uwsgiApplications)
    {
        log->info("uwsgi application   prefix: %s   port: %d\n", app.prefix.c_str(), app.port);
    }
    log->info("-----------------------------\n");
}


