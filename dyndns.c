#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h> //You must link the libcurl library or the code will fail to link

/*
Copyright (C) 2019  Justus Croskery
To contact me, email me at justus@olmmcc.tk.
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program. If not, see https://www.gnu.org/licenses/.
*/

/*
To use this program, you must pass at least 4 command line arguments:

The first: Whether you want to use the caching feature of this program to cache your Cloudflare zone id and record id(s).
A value of 0 means "do not cache" and a value of 1 means "cache".
If you are using caching and the cache is corrupted or the domain argument(s) passed to the program change, then the program will delete the cache and regenerate it.

The second: Your Cloudflare account's email address, e.g. "X-Auth-Email: youremail@example.com".

The third: Your 32 digit Cloudflare API key, e.g. "X-Auth-Key: 00000000000000000000000000000000".

The fourth: Domain name you wish to update with your IP, e.g. "example.com" or "www.example.com".

Other arguments will be treated in the same way as the fourth argument.
*/

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

size_t GetResult(void *contents, size_t sizeOfMembers, size_t numberOfMembers, void *myPointer)
{
    size_t realSize = sizeOfMembers * numberOfMembers;
    struct Result *returnStruct = (struct Result *)myPointer;
    returnStruct->memory = realloc(returnStruct->memory, returnStruct->sizeOfMemory + realSize + 1); //I am lazy, so if we're out of memory then I don't know what happens
    memcpy(&(returnStruct->memory[returnStruct->sizeOfMemory]), contents, realSize);
    returnStruct->sizeOfMemory += realSize;
    returnStruct->memory[returnStruct->sizeOfMemory] = 0;

    return realSize; //For some reason this must be returned
}
static CURLcode submitCurlRequest(CURL* curl, struct Result* requestResult)
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
    list = curl_slist_append(list, argv[2]);
    list = curl_slist_append(list, argv[3]);
    list = curl_slist_append(list, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, "https://ipecho.net/plain");
    struct Result ip;
    submitCurlRequest(curl, &ip);

    char baseUrl[1024] = "https://api.cloudflare.com/client/v4/zones/";
    char* requestUrl[argc-4];
    for(int i = 4; i < argc; i++)
    {
        requestUrl[i-4] = malloc(1024);
    }

    int baseFilePathLength = strrchr(argv[0], '/') - argv[0];
    char* filepath = malloc(baseFilePathLength + 100);
    sprintf(filepath, "%.*s/.dyndnsCache", baseFilePathLength, argv[0]);
    FILE *readFromCache = fopen(filepath, "r");
    if(*argv[1] == '0' || !readFromCache)
    {
        noCache:
        curl_easy_setopt(curl, CURLOPT_URL, baseUrl);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
        struct Result zoneIdRequest;
        submitCurlRequest(curl, &zoneIdRequest);
        sprintf(baseUrl + strlen(baseUrl), "%.32s/dns_records", strstr(zoneIdRequest.memory, "id") + 5);
        free(zoneIdRequest.memory);
        for(int i = 4; i < argc; i++)
        {
            sprintf(requestUrl[i-4], "%s?type=A&name=%s", baseUrl, argv[i]);
            curl_easy_setopt(curl, CURLOPT_URL, requestUrl[i-4]);
            struct Result recordIdRequest;
            submitCurlRequest(curl, &recordIdRequest);
            sprintf(requestUrl[i-4], "%s/%.32s", baseUrl, strstr(recordIdRequest.memory, "id") + 5);
            free(recordIdRequest.memory);
        }
        if(*argv[1] != '0')
        {
            printf("Creating new cache file.\n");
            FILE* writeToCache = fopen(filepath, "w");
            for(int i = 4; i < argc; i++)
            {
                fprintf(writeToCache, "%s\n", requestUrl[i-4]);
            }
            fclose(writeToCache);
        }
    }
    else
    {
        printf("Reading from cache file.\n");
        for(int i = 4; i < argc; i++)
        {
            fscanf(readFromCache,"%s[^\n]", requestUrl[i-4]);
        }
        fclose(readFromCache);
    }
    for(int i = 4; i < argc; i++)
    {
        curl_easy_reset(curl);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
        curl_easy_setopt(curl, CURLOPT_URL, requestUrl[i-4]);
        struct Result cloudflareIpRequest;
        submitCurlRequest(curl, &cloudflareIpRequest);
        char* quotedArg = malloc(strlen(argv[i]) + 10);
        sprintf(quotedArg, "\"name\":\"%s\"", argv[i]);
        if(strstr(cloudflareIpRequest.memory, "\"success\":false") || !strstr(cloudflareIpRequest.memory, quotedArg))  //Check if file is corrupted or program arguments have changed
        {
            printf("The cache file is outdated or corrupted. Regenerating cache file and retrying.\n");
            free(quotedArg);
            free(cloudflareIpRequest.memory); //Free our memory before exiting!
            remove(filepath);
            goto noCache; //Retry
        }
        else
        {
            char cloudflareIp[16];
            char* beginningOfCloudflareIp = strstr(cloudflareIpRequest.memory, "content") + 10;
            sprintf(cloudflareIp, "%.*s", (int) (strstr(beginningOfCloudflareIp, "\"") - beginningOfCloudflareIp), strstr(cloudflareIpRequest.memory, "content") + 10);
            free(cloudflareIpRequest.memory);
            free(quotedArg);
            if(!strcmp(ip.memory, cloudflareIp)) //strcmp returns 0 if the strings match, so negate it
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
        }

    }
    for(int i = 4; i < argc; i++)  //Free all memory still in use
    {
        free(requestUrl[i-4]);
    }
    free(filepath);
    free(ip.memory);
    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return 0;
}
