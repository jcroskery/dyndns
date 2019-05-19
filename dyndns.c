#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h> //You must link the libcurl library or the code will fail to link

#define BASE_URL "https://api.cloudflare.com/client/v4/zones/"
#define DNS_RECORDS "/dns_records"
//To use this program, you must pass at least 3 command line arguments
//The first: Your Cloudflare account's email address, e.g. "X-Auth-Email: youremail@example.com"
//The second: Your 32 digit Cloudflare API key, e.g. "X-Auth-Key: 00000000000000000000000000000000"
//The third: Domain names you wish to update with your IP, e.g. "olmmcc.tk"
//Other arguments will be treated in the same way as the third argument

struct Result
{
    char *memory;
    size_t sizeOfMemory;
};
struct Result* initResult(struct Result* uninitResult)
{
    uninitResult->memory = malloc(1);
    uninitResult->sizeOfMemory = 0;
    return uninitResult;
}
void addDataToResult(struct Result *returnStruct, void *contents, size_t realSize)
{
    returnStruct->memory = realloc(returnStruct->memory, returnStruct->sizeOfMemory + realSize + 1); //I am lazy, so if we're out of memory then I don't know what happens
    memcpy(&(returnStruct->memory[returnStruct->sizeOfMemory]), contents, realSize);
    returnStruct->sizeOfMemory += realSize;
    returnStruct->memory[returnStruct->sizeOfMemory] = 0;
}

static size_t GetResult(void *contents, size_t sizeOfMembers, size_t numberOfMembers, void *myPointer)
{
    size_t realSize = sizeOfMembers * numberOfMembers;
    struct Result *returnStruct = (struct Result *)myPointer;
    addDataToResult(returnStruct, contents, realSize);

    return realSize; //For some reason this must be returned
}
CURLcode submitCurlRequest(CURL* curl, struct Result* requestResult)
{
    initResult(requestResult);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, GetResult);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)requestResult);
    return curl_easy_perform(curl);
}
size_t read_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    size_t realSize = size * nitems;
    struct Result *uploadData = (struct Result *)userdata;
    size_t amountOfMemory = (realSize > uploadData->sizeOfMemory) ? uploadData->sizeOfMemory : realSize;
    memcpy(buffer, uploadData->memory, amountOfMemory);
    uploadData->sizeOfMemory -= amountOfMemory;
    uploadData->memory += amountOfMemory; //Causes a memory leak but who cares?
    return amountOfMemory;
}

int main(int argc, char *argv[])
{
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();
    struct curl_slist *list = NULL;
    list = curl_slist_append(list, argv[1]);
    list = curl_slist_append(list, argv[2]);
    list = curl_slist_append(list, "Content-Type: application/json");

    struct Result url;
    initResult(&url);
    addDataToResult(&url, BASE_URL, strlen(BASE_URL));
    curl_easy_setopt(curl, CURLOPT_URL, "https://ipecho.net/plain");
    struct Result ip;
    submitCurlRequest(curl, &ip);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    struct Result zoneIdRequest;
    submitCurlRequest(curl, &zoneIdRequest);
    addDataToResult(&url, strstr(zoneIdRequest.memory, "id") + 5, 32);
    addDataToResult(&url, DNS_RECORDS, strlen(DNS_RECORDS));
    free(zoneIdRequest.memory);
    for(int i = 3; i < argc; i++)
    {
        char tempRequestUrl[1024];
        snprintf(tempRequestUrl, sizeof tempRequestUrl, "%s?type=A&name=%s", url.memory, argv[i]);
        curl_easy_reset(curl);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
        curl_easy_setopt(curl, CURLOPT_URL, tempRequestUrl);
        struct Result recordIdRequest;
        submitCurlRequest(curl, &recordIdRequest);
        char* tmpMalloc = malloc(32);
        memcpy(tmpMalloc, strstr(recordIdRequest.memory, "id") + 5, 32);
        snprintf(tempRequestUrl, sizeof tempRequestUrl, "%s/%s", url.memory, tmpMalloc);
        free(tmpMalloc);
        free(recordIdRequest.memory);
        curl_easy_setopt(curl, CURLOPT_URL, tempRequestUrl);
        struct Result cloudflareIpRequest;
        submitCurlRequest(curl, &cloudflareIpRequest);

        struct Result cloudflareIp;
        initResult(&cloudflareIp);
        char* beginningOfCloudflareIp = strstr(cloudflareIpRequest.memory, "content") + 10;
        addDataToResult(&cloudflareIp, beginningOfCloudflareIp, strstr(beginningOfCloudflareIp, "\"") - beginningOfCloudflareIp);
        free(cloudflareIpRequest.memory);
        if(!strcmp(ip.memory, cloudflareIp.memory)) //strcmp is really weird, why is the ! needed?
        {
            printf("The IP address for %s is up to date.\n", argv[i]);
        }
        else
        {
            struct Result putFile;
            initResult(&putFile);
            char tmpPutFile[1024];
            sprintf(tmpPutFile, "{\"type\":\"A\",\"name\":\"%s\",\"content\":\"%s\",\"proxied\":false}", argv[i], ip.memory);
            addDataToResult(&putFile, tmpPutFile, strlen(tmpPutFile));
            struct Result ipChangeResult;
            curl_easy_setopt(curl, CURLOPT_READDATA, &putFile);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(curl, CURLOPT_URL, tempRequestUrl);
            curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, putFile.sizeOfMemory);
            submitCurlRequest(curl, &ipChangeResult);
            printf("The IP for %s has been successfully updated.\n", argv[i]);
        }
    }
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    //End of program automatically frees variables
    return 0;
}
