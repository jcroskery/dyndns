#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

//C programming is fun!

struct Result
{
    char *memory;
    size_t sizeOfMemory;
};

static size_t GetResult(void *contents, size_t sizeOfMembers, size_t numberOfMembers, void *myPointer)
{
    size_t realSize = sizeOfMembers * numberOfMembers;
    struct Result *returnStruct = (struct Result *)myPointer;
    returnStruct->memory = realloc(returnStruct->memory, returnStruct->sizeOfMemory + realSize + 1); //I am lazy, so if we're out of memory then I don't know what happens
    memcpy(&(returnStruct->memory[returnStruct->sizeOfMemory]), contents, realSize);
    returnStruct->sizeOfMemory += realSize;
    returnStruct->memory[returnStruct->sizeOfMemory] = 0; //Don't include garbage in our final result!

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

int main(void)
{
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, "https://ipecho.net/plain");
    struct Result ip;
    submitCurlRequest(curl, &ip);

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.cloudflare.com/client/v4/zones/");
    struct Result zoneId;
    submitCurlRequest(curl, &zoneId);
    printf("%s", zoneId.memory);

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    //End of program automatically frees variables
    return 0;
}
