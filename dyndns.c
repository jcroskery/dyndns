#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h> //You must link the libcurl library or the code will fail to link

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
struct UploadData
{
    char *memory;
    char sizeOfMemory;
    size_t sizeOfRemainder;
};

static size_t GetResult(void *contents, size_t sizeOfMembers, size_t numberOfMembers, void *myPointer)
{
    size_t realSize = sizeOfMembers * numberOfMembers;
    struct Result *returnStruct = (struct Result *)myPointer;
    returnStruct->memory = realloc(returnStruct->memory, returnStruct->sizeOfMemory + realSize + 1); //I am lazy, so if we're out of memory then I don't know what happens
    memcpy(&(returnStruct->memory[returnStruct->sizeOfMemory]), contents, realSize);
    returnStruct->sizeOfMemory += realSize;
    returnStruct->memory[returnStruct->sizeOfMemory] = 0;

    return realSize; //For some reason this must be returned
}
CURLcode submitCurlRequest(CURL* curl, struct Result* requestResult)
{
    requestResult->memory = malloc(1);
    requestResult->sizeOfMemory = 0;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, GetResult);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)requestResult);
    return curl_easy_perform(curl);
}
size_t read_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    size_t realSize = size * nitems;
    struct UploadData *uploadJSON = (struct UploadData *)userdata;
    size_t amountOfMemory = (realSize > uploadJSON->sizeOfRemainder) ? uploadJSON->sizeOfRemainder : realSize;
    memcpy(buffer, uploadJSON->memory, amountOfMemory);
    uploadJSON->sizeOfRemainder -= amountOfMemory;
    uploadJSON->memory += amountOfMemory;
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

    curl_easy_setopt(curl, CURLOPT_URL, "https://ipecho.net/plain");
    struct Result ip;
    submitCurlRequest(curl, &ip);

    char* url = malloc(1024);
    sprintf(url, "https://api.cloudflare.com/client/v4/zones/");
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    struct Result zoneIdRequest;
    submitCurlRequest(curl, &zoneIdRequest);
    sprintf(url + strlen(url), "%.32s/dns_records", strstr(zoneIdRequest.memory, "id") + 5);
    free(zoneIdRequest.memory);
    for(int i = 3; i < argc; i++)
    {
        char* requestUrl = malloc(1024);
        sprintf(requestUrl, "%s?type=A&name=%s", url, argv[i]);
        curl_easy_reset(curl);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
        curl_easy_setopt(curl, CURLOPT_URL, requestUrl);
        struct Result recordIdRequest;
        submitCurlRequest(curl, &recordIdRequest);
        sprintf(requestUrl, "%s/%.32s", url, strstr(recordIdRequest.memory, "id") + 5);
        free(recordIdRequest.memory);

        curl_easy_setopt(curl, CURLOPT_URL, requestUrl);
        struct Result cloudflareIpRequest;
        submitCurlRequest(curl, &cloudflareIpRequest);

        char cloudflareIp[16];
        char* beginningOfCloudflareIp = strstr(cloudflareIpRequest.memory, "content") + 10;
        sprintf(cloudflareIp, "%.*s", (int) (strstr(beginningOfCloudflareIp, "\"") - beginningOfCloudflareIp), strstr(cloudflareIpRequest.memory, "content") + 10);
        free(cloudflareIpRequest.memory);
        if(!strcmp(ip.memory, cloudflareIp)) //strcmp return 0 if the strings match, so negate it
        {
            printf("The IP address for %s is up to date.\n", argv[i]);
        }
        else
        {
            struct UploadData uploadJSON;
            uploadJSON.memory = malloc(100+strlen(argv[i]));
            sprintf(uploadJSON.memory, "{\"type\":\"A\",\"name\":\"%s\",\"content\":\"%s\",\"proxied\":false}", argv[i], ip.memory);
            uploadJSON.sizeOfMemory = uploadJSON.sizeOfRemainder = strlen(uploadJSON.memory);
            curl_easy_setopt(curl, CURLOPT_READDATA, &uploadJSON);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, uploadJSON.sizeOfMemory);
            struct Result ipChangeResult;
            submitCurlRequest(curl, &ipChangeResult);
            printf("The IP for %s has been successfully updated.\n", argv[i]);
            free(uploadJSON.memory - uploadJSON.sizeOfMemory);
            free(ipChangeResult.memory);
        }
        free(requestUrl);
    }
    free(url);
    free(ip.memory);
    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    //End of program automatically frees variables
    return 0;
}
