#include <tinyxml2.h>

#include <ServerParameters.h>

int ServerParameters::load(const char *fileName)
{
    tinyxml2::XMLDocument doc;
    doc.LoadFile(fileName);

    if(doc.ErrorID() != 0)
    {
        printf("can't load config file\n");
        return -1;    
    }    

    tinyxml2::XMLElement *el = doc.FirstChildElement();
    if(el == nullptr)
    {
        printf("invalid config file\n");
        return -1;
    }
    el = el->FirstChildElement("maxClients");
    if(el == nullptr)
    {
        printf("invalid config file\n");
        return -1;
    }    
    if(el->QueryIntText(&maxClients) != 0)
    {
        printf("invalid maxClients value\n");
        return -1;
    }

    


    return 0;
}


