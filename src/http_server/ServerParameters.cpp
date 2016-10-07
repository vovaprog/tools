#include <tinyxml2.h>

#include <ServerParameters.h>

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

    if(readElementString(root, "rootFolder", rootFolder) != 0)
    {
        return -1;
    }

    std::string s;
    if(readElementString(root, "logLevel", s) != 0)
    {
        return -1;
    }
    if(s == "debug") logLevel = Log::Level::debug;
    else if(s == "info") logLevel = Log::Level::info;
    else if(s == "warning") logLevel = Log::Level::warning;
    else if(s == "error") logLevel = Log::Level::error;
    else
    {
        printf("invalid logLevel\n");
        return -1;
    }

    s = "";
    if(readElementString(root, "logType", s) != 0)
    {
        return -1;
    }
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
        for (tinyxml2::XMLElement *child = parent->FirstChildElement("httpPort"); child != NULL; child = child->NextSiblingElement())        
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
        for (tinyxml2::XMLElement *child = parent->FirstChildElement("httpsPort"); child != NULL; child = child->NextSiblingElement())        
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
    
    return 0;
}



